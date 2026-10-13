//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        hosthiosocket.cpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the an implementation of sockets using
//              the GameCube USB Host I/O communicaiton system.
//
//				The code contained in this file is strictly for the host.
//				This implementation makes many assumptions on how 
//              it is used. It is in no way a complete and true implementation
//              of sockets.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	May 31, 2001
//
// Notes:       This module interfaces with the game cube host i/o libary.
//              To not have to deal with manageing third party headers and
//              libraries, this module dynamically loads and bind to the
//              game cube DLL. This is not a big deal as the functions are
//              very simple and there are not many of them.
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <raddebug.hpp>
#include <radmemorymonitor.hpp>
#include "hosthiosocket.hpp"

//=============================================================================
// Constants.
//=============================================================================

#define MUTEX_NAME           "RadCoreHIOMutex"
#define MEMORYFILE_NAME      "RadCoreHIOMemory"

//
// Name of the DLL to load and bind to.
// 
#define HIODLL_NAME     "HIOD.DLL" 

//
// Time-out for target communication. Can be superceded by altering registery value.
//
#define TARGET_COMMUNICATION_TIMEOUT    5000
#define SOCKET_CONNECTION_TIMEOUT_REGISTRY_KEY    "Software\\Radical Entertainment\\rDbgCom\\SocketConnectionTimeout"
#define SOCKET_TIMEOUT_FIELD       "TimeoutValue"

//
// Time-out for connections waiting for a listen.
//
#define CONNECTION_TIMEOUT              5000

//=============================================================================
// Static Data
//=============================================================================

//=============================================================================
// Member Functions
//=============================================================================

//=============================================================================
// Function:    CHostHioSocket::CHostHioSocket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

CHostHioSocket::CHostHioSocket( void )
    :
        m_ErrorDisplayed( false ),
        m_pHioMemory( NULL )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "CHostHioSocket" );

    //
    // Set up the socket connection timeout value based on the entry in Windows Registry.
	// This allows the game team to adjust the timeout value more easily.
    //
	m_SocketConnectionTimeoutValue = TARGET_COMMUNICATION_TIMEOUT;

    HKEY RegKey;

    //
	// try to open the registry key testing if it exists or not
    //
	LONG result = RegOpenKeyEx(HKEY_CURRENT_USER,
								SOCKET_CONNECTION_TIMEOUT_REGISTRY_KEY,
								0,
								KEY_ALL_ACCESS,
								&RegKey);

	if( (result == ERROR_SUCCESS) || (result == REG_OPENED_EXISTING_KEY) )
	{
        //
		// the key exists
		unsigned int timeoutValue = 0;
	    DWORD tmpLength = sizeof( unsigned int );

		// query the field value
        LONG result = RegQueryValueEx( RegKey, SOCKET_TIMEOUT_FIELD, 0, NULL, (unsigned char *)&timeoutValue, &tmpLength);
		if(result == ERROR_SUCCESS)
		{
			// the field exists
			m_SocketConnectionTimeoutValue = timeoutValue;
		}
	}    

    RegCloseKey( RegKey );

    //
    // First lets load and bind to the GameCube host I/O Dll. This makes it so
    // we don't have to deal with third party distribution of libarires and headers.
    //
    m_HioModule = LoadLibrary( HIODLL_NAME );
    rAssertMsg( m_HioModule != NULL, "Could not load Host I/O Dll. Check installation");

    m_HioInit = (HIOInit) GetProcAddress( m_HioModule, "HIOInit"); 
    m_HioTerm = (HIOTerm) GetProcAddress( m_HioModule, "HIOExit"); 
    m_HioReadMailbox = (HIOReadMailbox) GetProcAddress( m_HioModule, "HIOReadMailbox");
    m_HioWriteMailbox = (HIOWriteMailbox) GetProcAddress( m_HioModule, "HIOWriteMailbox");
    m_HioRead = (HIORead) GetProcAddress( m_HioModule, "HIORead");
    m_HioWrite = (HIOWrite) GetProcAddress( m_HioModule, "HIOWrite");
    m_HioReadStatus = (HIOReadStatus) GetProcAddress( m_HioModule, "HIOReadStatus");
  
    //
    // First obtain access to the mutex we use to protect manipulation of 
    // shared memory.
    //
    m_MutexHandle = CreateMutex( NULL, FALSE, MUTEX_NAME );
    rAssert( m_MutexHandle != NULL );

    //
    // Now gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Gain access to the shared memory containing our socket info.
    // If it does not exist yet, we are responsible for initializing it.
    //
    m_MemoryFileHandle = CreateFileMapping( (HANDLE) 0xFFFFFFFF, NULL, PAGE_READWRITE, 0, 
                                            sizeof( SharedMemoryMap ),
                                            MEMORYFILE_NAME );
    rAssert( m_MemoryFileHandle  != NULL );

    //
    // Get the last error to see if the file already exists. If not, we are responsible
    // for initializing it.
    //
    bool AlreadyInitialized = (ERROR_ALREADY_EXISTS == GetLastError( ) );

    //
    // Map the view of the file into our process space.
    //
    m_pSharedData = (SharedMemoryMap*) MapViewOfFile( m_MemoryFileHandle, FILE_MAP_WRITE, 0, 0, 0 );
    rAssert( m_pSharedData != NULL );

    if( !AlreadyInitialized )
    {
        //
        // We are the first process launched. Lets initialized the shared memory.
        //
        for( unsigned int i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
        {
            m_pSharedData->m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
            m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
            m_pSharedData->m_SocketArray[ i ].m_State = Free;
        }

        //
        // Clear flag inidicating that no thread is currently doing the host i/o interface
        // calls. First created socket will do this. Also indicate that things need
        // to be initialized.
        //
        m_pSharedData->m_HIOThreadActive = false;
        m_pSharedData->m_HIODataInitialized = false;
        m_pSharedData->m_LastSendIndex = 0;
    }

    //
    // Since we have the shared memory visble in this process space and it has been
    // initialized, we can release the mutex.
    //
    ReleaseMutex( m_MutexHandle );
}

//=============================================================================
// Function:    CHostHioSocket::~CHostHioSocket( void )
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

CHostHioSocket::~CHostHioSocket( void )
{
    //
    // Lets clean-up. Release our resources.
    // Gain ownership of this mutex. Wait forever.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Unmap view of the file and close it.
    //
    UnmapViewOfFile( m_pSharedData );

    CloseHandle( m_MemoryFileHandle );

    //
    // Release our mutex.
    //
    ReleaseMutex( m_MutexHandle );
            
    CloseHandle( m_MutexHandle );

    //
    // Get rid of the HIO DLL.
    //
    FreeLibrary( m_HioModule );
}

//=============================================================================
// Function:    CHostHioSocket::socket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------
    
int CHostHioSocket::socket
( 
    int     af, 
    int     type,
    int     protocol
)
{
    //
    // Find a free entry in the socket pool and return it.
    // Do this under protection of our
    // mutex as this table is used by all processes and the hio thread.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    for( unsigned int i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
    {
        if( m_pSharedData->m_SocketArray[ i ].m_State == Free )
        {
            m_pSharedData->m_SocketArray[ i ].m_State = Idle;
            m_pSharedData->m_SocketArray[ i ].m_BytesInRxBuffer = 0;
            m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer = 0;
            
            ReleaseMutex( m_MutexHandle );
          
            //
            // Save the parent object in the table so the thread can get it back.
            //
            m_pSharedData->m_SocketArray[ i ].m_pHostHioSocket = this;

            //
            // We also save the address of the socket in the shared memory. This memory
            // is not guareented to be mapped into the same address space in all processes
            //
            m_pSharedData->m_SocketArray[ i ].m_Socket = &m_pSharedData->m_SocketArray[ i ];
                
            //
            // Lets create a thread to do work on behalf of this socket.
            //
            m_pSharedData->m_SocketArray[ i ].m_ThreadHandle = 
                CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) HIOSocketThread, 
                              &m_pSharedData->m_SocketArray[ i ], 0, 
                              &m_pSharedData->m_SocketArray[ i ].m_ThreadID );

            return( (int) &m_pSharedData->m_SocketArray[ i ] );
        }        
    }
    
    //
    // If we get here, we have too many connections.
    //
    ReleaseMutex( m_MutexHandle );
    
    return( INVALID_SOCKET );
}
    
//=============================================================================
// Function:    CHostHioSocket::setsockopt
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::setsockopt
( 
    int             sock, 
    int             level, 
    int             optname,
    const char*     optval,
    int             optlen
)
{
    //
    // We just return success as we know the socket behavior.
    //
    return( 0 );
}

//=============================================================================
// Function:    CHostHioSocket::bind
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::bind
( 
    int     sock,
    struct  sockaddr* addr,
    int     addrlen
)
{
    //
    // Does nothing
    //
    return( 0 );
};

//=============================================================================
// Function:    CHostHioSocket::listen
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::listen( int sock, int backlog)
{
    // Not invoked by host
    rAssert( false );
    return( -1 );
}

//=============================================================================
// Function:    CHostHioSocket::accept
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::accept( int sock, struct sockaddr* addr, int* addrlen)
{
    // Not invoked by host
    rAssert( false );
    return( -1 );
}


//=============================================================================
// Function:    CHostHioSocket::AsyncSelect
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::AsyncSelect( SOCKET s, HWND Wnd, unsigned int wMsg, long levent )
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) s ;

    //
    // Save the Window handle and the mesage 
    //
    pSocket->m_hWnd = Wnd;
    pSocket->m_WndMsg = wMsg;

    return( 0 );
}

//=============================================================================
// Function:    CHostHioSocket::closesocket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::closesocket( int sock )
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    //
    // Grab the mutex and check the state.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Check if connected. If not don't worry.
    //
    if( pSocket->m_State == Connected )
    {
        //
        // Check it we have data to send. If so set pending disconnect.
        // Else set disconnect.
        //
        if( pSocket->m_BytesInTxBuffer == 0 )
        {
            pSocket->m_State = Disconnected;
        }
        else
        {
            pSocket->m_State = PendingDisconnect;
        }
    }
    else
    {
        pSocket->m_State = ToBeFreed;
    }                   
    
    HANDLE threadHandle = pSocket->m_ThreadHandle; 

    ReleaseMutex( m_MutexHandle );

    //
    // Wait here for the thread to terminate. It will since we ininitaited a free.
    //
    WaitForSingleObject( threadHandle, INFINITE );

    CloseHandle( threadHandle );

    return( 0 );
}

//=============================================================================
// Function:    CHostHioSocket::connect
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::connect( int sock, const struct sockaddr* addr, int addrlen)
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    //
    // IP address is ignored as the connection is point to point.
    //
   
    // 
    // Set state to connecting. All stuff happens in tha background.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    rAssert( pSocket->m_State == Idle );
    pSocket->m_State = Connecting;
    pSocket->m_ConnectTime = GetTickCount( );
    
    //
    // Set the hio channel index to invalid. This will signal we have yet to obtain
    // a channel.
    //
    pSocket->m_HioChannel = HIO_CHANNEL_COUNT;
       
    ReleaseMutex( m_MutexHandle );

    return( SOCKET_ERROR );
}

//=============================================================================
// Function:    CHostHioSocket::lasterror
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::lasterror( int sock )
{

    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    //
    // This routine gets the last error. It assumes that only called when 
    // a send or receive fails or when connect not complete.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    if( (pSocket->m_State == Connected) || (pSocket->m_State == Connecting ) )
    {
        ReleaseMutex( m_MutexHandle );
        return( WSAEWOULDBLOCK );
    }
    else
    {
        ReleaseMutex( m_MutexHandle );
        return( WSAENOTCONN );
    }
}

//=============================================================================
// Function:    CHostHioSocket::recv
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::recv( int sock, char* buf, int len, int flags)
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Make sure connecteded. If not return -1;
    //
    if( pSocket->m_State != Connected )
    {
        ReleaseMutex( m_MutexHandle );
        return( -1 );
    }
    
    //
    // See if any data. If so return as much as the caller asked for.
    //
    if( pSocket->m_BytesInRxBuffer == 0 )
    {
       ReleaseMutex( m_MutexHandle );
       return( 0 );
    }
    
    if( len >= (int) pSocket->m_BytesInRxBuffer )
    {
        len = pSocket->m_BytesInRxBuffer;
        memcpy( buf, pSocket->m_RxBuffer, len );
        pSocket->m_BytesInRxBuffer = 0;
    }
    else
    {
        pSocket->m_BytesInRxBuffer -= len;
        memcpy( buf, pSocket->m_RxBuffer, len );
        
        //
        // This is lazy, but will revisit if performance is bad.
        //
        memmove( pSocket->m_RxBuffer, &pSocket->m_RxBuffer[ len ], pSocket->m_BytesInRxBuffer );
    }

    ReleaseMutex( m_MutexHandle );

    return( len );
}

//=============================================================================
// Function:    CHostHioSocket::send
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::send( int sock, const char* buf, int len, int flags)
{
    //
    // Socket passed in is actual a pointer into the shared memory socket structure
    //
    Socket* pSocket = (Socket*) sock ;

    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Make sure connecteded. If not return -1;
    //
    if( pSocket->m_State != Connected )
    {
        ReleaseMutex( m_MutexHandle );
        return( -1 );
    }
    
    //
    // See if any data. If so return as much as the caller asked for.
    //
    if( pSocket->m_BytesInTxBuffer != 0 )
    {   
        ReleaseMutex( m_MutexHandle );
        return( 0 );
    }
 
    if( len > HIO_CHANNEL_SIZE )
    {
        len = HIO_CHANNEL_SIZE;
    }

    memcpy( pSocket->m_TxBuffer, buf, len );
    pSocket->m_BytesInTxBuffer = len;

    ReleaseMutex( m_MutexHandle );

    return( len );
}

//=============================================================================
// Function:    CHostHioSocket::ioctlsocket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CHostHioSocket::ioctlsocket( SOCKET s, long cmd, u_long *argp )
{
    return( 0 );
}

//=============================================================================
// Function:    CHostHioSocket::HIOSocketThread
//=============================================================================
// Description: This is the thread that does work for the socket. One of 
//              the threads is responsible for performing i/o on behalf of
//              the all processes. This is needed because the host i/o interface
//              from Nintendo is poor.
//
// Parameters:  Pointer to socket in shared memory
//              
// Returns:     0
//
// Notes:       
//------------------------------------------------------------------------------

DWORD WINAPI CHostHioSocket::HIOSocketThread
( 
    struct Socket* pSocket
)
{
    //
    // Get the socket implementation object from the socket.
    // 
    CHostHioSocket* pHostHioSocket = pSocket->m_pHostHioSocket;

    //
    // Here we initialized a flag indicating that we are not the thread that
    // does target communition. Will change based on creation and termination
    // of processe which are using this system.
    //
    bool    doingTargetCommunicaiton = false;

    do
    {
        //
        // Be a well behaved citizen. Should developed this system to suspend exclicitly
        // waiting for changes. Sleep for now.
        //
        Sleep( 0 );

        //
        // Will do the communicaiton if no other thread is.
        //
        doingTargetCommunicaiton = pHostHioSocket->DoTargetCommunication( doingTargetCommunicaiton );

    }
    while( pSocket->m_State != ToBeFreed );

    //
    // Actually set the socket free.
    //
    pSocket->m_State = Free;    
     
    //
    // We we break out of the above loop, it means the socket was disconnected or closed.
    // If we were doing target i/o for everyone else, then miust release the Nintendo resources,
    // and set a flag so some other thread will take up the torch.
    //
    if( doingTargetCommunicaiton )
    {
        pHostHioSocket->DoTargetCommunicationCleanUp(  );
    }

    return( 0 );
}

//=============================================================================
// Function:    DoTargetCommunication:        
//=============================================================================
// Description: This function is invoked for two purposes. One is to check if
//              a thread exists for doing the host communication. If so, then
//              do nothing. If not, then this thread becomes the thread responsible
//              for performing i/o.
//
// Parameters:  boolean indicating if the thread was previously doing host i/o
//              
// Returns:     true if this thread is doing the i/o.
//              false if not.
//
// Notes:       
//------------------------------------------------------------------------------

bool CHostHioSocket::DoTargetCommunication( bool doingTargetCommunication )
{
    unsigned int i;

    if( !doingTargetCommunication )
    {
        //
        // Lets see if there is another thread doing the work. If so, we don't
        // need to do anything.
        //
        WaitForSingleObject( m_MutexHandle, INFINITE );
        
        if( m_pSharedData->m_HIOThreadActive )
        {
            //
            // Here the flag indicates that there is a thread doing host communication.
            // No need for us to do it.
            //
            ReleaseMutex( m_MutexHandle );
            
            return( doingTargetCommunication );
        }
    
        //
        // Here there is not thread doing the communication. Set ourself up as this
        // thread and initialized things.
        //
        doingTargetCommunication = true;
        m_pSharedData->m_HIOThreadActive = true;

        ReleaseMutex( m_MutexHandle );
    
        //
        // Since we have just become the owner, set time of last contact to the current
        // time. Use this to time out loss of contact with target.
        //
        m_TimeOfLastContact = GetTickCount( );

        //
        // Lets initialize. For know we just us channel zero.
        //
        if( !m_HioInit( 0, NULL ) )
        {
            //
            // Failed to initialize. Display message box if not already done so.
            //
            if( !m_ErrorDisplayed )
            {
                m_ErrorDisplayed = true;
                MessageBox( NULL, "Error: Cannot initialize I/O with GameCube. Check installation",
                            "Host Communication", MB_ICONEXCLAMATION | MB_OK );
            }

            m_pSharedData->m_HIOThreadActive = false;
            return( false );
        }

        //
        // Lets see if the actual USB HIO data needs initialization. Only done if first time
        // or if we lose contact.
        //
        if( !m_pSharedData->m_HIODataInitialized )
        {
            //
            // Lets initialize things. First lets set up the shared memory. 
            // We will give it to the target.
            //
            HioChannelControlBlock  controlBlock;
            for( i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
            {
                controlBlock.m_ChannelAllocationMap[ i ] = 0;
                controlBlock.m_HostTransmitCount[ i ] = 0;
                controlBlock.m_TargetTransmitCount[ i ] = 0;
                controlBlock.m_MessageMap[ i ] = HioChannelControlBlock::NullMessage;
            }

            for( i = 0 ; i < HIO_BUFFER_COUNT ; i++ )
            {
                controlBlock.m_HostTransitBufferOwner[ i ] = 0xffffffff;
                controlBlock.m_TargetTransitBufferOwner[ i ] = 0xffffffff;
            }

            m_pSharedData->m_HIODataInitialized = true;
            m_pSharedData->m_LastSendIndex = 0;
    
            controlBlock.m_TargetOwnsMemory = HIO_TARGETOWNSMEMORY;
            
            //
            // Lets write the data into the USB shared memory.
            //
            m_HioWrite( &m_pHioMemory->m_ChannelControlBlock, &controlBlock, sizeof( controlBlock ) );
        }
    }

    //
    // Here we are the thread doing the communication with the target. Check 
    // if ownership of the memory has been transfered to us. If not, just return ( unless time-out )
    // expired.
    //
    HioChannelControlBlock  controlBlock;

    m_HioRead( &m_pHioMemory->m_ChannelControlBlock, &controlBlock, sizeof( controlBlock ) );

    //
    // This is a hack added by Wng to deal with restarts.
    //
	if( controlBlock.m_TargetOwnsMemory == HIO_HOSTOWNSMEMORY + HIO_TARGETOWNSMEMORY )
	{
		rDebugString("Target is restarted, force itself to time out\n");
		m_TimeOfLastContact = GetTickCount() - m_SocketConnectionTimeoutValue;
	}

    if( controlBlock.m_TargetOwnsMemory != HIO_HOSTOWNSMEMORY )
    {
        //
        // Here there the value is non-zero meaning that the target has not granted us 
        // ownership or the target is not there. Check for time-out.
        //
        if( GetTickCount( ) - m_TimeOfLastContact < m_SocketConnectionTimeoutValue )
        {
            //
            // No problem.
            //
            return( doingTargetCommunication );
        }
        
        //
        // Here we have experience a time out. Set state of all non-connecting, non-idle, non-free
        // sockets to remote disconnet.
        //
        WaitForSingleObject( m_MutexHandle, INFINITE );

        for( i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
        {
            if( (m_pSharedData->m_SocketArray[ i ].m_State != Connecting) && (m_pSharedData->m_SocketArray[ i ].m_State != Free)
                && (m_pSharedData->m_SocketArray[ i ].m_State != Idle) && (m_pSharedData->m_SocketArray[ i ].m_State != ToBeFreed) )
            {
                m_pSharedData->m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
                m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
                if( m_pSharedData->m_SocketArray[ i ].m_State == Connected )
                {
                    m_pSharedData->m_SocketArray[ i ].m_State = RemoteDisconnect;
                    //
                    // Send message disconnect occured.
                    //
                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_CLOSE );
   
                }
                else
                {
                    m_pSharedData->m_SocketArray[ i ].m_State = ToBeFreed;
                }
            } 
            else if( m_pSharedData->m_SocketArray[ i ].m_State == Connecting )
            {
                if( controlBlock.m_TargetOwnsMemory == HIO_HOSTOWNSMEMORY + HIO_TARGETOWNSMEMORY )
                {
                    //
                    // This is another hack by Wng. 
   					// This is a special case: The Target is just restarted and one of the sockets 
					// is in connecting state.  We need to close this socket and eventually Hio_Init
					// will be called and we will be in business.
                    //
					m_pSharedData->m_SocketArray[ i ].m_State = Idle;

                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_CLOSE );
   
                }
                else
                {
                    //
                    // Set state to idle and post a message inidcating connection failed. 
                    //
                    m_pSharedData->m_SocketArray[ i ].m_State = Idle;
                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, 0xFFFF0000 | FD_CONNECT );
                }
            }
        }

        ReleaseMutex( m_MutexHandle );
    
        //
        // Reset the shared memory and transfer it to the target
        //
        HioChannelControlBlock  controlBlock;
        
        for( i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
        {
            controlBlock.m_ChannelAllocationMap[ i ] = 0;
            controlBlock.m_HostTransmitCount[ i ] = 0;
            controlBlock.m_TargetTransmitCount[ i ] = 0;
            controlBlock.m_MessageMap[ i ] = HioChannelControlBlock::NullMessage;
        }

        for( i = 0 ; i < HIO_BUFFER_COUNT ; i++ )
        {
            controlBlock.m_HostTransitBufferOwner[ i ] = 0xffffffff;
            controlBlock.m_TargetTransitBufferOwner[ i ] = 0xffffffff;
        }

        controlBlock.m_TargetOwnsMemory = HIO_TARGETOWNSMEMORY;
        
        //
        // Lets write the data into the USB shared memory.
        //
        m_HioWrite( &m_pHioMemory->m_ChannelControlBlock, &controlBlock, sizeof( controlBlock ) );

        m_TimeOfLastContact = GetTickCount( );

        return( doingTargetCommunication );
    }

    //
    // Here we have received ownership of the HIO memory. Re-read the control block to 
    // prevernt problems with non-atomic reads of the block.
    //
    m_HioRead( &m_pHioMemory->m_ChannelControlBlock, &controlBlock, sizeof( controlBlock ) );

    //
    // Protect access during this part.
    //
    WaitForSingleObject( m_MutexHandle, INFINITE );

    //
    // Lets first read up any received data. 
    //      
    for( i = 0 ; i < HIO_BUFFER_COUNT ; i++ )
    {
        if( controlBlock.m_TargetTransitBufferOwner[ i ] != 0xffffffff )
        {
            //
            // Here there is some data in a target transit buffer. Get the channel index.
            //
            unsigned int channelIndex = controlBlock.m_TargetTransitBufferOwner[ i ];

            //
            // Get the local socket index. 
            //
            unsigned int socketIndex = GetSocketIndex( channelIndex );

            //
            // Make sure connected. If not, free the buffer,
            //
            if( (socketIndex != HIO_CHANNEL_COUNT) && m_pSharedData->m_SocketArray[ socketIndex ].m_State == Connected )
            {
                unsigned int bytesInReceiver = controlBlock.m_TargetTransmitCount[ channelIndex ];
                   
                if( (bytesInReceiver != 0) && ( m_pSharedData->m_SocketArray[ socketIndex ].m_BytesInRxBuffer == 0) )  
                {
                    //
                    // Here we have some data to read and a place to put it. Do the read. Make sure
                    // read is a multiple of 32, always rounding up as necessary.
                    //
                    m_HioRead( &m_pHioMemory->m_TargetTransmitBuffer[ i ][0],
                               m_pSharedData->m_SocketArray[ socketIndex ].m_RxBuffer, RoundUpTo32( bytesInReceiver ) );

                    m_pSharedData->m_SocketArray[ socketIndex ].m_BytesInRxBuffer = bytesInReceiver;
                    controlBlock.m_TargetTransmitCount[ channelIndex ] = 0;
                    controlBlock.m_TargetTransitBufferOwner[ i ] = 0xffffffff;
                    //
                    // Indicate data is available.
                    //
                    PostMessage( m_pSharedData->m_SocketArray[ socketIndex ].m_hWnd, m_pSharedData->m_SocketArray[ socketIndex ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ socketIndex ].m_Socket, FD_READ );

                }
            }
            else
            {
                controlBlock.m_TargetTransitBufferOwner[ i ] = 0xffffffff;
            }     
        }
    }
      
    //
    // Now lets send any data that we can. Since there is limited transmit buffers, we start this operation
    // from the last channel to get a buffer. This way everyone gets a turn.
    //
    unsigned int socketIndex = m_pSharedData->m_LastSendIndex;

    for( i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
    {            
        if( (m_pSharedData->m_SocketArray[ socketIndex ].m_State == Connected) || (m_pSharedData->m_SocketArray[ socketIndex ].m_State == PendingDisconnect) )
        {
            unsigned int bytesInTransmitter = controlBlock.m_HostTransmitCount[ m_pSharedData->m_SocketArray[ socketIndex ].m_HioChannel ];
                   
            if( (bytesInTransmitter == 0) && (m_pSharedData->m_SocketArray[ socketIndex ].m_BytesInTxBuffer != 0) )
            {
                //
                // Here we have some data to write and a place to put it. Lets see if one of the shared buffers is
                // available.
                //
                unsigned int bufferIndex = GetBufferIndex( &controlBlock );
                if( bufferIndex != 0xffffffff )
                {
                    m_pSharedData->m_LastSendIndex = socketIndex;

                    m_HioWrite( &m_pHioMemory->m_HostTransmitBuffer[ bufferIndex] [0],
                                m_pSharedData->m_SocketArray[ socketIndex ].m_TxBuffer, RoundUpTo32( m_pSharedData->m_SocketArray[ socketIndex ].m_BytesInTxBuffer ) );

                    controlBlock.m_HostTransmitCount[ m_pSharedData->m_SocketArray[ socketIndex ].m_HioChannel ] = 
                                m_pSharedData->m_SocketArray[ socketIndex ].m_BytesInTxBuffer;
                    m_pSharedData->m_SocketArray[ socketIndex ].m_BytesInTxBuffer = 0;

                    controlBlock.m_HostTransitBufferOwner[ bufferIndex ] = m_pSharedData->m_SocketArray[ socketIndex ].m_HioChannel;

                    //
                    // Indicate transmitter is empty.
                    //
                    PostMessage( m_pSharedData->m_SocketArray[ socketIndex ].m_hWnd, m_pSharedData->m_SocketArray[ socketIndex ].m_WndMsg,
                                   (unsigned int) m_pSharedData->m_SocketArray[ socketIndex ].m_Socket, FD_WRITE );
                }
            }
        }
        
        socketIndex++;
        if( socketIndex == HIO_CHANNEL_COUNT )
        {
            socketIndex = 0;
        }
    }

    //
    // Now lets resolve state and process messages. First lets deal with connected or local disconnected
    // channels.
    //
    for( i = 0; i < HIO_CHANNEL_COUNT ; i++ )
    {
        if( (m_pSharedData->m_SocketArray[ i ].m_State == Connected) || 
            (m_pSharedData->m_SocketArray[ i ].m_State == PendingDisconnect) ||
            (m_pSharedData->m_SocketArray[ i ].m_State == Disconnected) )
        {
            //
            // Here we have a connection or are about to shut one down. We have 
            // a valid index into the message table. The only valid messages from the
            // target are disconnect or null message.
            //
            unsigned int channel = m_pSharedData->m_SocketArray[ i ].m_HioChannel;
            HioChannelControlBlock::ControlMessage message = controlBlock.m_MessageMap[ channel ];;
            rAssert( (message == HioChannelControlBlock::NullMessage) || (message == HioChannelControlBlock::DisconnectMessage ) );
 
            //
            // Lets let the target disconnect message take priority. Set our state to disconnected.
            //
            if( message == HioChannelControlBlock::DisconnectMessage )
            {
                //
                // Clear the message entry, free the channel, and set our local state. 
                // 
                controlBlock.m_MessageMap[ channel ] = HioChannelControlBlock::NullMessage;
                controlBlock.m_ChannelAllocationMap[ channel ] = 0;
                if( m_pSharedData->m_SocketArray[ i ].m_State == Connected )
                {
                    //
                    // Set it disconnected. The client will free the entry,
                    //
                    m_pSharedData->m_SocketArray[ i ].m_State = RemoteDisconnect;

                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_CLOSE );

                }
                else
                {
                    m_pSharedData->m_SocketArray[ i ].m_State = ToBeFreed; 
                }
            }
            else
            {
                //
                // No message from the target. Check our state. If connected, do nothing. If
                // disconnected, send disconnect message. If Pending disconnect, and tx buffer
                // empty, send disconnect message. Target frees channel when sees message
                //
                if( (m_pSharedData->m_SocketArray[ i ].m_State == Disconnected) ||
                    ( (m_pSharedData->m_SocketArray[ i ].m_State == PendingDisconnect) && (m_pSharedData->m_SocketArray[ i ].m_BytesInTxBuffer == 0) ) )
                {
                    controlBlock.m_MessageMap[ channel ] = HioChannelControlBlock::DisconnectMessage;
                    m_pSharedData->m_SocketArray[ i ].m_State = ToBeFreed; 
                }                                
            }
        }  
    }

    //
    // Here we have dealt with all the established or disconnected sockets. Now lets deal with
    // the connecting sockets. Lets iterate over all allocated shared memory channels.
    //
    for( i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
    {
        if( m_pSharedData->m_SocketArray[ i ].m_State == Connecting )
        {
            //
            // Check if we have allocated a channel. If not do so now and send the message.
            //
            if( m_pSharedData->m_SocketArray[ i ].m_HioChannel == HIO_CHANNEL_COUNT )
            {
                //
                // Note allocated. There had better be a free channel or logic is hooped.
                //
                for( unsigned int channel = 0 ; channel < HIO_CHANNEL_COUNT ; channel++ )
                {
                    if( controlBlock.m_ChannelAllocationMap[ channel ] == 0 )
                    {
                        //
                        // Set it allocated and send the message.
                        //
                        controlBlock.m_ChannelAllocationMap[ channel ] = 1;
                        controlBlock.m_HostTransmitCount[ channel ] = 0;
                        controlBlock.m_TargetTransmitCount[ channel ] = 0;
                        controlBlock.m_MessageMap[ channel ] = HioChannelControlBlock::ConnectRequestMessage;
                        m_pSharedData->m_SocketArray[ i ].m_HioChannel = channel;

                        break;
                    }
                }
                
            }
            else
            {
                //
                // We have a channel. The response must eithor be a connect accept or reject.
                //
                unsigned int channel = m_pSharedData->m_SocketArray[ i ].m_HioChannel;
                HioChannelControlBlock::ControlMessage message = controlBlock.m_MessageMap[ channel ];
                rAssert( (message == HioChannelControlBlock::ConnectAcceptMessage ) ||
                         (message == HioChannelControlBlock::ConnectRejectMessage ) );
            
                if( message == HioChannelControlBlock::ConnectAcceptMessage )
                {
                    //
                    // Set state to connected and inform the client of the successful connect.
                    //
                    m_pSharedData->m_SocketArray[ i ].m_State = Connected;
                    controlBlock.m_MessageMap[ channel ] = HioChannelControlBlock::NullMessage;

                    PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, FD_CONNECT );
                }
                else
                {
                    //
                    // Message was a connect reject. Check time-out if not expired, sent another connect.
                    //
                    if( GetTickCount( ) - m_pSharedData->m_SocketArray[ i ].m_ConnectTime > CONNECTION_TIMEOUT )
                    {
                        //
                        // Set state to idle and post a message inidcating connection failed. Free channel
                        //
                        controlBlock.m_MessageMap[ channel ] = HioChannelControlBlock::NullMessage;
                        controlBlock.m_ChannelAllocationMap[ channel ] = 0;
                        m_pSharedData->m_SocketArray[ i ].m_State = Idle;
                        PostMessage( m_pSharedData->m_SocketArray[ i ].m_hWnd, m_pSharedData->m_SocketArray[ i ].m_WndMsg,
                                 (unsigned int) m_pSharedData->m_SocketArray[ i ].m_Socket, 0xFFFF0000 | FD_CONNECT );
                    }
                    else
                    {
                        //
                        // Send another connect request.
                        //
                        controlBlock.m_MessageMap[ channel ] = HioChannelControlBlock::ConnectRequestMessage;
                    } 
                }
            }
        }
    }

    ReleaseMutex( m_MutexHandle );

    //
    // Phew!! Everything is tiddly. Lets write the control block and transfer ownership to target.
    //
    controlBlock.m_TargetOwnsMemory = HIO_TARGETOWNSMEMORY;

    m_HioWrite( &m_pHioMemory->m_ChannelControlBlock, &controlBlock, sizeof( controlBlock ) );

    m_TimeOfLastContact = GetTickCount( );

    return( doingTargetCommunication );
}

//=============================================================================
// Function:    DoTargetCommunicationCleanUp
//=============================================================================
// Description: Release the Nintendo host I/O resources so another thread can
//              use it.
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

void CHostHioSocket::DoTargetCommunicationCleanUp( void )
{
    m_HioTerm( );

    //
    // Indicate that some other thread should take up the torch.
    //
    m_pSharedData->m_HIOThreadActive = false;
}

//=============================================================================
// Function:    CHostHIOSocket::RoundUpTo32
//=============================================================================
// Description: Member to round number up to multiple of 32
//
// Parameters:  unsinged int
//              
// Returns:     converted usinged int.
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CHostHioSocket::RoundUpTo32
( 
    unsigned int value
)
{
    if( value % 32 == 0 )
    {
        return( value );
    }
    else
    {
        return( ((value / 32) * 32) + 32 );
    }
}

//=============================================================================
// Function:    CHostHIOSocket::GetSocketIndex
//=============================================================================
// Description: Helper to translate the channel index to a socket index.
//
// Parameters:  unsinged int
//              
// Returns:     converted usinged int.
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CHostHioSocket::GetSocketIndex( unsigned int channelIndex )
{
    for( unsigned int i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
    {
        if( m_pSharedData->m_SocketArray[ i ].m_HioChannel == channelIndex )
        {
            return( i );
        }
    }

    return( HIO_CHANNEL_COUNT );
}

//=============================================================================
// Function:    CHostHIOSocket::GetBufferIndex( )
//=============================================================================
// Description: Helper to find a free buffer,
//
// Parameters:  unsinged int
//              
// Returns:     converted usinged int.
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CHostHioSocket::GetBufferIndex( HioChannelControlBlock* pControlBlock )
{
    for( unsigned int i = 0; i < HIO_BUFFER_COUNT ; i++ )
    {
        if( pControlBlock->m_HostTransitBufferOwner[ i ] == 0xffffffff )
        {
            return( i );
        }
    }
    return( 0xffffffff );
}


