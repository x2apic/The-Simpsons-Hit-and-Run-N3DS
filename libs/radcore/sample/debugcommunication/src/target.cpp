//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Target.cpp
//
// Subsystem:	Foundation Technologies - Debug Com Target Sample Program
//
// Description:	This file contains sample code to illustrate communication
//              with the host application
//
// Date:    	April 20, 2001
//
// Revised by:	AP
//
// Usage:       Under Win32 simply launch. Under RAD_PS2, lauch with
//              command line arguement to control whether to use the DECI
//              or sockets for communicaiton.
//                  -DECI for deci
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <radplatform.hpp>
#include <radthread.hpp>
#include <raddebug.hpp>
#include <raddebugcommunication.hpp>
#include <radtime.hpp>


//=============================================================================
// Local Defintions
//=============================================================================

#ifdef RAD_PS2
//
// Edit these to match your configuration.
//
#define SNPS2_IP_ADDR   "10.1.10.191"
#define SNPS2_SUB_MSK   "255.255.0.0"
#define SNPS2_GATEWAY   "10.1.1.14"

#endif

//=============================================================================
// Global Data Declarations
//=============================================================================
class StatusCallback;
unsigned int ChannelSendingThreadFunction( void* userData );
//
// Pointers to classes and structures used to direct testing.
//
const unsigned int CHANNEL_COUNT = 5;
static IRadDbgComChannel* g_Channels[ CHANNEL_COUNT ] = { NULL };
static StatusCallback * g_StatusCallback[ CHANNEL_COUNT ] = { NULL };
static IRadThread * g_WorkerThreads[ CHANNEL_COUNT ] = { NULL };

//
// This variable is used to terminate our main game loop.
//
bool g_Done = false;

//=============================================================================
// Public Functions
//=============================================================================

//
// Gamecube currently requires that operators new and delete be implemented.
// We do it for all plaforms just for consistency.
//
void* operator new( size_t size )
{
    return( radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, size ) );
}

void* operator new[ ]( size_t size )
{
    return( radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, size ) );
}

void operator delete( void* ptr )
{
    radMemoryFree( ptr );
}

inline void operator delete[ ]( void* ptr )
{
    radMemoryFree( ptr );
}

//=============================================================================
// Class Declarations/Implementation
//=============================================================================

//
// This class is used to field status callbacks from the host channel.
// Currently all it does is echo the stuff to the debug console window.
//
class StatusCallback : public IRadDbgComChannelStatusCallback,
                       public IRadDbgComChannelSendCallback,
                       public IRadDbgComChannelReceiveCallback,
                       public IRadThreadMutex
{
    public:
		StatusCallback( IRadDbgComChannel* g_pChannel );
		~StatusCallback( void );
		virtual void OnStatusChange( IRadDbgComChannel::ConnectionState connectionState, const char* pMessage );
		virtual void OnSendComplete( bool Successful );
		virtual void OnReceiveComplete( bool Successful, unsigned int bytesReceived );

		//
		// Member functions implemented for IRefCound
		//
		virtual void AddRef( void );
		virtual void Release( void ); 

        virtual void Lock( void );
        virtual void Unlock( void );

        bool SendIfReady();

        unsigned int GetChannelID( )
        {
            return m_ChannelID;
        }

        void SetChannelID( unsigned int ChannelID )
        {
            m_ChannelID = ChannelID;
        }

	private:
		char   m_RxBuffer[ 20 * 1024 ];
		char   m_TxBuffer[ 20 * 1024 ];

		unsigned int m_Count;
        unsigned int m_RxPacketSize;
		unsigned int m_RxCount;
		unsigned int m_RefCount;
		IRadDbgComChannel* m_pChannel;
		
		bool m_WaitingRxComplete;
		bool m_WaitingTxComplete;
        bool m_Attached;
        unsigned int m_ChannelID;
        unsigned int m_PacketCount;
};

//=============================================================================
// Function:    StatusCallback::StatusCallback
//=============================================================================
// Description: Constructor for InputCallback
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
StatusCallback::StatusCallback( IRadDbgComChannel* g_pChannel ) :
    m_RefCount( 1 ),
    m_pChannel( g_pChannel ),
    m_WaitingRxComplete(false), 
    m_WaitingTxComplete(false),
    m_RxPacketSize( 0 ),
    m_Attached( false ),
    m_PacketCount( 0 )
{
    Lock( );
    rAssert( g_pChannel );
	m_pChannel->AddRef();
    Unlock( );
}

//=============================================================================
// Function:    InputCallback::~InputCallback
//=============================================================================
// Description: Destructor for InputCallback
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
StatusCallback::~StatusCallback( void )
{
    Lock( );
	m_pChannel->Release();
    Unlock( );
}

//=============================================================================
// Function:    InputCallback::AddRef
//=============================================================================
// Description: Get a reference count to the object holding onto now
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void StatusCallback::AddRef( void )
{
    Lock( );
    m_RefCount++;
    Unlock( );
}

//=============================================================================
// Function:    InputCallback::Release
//=============================================================================
// Description: Remove a reference count to the object that is no longer valid
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void StatusCallback::Release( void )
{
    Lock( );
    m_RefCount--;
    if( m_RefCount == 0 )
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
// Function:    StatusCallback::Lock
//=============================================================================
// Description: Lock the mutex
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void StatusCallback::Lock( void )
{
    radDbgComMutexLock( );
}

//=============================================================================
// Function:    StatusCallback::Unlock
//=============================================================================
// Description: Unlock the mutex
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void StatusCallback::Unlock( void )
{
    radDbgComMutexUnlock( );
}

//=============================================================================
// Function:    StatusCallBack::OnStatusChange
//=============================================================================
// Description: Member invoked when we get a status change.
//
// Parameters:  connectionState, message
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void StatusCallback::OnStatusChange( IRadDbgComChannel::ConnectionState connectionState, const char* pMessage )
{
    radSingleLock< StatusCallback > singleLock( this, true );
	char outbuf[ 200 ];
    
	if( connectionState == IRadDbgComChannel::Detached )
    {
        m_Attached = false;
		sprintf( outbuf, "State: [Detached], Message: [%s]\n",pMessage );
		g_Done = true;
    }

    else if( connectionState == IRadDbgComChannel::Detaching )
    {
        m_Attached = false;
        sprintf( outbuf, "State: [Detaching], Message: [%s]\n",pMessage );
    }   
    else if( connectionState == IRadDbgComChannel::Attaching )
    {
        m_Attached = false;
        sprintf( outbuf, "State: [Attaching], Message: [%s]\n",pMessage );
    }   
    else if( connectionState == IRadDbgComChannel::Attached )
    {
		sprintf( outbuf, "State: [Attached], Message: [%s]\n",pMessage );
        m_Attached = true;
        m_RxCount = 0;
		m_Count = 0;
        m_PacketCount = 0;
        m_pChannel->ReceiveAsync( m_RxBuffer, sizeof( m_RxBuffer ), this );
    }   

    rDebugString( outbuf );
}

//=============================================================================
// Function:    StatusCallBack::OnSendComplete
//=============================================================================
// Description: Member invoked when send completeed or failed.
//
// Parameters:  successful - true/false 
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void StatusCallback::OnSendComplete( bool Successful )
{
    radSingleLock< StatusCallback > singleLock( this, true );
	if( Successful )
    {
		m_Count++;
		if( m_Count == 10000 )
		{
			m_pChannel->Detach( );
		}
            
		rDebugString( "Send Success\n" );

    }
    else
    {
        rDebugString( "Send Failed\n" );
    }

	m_WaitingTxComplete = false;
}

//=============================================================================
// Function:    StatusCallBack::OnSendComplete
//=============================================================================
// Description: Member invoked when receive completeed or failed.
//
// Parameters:  successful - true/false 
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void StatusCallback::OnReceiveComplete( bool Successful, unsigned int bytesReceived )
{
    radSingleLock< StatusCallback > singleLock( this, true );
    if( Successful )
    {
        //
        // if m_RxPacketSize is 0, we are ready to recieve a new packet
        //
        if ( m_RxPacketSize == 0 )
        {
            rAssert( bytesReceived >= 4 );
            m_RxPacketSize = *(unsigned int*)( m_RxBuffer );
			m_RxPacketSize = ::radPlatformEndian32( m_RxPacketSize );
			rDebugPrintf( "Receiving [%u] bytes.\n", m_RxPacketSize );
        }

        m_RxCount += bytesReceived;

        if( m_RxCount == m_RxPacketSize )
        {
            //
            // we have recieved a complete packet
            //
            m_RxPacketSize = 0;

            m_RxCount = 0;
			
			m_WaitingRxComplete = false;

            rDebugString( "Receive Successful\n" );
        }

        m_pChannel->ReceiveAsync( (void*) m_RxBuffer, sizeof( m_RxBuffer ), this );
    }
    else
    {
        rDebugString( "Receive Failed \n" );
    }
}


//=============================================================================
// Function:    StatusCallback::SendIfReady
//=============================================================================
// Description: The Sending scheme for the CallBack.
//
// Parameters:  n/a
//
// Returns:     bool
//				false - not ready to send.
//				true  - ready to send and has issued a send already.
//
// Notes:		This method implements the specific sending scheme used in this
//				sample code.  In this case, it issues a send only when:-
//				1) the previous send is completed and
//				2) has received a response for its previous send.
//
//------------------------------------------------------------------------------
bool StatusCallback::SendIfReady()
{
    radSingleLock< StatusCallback > singleLock( this, true );

    bool result = false;		// initialize it to false i.e not ready
	if( (m_Attached == true) && (m_WaitingTxComplete == false) && (m_WaitingRxComplete == false) )
	{
        static char szMsg[ 128 ] = { NULL };
        sprintf( szMsg, "Channel [%d]", m_ChannelID );
        memset( m_TxBuffer, 0xEF, sizeof( m_TxBuffer ) );
        int size = ( rand( ) % ( sizeof( m_TxBuffer ) - sizeof( int ) * 2 + strlen( szMsg ) + 1 ) ) + sizeof( int ) * 2 + strlen( szMsg ) + 1;

        //
        // embeding size of the packet as the first item in the buffer
        //
        int * pHeader = (int*)( m_TxBuffer );
        unsigned int * pPacketCount = (unsigned int*)( (unsigned int)m_TxBuffer + sizeof( int ) );
        *pHeader = ::radPlatformEndian32( size );
        *pPacketCount = ::radPlatformEndian32( m_PacketCount );
        m_PacketCount++;
        memcpy( &m_TxBuffer[ sizeof( int ) * 2 ], szMsg, strlen( szMsg ) + 1 );
		rDebugPrintf( "Sending [%u] bytes.\n", size );

        m_pChannel->SendAsync( m_TxBuffer, size, this );
		m_WaitingTxComplete = true;
		m_WaitingRxComplete = true;
		result = true;
	}
	return result;
}

//=============================================================================
// Function:    main
//=============================================================================
// Description: Main entry to the test program.
//
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
#ifdef RAD_WIN32
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#endif

#ifdef RAD_XBOX
int _cdecl main (void)
#endif

#ifdef RAD_PS2
int main( int argc, char*   argv[ ] )
#endif
#ifdef RAD_GAMECUBE
void main( void )
#endif
{
    //
    // Display banner message.
    //
    rDebugString( "Debug Communication Target Sample Program V1.00\n" );

    radTimeInitialize( );
    radThreadInitialize( );

	//
    // First initialized the platform. Each platform has specific initialization,
    // PS2 requires path to where IRXs reside and where to load them form. We
    // also intialize the debug communication system. This is required if the
    // Radical File Server will be used.
    //
#if defined  RAD_PS2

    radMemoryInitialize( );
    radPlatformInitialize( "irx\\", IOPMediaHost );

#elif defined RAD_WIN32

    radMemoryInitialize( );
    //
    // Windows initialization requires instance and main window. This program does
    // not use a main window, so just set it to null,
    //
    radPlatformInitialize( NULL, hInstance );

#elif defined RAD_XBOX

    radMemoryInitialize( );
    //
    // XBOX does not need any info.
    //
    radPlatformInitialize( );

#elif defined RAD_GAMECUBE

    radMemoryInitialize( 0, 0 );
    //
    // XBOX does not need any info.
    //
    radPlatformInitialize( );

#endif

    //
    // randomize
    //
    ::srand( ::radTimeGetMilliseconds( ) );

#if defined RAD_PS2
    //
    // For the PS2, we accept a command line arguement. If one is present, we set
    // up debug communicaiton system for USBTcpip/
    //
    if( argc == 2 )
    {
        //
        // When using TCPIP via USB, must provide address info.
        //
    	radDbgComUsbTcpIpInitInfo IpInfo;

    	strcpy( IpInfo.m_IPAddress, argv[ 1 ] );
    	strcpy( IpInfo.m_SubMask, SNPS2_SUB_MSK );
		strcpy( IpInfo.m_Gateway, SNPS2_GATEWAY );

        radDbgComTargetInitialize( UsbTcpIp, radDbgComDefaultPort, &IpInfo );
    }
    else
    {
        //
        // Initialize using DECI as the communication avenue.
        //
        radDbgComTargetInitialize( Deci );
    }
#elif defined RAD_WIN32 || defined RAD_XBOX
    radDbgComTargetInitialize( WinSocket );
#elif defined RAD_GAMECUBE
    radDbgComTargetInitialize( HostIO );
#endif

    //
    // Get the debug communication target.
    //
    for ( int i = 0; i < CHANNEL_COUNT; i++ )
    {
        radDbgComTargetCreateChannel( 0xee10+i, &g_Channels[ i ] );
        rAssert( g_Channels[ i ] != NULL );

        g_StatusCallback[ i ] = new StatusCallback( g_Channels[ i ] );
        g_StatusCallback[ i ]->SetChannelID( i );
        g_Channels[ i ]->RegisterStatusCallback( g_StatusCallback[ i ] );
        g_Channels[ i ]->Attach( );

        radThreadCreateThread( & g_WorkerThreads[ i ], ChannelSendingThreadFunction, g_StatusCallback[ i ] );
    }

    while( !g_Done )
    {
        radThreadSleep( 5 );
        radDbgComService();
    }

	//
	// Clean up references to the objects still left in memory
	// that we don't need anymore. 
	//
    for ( int i = 0; i < CHANNEL_COUNT; i++ )
    {
        g_Channels[ i ]->RegisterStatusCallback( NULL );
        g_Channels[ i ]->Detach( );
        g_Channels[ i ]->Release( );
        g_Channels[ i ] = NULL;

        g_StatusCallback[ i ]->Release( );
        g_StatusCallback[ i ] = NULL;

        g_WorkerThreads[ i ]->WaitForTermination( );
        g_WorkerThreads[ i ]->Release( );
        g_WorkerThreads[ i ] = NULL;
    }

	radDbgComTargetTerminate( );
	radPlatformTerminate( );
    radMemoryTerminate( );
    radThreadTerminate( );

    //
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //

    #ifdef RAD_DEBUG
    radObject::DumpObjects( );    
    #endif

	rDebugString( "Stopped Debug Communication Target Sample Progam V1.00\n" );
#ifndef RAD_GAMECUBE
	return 0;
#endif
}


unsigned int ChannelSendingThreadFunction( void* userData )
{
    StatusCallback * pStatusCallback = reinterpret_cast< StatusCallback * >( userData );
    while( !g_Done )
    {
        pStatusCallback->SendIfReady( );
        radThreadSleep( 5 );
    }
    return 0;
}
