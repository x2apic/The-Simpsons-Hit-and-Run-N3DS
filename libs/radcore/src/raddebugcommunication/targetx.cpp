//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        targetx.cpp
//
// Subsystem:   Radical Debug Communication Target
//
// Description:	This file contains the implementation of the Debug Communication
//              target. 
//
// Author:		PGM
//
// Revisions:	Nov 17, 2000    Creation
//
// Notes:       The x at the end of the file name is to prevent problems with the make.
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#ifdef WIN32
#include <windows.h>
#endif // WIN32
#ifdef RAD_XBOX
#include <xtl.h>
#endif // RAD_XBOX 

#include <stdlib.h>

#ifdef RAD_PS2
#ifdef SN_TCPIP
extern "C"
{
#include "../../../Sdks/sntcpip/inc/snsocket.h"
#include "../../../Sdks/sntcpip/inc/sneeutil.h"
#include "../../../Sdks/sntcpip/inc/snskdefs.h"
}
#else
#define SO_NBIO         0x1013          /* Set socket non-blocking */
#endif
#include <sifrpc.h>
#endif

#include <string.h>
#include <radthread.hpp>
#include <radplatform.hpp>
#include <radmemorymonitor.hpp>
#include "targetconnection.hpp"     // Connection objects
#include "targetx.hpp"              // This module
#include "targetdecichannel.hpp"    // Deci channel implementation
#include "targetsocketchannel.hpp"  // Socket channel implementation

#ifdef RAD_GAMECUBE
    #include "targethiosocket.hpp"
#endif

#ifdef RAD_PS2
    #include "target1394socket.hpp"     // Socket implementation using 1394 FireWire
#endif

#undef NULL
#define NULL 0

//=============================================================================
// Local Defintions
//=============================================================================

//
// This constant governs the period at which we check for incoming connections.
//
const unsigned int CheckForConnectTimeout = 100;    // Milliseconds

#ifdef RAD_PS2
#define INVALID_SOCKET -1 
#endif

//=============================================================================
// Static Data Defintions
//=============================================================================

//
// This static maintains a pointer to the signleton debug com target.
//
static rDbgComTarget* s_theTarget = NULL;

//
// This static mutex is the mutex used by all debug communication system
// and those systems relies on debugcommunication.
// It is exported via radDbgComMutexLock( ) and radDbgComMutexUnlock( ).
//
static IRadThreadMutex * s_theDbgComSystemThreadMutex = NULL;

//=============================================================================
// Function:    radDbgComTargetInitialize
//=============================================================================
// Description: Invoke this member to construct the target system. Must not
//              be called more than once.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radDbgComTargetInitialize
( 
    radDbgComType targetType,
    unsigned short port,
    void* pInitInfo,
    radMemoryAllocator alloc
)
{
#ifndef RADDEBUGCOMMUNICATION

    (void) targetType;
    (void) port;
    (void) pInitInfo;
    (void) alloc;

#else

    rAssertMsg( s_theTarget == NULL, "radDebugCommunication already initialized" );

    if ( s_theDbgComSystemThreadMutex == NULL )
    {
        radThreadCreateMutex( & s_theDbgComSystemThreadMutex, alloc );
        rAssert( s_theDbgComSystemThreadMutex );
    }

    radDbgComMutexLock( );

    rDbgComTarget * pTarget = new( alloc ) rDbgComTarget( );

    pTarget->Initialize( targetType, port, pInitInfo, alloc );

    s_theTarget = pTarget;

    radDbgComMutexUnlock( );
#endif

};

//=============================================================================
// Function:    radDbgComTargetTerminate
//=============================================================================
// Description: Invoke this member to shut dowm the communication system.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radDbgComTargetTerminate( void )
{

#ifdef RADDEBUGCOMMUNICATION

    radDbgComMutexLock( );

    rAssertMsg( s_theTarget != NULL, "radDebugCommunication not initialized" );    
    
    s_theTarget->Terminate( );
    radRelease( s_theTarget, NULL );
    
    //
    // Here the static should have been zerod. If it is not zero, it indicates
    // clients are  still holding channels and did not clean up.
    //
    rAssertMsg( s_theTarget == NULL, "radDebugCommunicaiton detected clients still holding channels");

    radDbgComMutexUnlock( );

    radRelease( s_theDbgComSystemThreadMutex, NULL );

    s_theDbgComSystemThreadMutex = NULL;
#endif

}

//=============================================================================
// Function:    radDbgComService
//=============================================================================
// Description: Invoke this member to drive the asynchrnonous processing
//              required by the debug communication system.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radDbgComService( void )
{

#ifdef RADDEBUGCOMMUNICATION

    radDbgComMutexLock( );

    if( s_theTarget != NULL )
    {
        s_theTarget->Service( );
    }

    radDbgComMutexUnlock( );

#endif
}

//=============================================================================
// Function:    radDbgComService
//=============================================================================
// Description: 
// Use this interface to obtain a target comminication channel. The channel is initially 
// not connected. If the specified protocol is already in use on the specified target,
// then NULL is returned or if the system has not been initialized.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radDbgComTargetCreateChannel
( 
    unsigned short protocol, 
    IRadDbgComChannel** ppChannel,
    radMemoryAllocator alloc
)
{
#ifndef RADDEBUGCOMMUNICATION

    (void) protocol;
    (void) alloc;

    *ppChannel = NULL;

#else

    static bool bWarningShown = false;
    if( s_theTarget == NULL )
    {
        if ( bWarningShown == false )
        {
            rDebugString( "Warning: Debug Communication System not initialized\n");
            bWarningShown = true;
        }
        *ppChannel = NULL;
    }
    else
    {
        s_theTarget->CreateChannel( protocol, ppChannel, alloc );
    }

#endif

}

//=============================================================================
// Function:    radDbgComMutexLock
//=============================================================================
// Description: Mutex lock for debugcommunication system
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radDbgComMutexLock( )
{
    if ( s_theDbgComSystemThreadMutex == NULL )
    {
        radThreadCreateMutex( & s_theDbgComSystemThreadMutex );
        rAssert( s_theDbgComSystemThreadMutex );
    }
    s_theDbgComSystemThreadMutex->Lock( );
}

//=============================================================================
// Function:    radDbgComMutexUnlock
//=============================================================================
// Description: Mutex unlock for debugcommunication system
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

void radDbgComMutexUnlock( )
{
    if ( s_theDbgComSystemThreadMutex != NULL )
    {
        s_theDbgComSystemThreadMutex->Unlock( );
    }
}

//=============================================================================
// Public Member Functions
//=============================================================================

#ifdef RADDEBUGCOMMUNICATION

//=============================================================================
// Function:    rDbgComTarget::rDbgComTarget
//=============================================================================
// Description: Constructor for the target object.
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComTarget::rDbgComTarget( void )
    :
    m_TimerList( NULL ),
    m_ReferenceCount( 1 ),
    m_Timer( NULL )
{
    Lock( );

    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rDbgComTarget" );
    //
    // Initialize our array of protocols to be all free.
    //
    for( unsigned int i = 0 ; i < MaxProtocols ; i++ )
    {
        m_ProtocolTable[ i ].m_State = ProtocolInfoEntry::Free;
        m_ProtocolTable[ i ].m_pConnection = NULL;      
        m_ProtocolTable[ i ].m_pChannel = NULL;
        m_ProtocolTable[ i ].m_HostComputerName[ 0 ] = '\0';
    }

    Unlock( );
}

//=============================================================================
// Function:    rDbgComTarget::~rDbgComTarget
//=============================================================================
// Description: Destructor for the target object. 
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------

rDbgComTarget::~rDbgComTarget( void )
{
    Lock( );

    for( unsigned int i = 0 ; i < MaxProtocols ; i++ )
    {
        rAssert( m_ProtocolTable[ i ].m_State == ProtocolInfoEntry::Free );
    }

	#if defined( RAD_WIN32 ) || defined( RAD_XBOX )
    {
        //
        // Terminate our access to sockets for this process.
        //
        WSACleanup( );

        #ifdef RAD_XBOX
        {
            #ifndef OLD_XDK
                XNetCleanup();
            #else
                XnetCleanup();
            #endif
        }
        #endif // RAD_XBOX

        delete m_SocketImp;
     }
    #endif
     
    #ifdef RAD_PS2

    if( m_TargetType == UsbTcpIp )
    {
        #ifdef SN_TCPIP
        //
        // PS2 shutdown SNs stack. 
        //
        sockAPIderegthr();
        #endif
    }
    if( m_TargetType != Deci )
    {
        delete m_SocketImp;
    }

    #endif

    #ifdef RAD_GAMECUBE

    delete (CTargetHIOSocket*) m_SocketImp;
 
    #endif

    radRelease( m_Dispatcher, this );

    //
    // Finally clear the global indicating this system can be unloaded.
    //
    s_theTarget = NULL;

    Unlock( );
}            

//=============================================================================
// Function:    rDbgComTarget::Initialize
//=============================================================================
// Description: This member must be invokd prior to obtaining a channel. 
//              
// Parameters:  Target type - specifies low-level com channel
//              port - what port to use for communicaiton,
//              initInfo - additional information used for initialization
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTarget::Initialize
( 
    radDbgComType   targetType, 
    unsigned short  port,
    void*           initInfo,
    radMemoryAllocator alloc
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    int result;

    //
    // Save target type and port.
    //
    m_TargetType = targetType;
    m_Port = port;
    m_Allocator = alloc;

    //
    // Create dispatcher. We use this to drive other processing in this subsystem. 
    //
    radDispatchCreate( &m_Dispatcher, 10, alloc );
    rAssert( m_Dispatcher );
    radMemoryMonitorReportAddRef( m_Dispatcher, this );

	//
	// Create a timer list for timeout
	//
    radTimeCreateList( &m_TimerList, 10, alloc );
    rAssert( m_TimerList );
    radMemoryMonitorReportAddRef( m_TimerList, this );
    
    //
    // Lets initialize sockets. For now, if RAD_PS2 we assume the SNTCPIP stack and for Windows
    // just the Windows Sockets.
    //
	#if defined( RAD_WIN32 ) || defined( RAD_XBOX )
    if( m_TargetType == WinSocket )
    {
        //
        // Lets initialize sockets on for the calling processes.We want a socket implementation
        // of version 2.0 or greater. If less than 2.0, still use it.
        //
 
	    #ifdef RAD_XBOX
        #ifndef OLD_XDK

        XNetStartupParams xnsp;
        memset(&xnsp, 0, sizeof(xnsp));
        xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
        xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY; 
        XNetStartup(&xnsp);

        #else
    	    XnetInitialize( NULL, true );
        #endif
        #endif // RAD_XBOX

	    WORD    wVersionRequested;
        WSADATA wsaData;
        int     err;
 
        wVersionRequested = MAKEWORD( 2, 0 );
        err = WSAStartup( wVersionRequested, &wsaData );

        rAssert( err == 0 );

        m_SocketImp = new( alloc ) radSocket( );
    }
    #endif

    #ifdef RAD_PS2

    //
    // Now lets perform some initialization. The DECI system requires no explicit initialization
    // 
    if( m_TargetType == Deci )
    {
        return;
    }
    else if( m_TargetType == UsbTcpIp )
    {
        //
        // This code has been removed. Uncomment if you want to use TCPIP over USB. Performance
        // was very poor.
        //
        rAssert( false );
    
        #ifdef SN_TCPIP           
        
        //
        // Lets load and initialize SNs tcpip stack.
        //
        sceSifInitRpc(0);

        //
        // Load up the IRX modules. Need the USB drive and sns stack. Need to build up
        // the IP address information correctly. Check if the Modules are all ready in place.
        //
        if( SN_EIOPNORESP == sockAPIinit(1) )
        {
            radDbgComUsbTcpIpInitInfo*   pAddr =  (radDbgComUsbTcpIpInitInfo*) initInfo;
            char iop_params[ 512 ];
            unsigned int x = 0;
            strcpy( &iop_params[ x ], pAddr->m_IPAddress );
            x += (strlen( &iop_params[ x ] ) + 1 );
            strcpy( &iop_params[ x ], pAddr->m_SubMask );
            x += (strlen( &iop_params[ x ] ) + 1 );
            strcpy( &iop_params[ x ], pAddr->m_Gateway );
            x += (strlen( &iop_params[ x ] ) + 1 );
 
            IRadPlatform* pIPs2Platform;
            pIPs2Platform = radPlatformGet( );
	        pIPs2Platform->LoadIrxModule( "usbd.irx" );
	        pIPs2Platform->LoadIrxModule( "sndbget.irx", x, iop_params  );
	        
            result = sockAPIinit(1);
            rAssert( result == 0 );
        } 

        //
        // Register this thread.
        //
        sockAPIregthr();

        //
        // Wait to attach 
        //
        sn_bool Attached = SN_FALSE;
        short   idVendor;
        short   idProduct;

        while( Attached == SN_FALSE)
        {
            result = snmdm_get_attached(&Attached, &idVendor, &idProduct);
            rAssert( result == 0 );
            sn_delay(10);
        }

        //
        // Start the stack.
        //
        int currentState;
        sn_stack_state( SN_STACK_STATE_START, &currentState);
  
        //
        // Wait a couple seconds as per the example.
        //
        sn_delay(4000);

        m_SocketImp = new( alloc ) radSocket( );

        #endif

    }
    else if( m_TargetType == FireWire )
    {
        m_SocketImp = new( alloc ) CTarget1394Socket( );
    }
    else
    {
        rAssertMsg( false, "Invalid com type specified\n");
    }
    #endif

    //
    // For the GameCube, we have a socket emulation using HIO
    //
    #ifdef RAD_GAMECUBE
 
    rAssert( m_TargetType == HostIO );

    s32 hioChannel = 1;
    if( initInfo != NULL )
    {
        hioChannel = ((radDbgComGameCubeInitInfo *)initInfo)->m_HioChannel;
    }

    m_SocketImp = new( alloc ) CTargetHIOSocket(hioChannel);

    #endif

    //
    // Lets begin the set up of sockets. First create a socket we will use for
    // listening for host connections.
    //
    m_ListeningSocket = m_SocketImp->socket( AF_INET, SOCK_STREAM, 0 );          
    rAssert( m_ListeningSocket > 0 );
 
    //
    // Set socket options so we are allowed to re-use the address.
    //
    int ReuseAddr = 1;
    result = m_SocketImp->setsockopt( m_ListeningSocket, SOL_SOCKET, SO_REUSEADDR, (const char*) &ReuseAddr, sizeof( int ) );
    rAssert( result == 0 );

#ifndef RAD_XBOX
    int KeepAlive = 1;
    result = m_SocketImp->setsockopt( m_ListeningSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*) &KeepAlive, sizeof( int ) );
    rAssert( result == 0 );    
#endif // !RAD_XBOX

    //  
    // Bind the socket using the specified port.
    //
    sockaddr_in sockAddr;
	memset( &sockAddr, 0, sizeof(sockAddr) );
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl( INADDR_ANY );
    sockAddr.sin_port = htons( m_Port );
       
    result = m_SocketImp->bind( m_ListeningSocket, (struct sockaddr*) &sockAddr, sizeof( sockAddr ) );
    rAssert( result == 0 );

    //  
    // Set the socket option to be non-blocking. Must use two different techiques.
    //
    #ifdef RAD_PS2
    int NonBlocking = 1;
    result = m_SocketImp->setsockopt( m_ListeningSocket, SOL_SOCKET, SO_NBIO, (const char*) &NonBlocking, sizeof( int ) );
    rAssert( result == 0 );    
    #endif
    
	#if defined( RAD_WIN32 ) || defined( RAD_XBOX )
    unsigned long NonBlocking = 1;
    result = m_SocketImp->ioctlsocket( m_ListeningSocket, FIONBIO, &NonBlocking );
    rAssert( result == 0 );    
    #endif
    
    //
    // Now lets listen for incoming connections.
    //
    result = m_SocketImp->listen( m_ListeningSocket, 5 );
    rAssert( result == 0 );    
    
    //
    // Lets create a timer. We use this to periodically check for 
    // connections
    //
    m_TimerList->CreateTimer( &m_Timer, CheckForConnectTimeout, this );
    rAssert( m_TimerList );
    radMemoryMonitorReportAddRef( m_TimerList, this );
}

//=============================================================================
// Function:    rDbgComTarget::Terminate
//=============================================================================
// Description: This member is invoked once at the end to close down any pending
//              connections.
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTarget::Terminate
( 
    void
)
{

#if defined( RAD_WIN32 ) || defined( RAD_XBOX ) || defined( RAD_GAMECUBE )

    radSingleLock< IRadThreadMutex > singleLock( this, true );

    radRelease( m_Timer, this );

    //
    // Close our listening socket.
    //
    m_SocketImp->closesocket( m_ListeningSocket );

#endif

#ifdef RAD_PS2

    if( m_TargetType != Deci )
    {
        radRelease( m_Timer, this );
  
        //
        // Close our listening socket.
        //
        m_SocketImp->closesocket( m_ListeningSocket );
    }

#endif

    //
    // Deactivate any pending connections.
    //
    for( unsigned int i = 0 ; i < MaxProtocols ; i++ )
    {
        if( m_ProtocolTable[ i ].m_State == ProtocolInfoEntry::ConnectionPending )
        {
            m_ProtocolTable[ i ].m_pConnection->Deactivate( );
        }        
    }

#ifdef RAD_PS2

    //
    // It is possible for oustanding operations to exist. These operations
    // can complete as a result of iterrupts. To prevent problems, spin
    // here for a bit. This will only occur at terminatation of the game
    // and will not be probibitative.  Service the dispatcher while waiting 
    // for timer to expire. This is not a great solution but will do.
    //
    unsigned int StartTime = radTimeGetMilliseconds( );
        
    while( radTimeGetMilliseconds( ) - StartTime < 2000 )
    {
        m_TimerList->Service( );
        m_Dispatcher->Service( );
    }

#endif

    //
    // Spin here until no more dispatch events are queued.
    //
    while( 0 != m_Dispatcher->Service( ) )
    {
        m_TimerList->Service( );
    }
    radRelease( m_TimerList, this );
}

//=============================================================================
// Function:    rDbgComTarget::Service
//=============================================================================
// Description: This member is invoked to drive the asychronous processing of
//              this subsystem,
//              
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTarget::Service( void )
{
    Lock( );

    //
    // Simply drive our internal dispatcher and timer list.
    //
    m_Dispatcher->Service( );

    if( m_TimerList != NULL )
    {
        m_TimerList->Service( );
    }
    Unlock( );
}


//=============================================================================
// Function:    rDbgComTarget::CreateChannel
//=============================================================================
// Description: Use this member to create a channel for communication, Must
//              specify a non-zero protocol and it must not be in use.
//
// Parameters:  protocol    - what protocol to use.
//              ppChannel   - returns a channel object to use for communcation.
//           
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rDbgComTarget::CreateChannel
( 
    unsigned short          protocol,
    IRadDbgComChannel**     ppChannel,
    radMemoryAllocator      alloc    
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Validdate protocol
    //
    rAssert( (protocol >= 0xE000) && (protocol < 0xF000) );

    //
    // Based on the target type, construct the appropaite target channel object. Currently
    // we support DECI and sockets target channels.
    //
#if defined( RAD_WIN32 ) || defined( RAD_XBOX ) || defined( RAD_GAMECUBE )

     *ppChannel = new( alloc ) rDbgComSocketTargetChannel( this, protocol );        

#endif

#ifdef RAD_PS2    

    if( m_TargetType == Deci )
    {
        //
        // Construct up a DECITargetChannel
        //
        *ppChannel = new( alloc ) rDbgComDECITargetChannel( this, protocol );

    }
    else
    {
        //
        // Here we are using standard sockets for our communication. Construct 
        // the socket channel object.
        //  
        *ppChannel = new( alloc ) rDbgComSocketTargetChannel( this, protocol );        
    }

#endif
}

//=============================================================================
// Function:    rDbgComTarget::OnTimerDone
//=============================================================================
// Description: This member is invoked periodically to check drive our asychnronous
//              processing.
//
// Parameters:  not used
//           
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void rDbgComTarget::OnTimerDone
( 
    unsigned int    elapsedtime,
    void*           userData
)
{
    (void) userData;
    (void) elapsedtime;

    radSingleLock< IRadThreadMutex > singleLock( this, true );

    //
    // Lets check to see if anyone has called our socket.
    //
    SOCKET lsocket = m_SocketImp->accept( m_ListeningSocket, NULL, NULL );

    if( lsocket != INVALID_SOCKET )
    {
        //
        // Here the some host application has connected to us. Lets new
        // up a connection object. Note that it is intentional that the object
        // is not assigned to anything. The object internal maintains its lifetime.
        //
        rDbgComTargetConnection* p = new( m_Allocator ) rDbgComTargetConnection( lsocket, this );

        //
        // James Tan : don't trigger radMemoryMonitorReportRelease( ) here
        // 
        p->Release( );
    }       
}

//=============================================================================
// Function:    rDbgComTarget::FindProtocol
//=============================================================================
// Description: This member is invoked to find a protocol. If found, returns 
//              the protocolinfoentry record. 
//
// Parameters:  protocol
//              
// Returns:     NULL, not found
//
// Notes:
//------------------------------------------------------------------------------

ProtocolInfoEntry* rDbgComTarget::FindProtocol
( 
    unsigned short protocol
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    for( unsigned int i = 0 ; i < MaxProtocols ; i++ )
    {
        if( (m_ProtocolTable[ i ].m_State != ProtocolInfoEntry::Free) && (m_ProtocolTable[ i ].m_Protocol == protocol) )
        {
            return( &m_ProtocolTable[ i ] );
        }
    }
        
    return( NULL );
}

//=============================================================================
// Function:    rDbgComTarget::FindFreeProtocol
//=============================================================================
// Description: This member is invoked to find a free protocol entry. If one
//              found, returns the record
//
// Parameters:  n/a
//
// Returns:     NULL - no free records
//
// Notes:
//------------------------------------------------------------------------------

ProtocolInfoEntry* rDbgComTarget::FindFreeProtocol
( 
    void
)
{
    radSingleLock< IRadThreadMutex > singleLock( this, true );

    for( unsigned int i = 0 ; i < MaxProtocols ; i++ )
    {
        if( m_ProtocolTable[ i ].m_State == ProtocolInfoEntry::Free )
        {
            return( &m_ProtocolTable[ i ] );
        }
    }
        
    return( NULL );
}

//=============================================================================
// Function:    rDbgComTarget::Lock
//=============================================================================
// Description: Mutex Locking
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTarget::Lock( )
{
    radDbgComMutexLock( );
}

//=============================================================================
// Function:    rDbgComTarget::Unlock
//=============================================================================
// Description: Mutex unlocking
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void rDbgComTarget::Unlock( )
{
    radDbgComMutexUnlock( );
}

//=============================================================================
// Function:    rDbgComTarget::AddRef
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComTarget::AddRef( void )
{
    Lock( );
    m_ReferenceCount++;
    Unlock( );
}

//=============================================================================
// Function:    rDbgComTarget::Release
//=============================================================================
// Description: This member is used to update the reference count of the object.
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
    
void rDbgComTarget::Release( void )
{
    Lock( );
    m_ReferenceCount--;
    if( m_ReferenceCount == 0 )
    {
        Unlock( );
        delete this;
    }
    else
    {
        Unlock( );
    }
}

//=============================================================================
// Function:    rDbgComTarget::Dump
//=============================================================================
// Description: This member is used to display object info
//
// Parameters:  none
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_DEBUG
void rDbgComTarget::Dump( char* pStringBuffer, unsigned int bufferSize )
{
    Lock( );
    sprintf( pStringBuffer, "Object: [radDbgComTarget] At Memory Location:[%p]\n", this );
    Unlock( );
}

#endif

#endif
