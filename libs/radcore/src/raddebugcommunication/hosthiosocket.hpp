//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        hosthiosocket.hpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the host side definition of sockets using
//              the GameCube host I/O interface. This is note a true implementation
//              of sockets but meets teh requirements of the host communication
//              system.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	May 30, 2001
//
// Notes:       To get this solution to work involved creating threads to interface
//              with the HIO driver. This system is weak in that it can not be used
//              by multiple processes. As a result one thread is designated as
//              the HIO servicer. It places is results in shared memory which is
//              accessable to multiple process. 
//
//=============================================================================

#ifndef	HOSTHIOSOCKET_HPP
#define HOSTHIOSOCKET_HPP

//=============================================================================
// Include Files
//=============================================================================

#include "socket.hpp"
#include "hioshared.hpp"

//=============================================================================
// Defintions
//=============================================================================

//
// Function signatures for the various GameCube host i/o functions.
//
typedef void (WINAPI *HIOCallback)( void );
typedef BOOL (WINAPI *HIOInit)(int channel, HIOCallback callback );
typedef void (WINAPI *HIOTerm)( void );
typedef BOOL (WINAPI *HIOReadMailbox)( unsigned int* word );
typedef BOOL (WINAPI *HIOWriteMailbox)( unsigned int word );
typedef BOOL (WINAPI *HIORead)( void* addr, void* buffer, int size );
typedef BOOL (WINAPI *HIOWrite)( void* addr, void* buffer, int size );
typedef BOOL (WINAPI *HIOReadStatus)( unsigned int* word );

//=============================================================================
// Class Declaration
//=============================================================================

//
// Definition for the host side socket implementation using the GameCube HIO system. 
// 
class CHostHioSocket : public radSocket
{
    public:
    CHostHioSocket( void );
    virtual ~CHostHioSocket( void );

    //
    // Implement the various socket functions.
    //
    virtual int socket( int af, int type, int protocol);
    virtual int closesocket( int socket );
    virtual int setsockopt( int socket, int level, int optname, const char* optval, int optlen);
    virtual int bind( int socket, struct sockaddr* addr, int addrlen);
    virtual int connect( int socket, const struct sockaddr* addr, int addrlen);
    virtual int recv( int socket, char* buf, int len, int flags);
    virtual int send( int socket, const char* buf, int len, int flags);
    virtual int lasterror( int socket );
    virtual int ioctlsocket( SOCKET s, long cmd, u_long *argp );
    virtual int AsyncSelect( SOCKET s, HWND Wnd, unsigned int wMsg, long levent );
    virtual int accept( int socket, struct sockaddr* addr, int* addrlen);
    virtual int listen( int socket, int backlog);

    private:

    //
    // These data structure are used to manage socket information 
    // maintained in shared memory.
    //
    enum SocketState
    {
        Free,               // Socket is not in use
        ToBeFreed,          // Will be freed.
        Idle,               // In use, not yet connecting ( only on host );
        Listen,             // Socket is allocated and listening (not used by host)
        Connecting,         // Socket is connecting ( not currently used on target )
        Connected,          // Socket is connected,
        PendingDisconnect,  // Waiting for data to be sent before disconnect.
        Disconnected,       // Local Disconnected
        RemoteDisconnect    // Remote Disconnect
    };

    //
    // This is the definition of a socket data struct. It is access by the host
    // i/o thread.
    //
  	struct Socket
	{
        //
        // The first part of this structure is alter by the background thread performing
        // i/o for all processes and the owner of the socket.
        //
        SocketState     m_State;                          // Maintains state of the connection.
        unsigned int    m_HioChannel;                     // Shared memory index
		unsigned int    m_BytesInTxBuffer;                // Bytes queued for transmision.
		unsigned int    m_BytesInRxBuffer;                // Bytes received
		unsigned char   m_TxBuffer[ HIO_CHANNEL_SIZE ];   // Buffer for sending data
		unsigned char   m_RxBuffer[ HIO_CHANNEL_SIZE ];   // Buffer for receiving data.
        unsigned int    m_ConnectTime;                    // Used to time-out pending connections

        HWND            m_hWnd;                             // To send socket messages to
        unsigned int    m_WndMsg;                           // Message
        Socket*         m_Socket;                           // Socket in process address space.

        HANDLE          m_ThreadHandle;                   // Thread handle
        DWORD           m_ThreadID;                       // Corresponding id       

        //
        // This member is a little weak. It is used by the thread which needs
        // to see its parent socket class. Essentially these objects are one to
        // one but may change in future.
        //
        CHostHioSocket* m_pHostHioSocket;

	};

    //
    // This is the data structure visible to all sockets, in all processes, along
    // with their background threads. It contains the array of socket information
    // along with flag indicating that a thread is currently performing the host
    // I/o operations. When a thread is started it checks this flag and will try
    // to gain it. 
    //
    struct SharedMemoryMap
    {
        Socket          m_SocketArray[ HIO_CHANNEL_COUNT ];     // Sccket data
        bool            m_HIOThreadActive;                      // Is someone doing I/O
        bool            m_HIODataInitialized;                   // Has HIO memory ever been initialzied
        unsigned int    m_LastSendIndex;                        // Controls which one gets next send buffer.
    };

    //
    // Static member is the thread enrty point.
    //
    static DWORD WINAPI HIOSocketThread( Socket* pSocket );

    //
    // Functions to perform communication with target on behalf of all processes.
    // Returns true if it is doing the communication. It receices indication if
    // if was previously doing communicaiton.
    //
    bool DoTargetCommunication( bool isDoingCommunication );
    
    //
    // Cleans up if was doing communicaiton/
    //
    void DoTargetCommunicationCleanUp( void );

    //
    // Helper to round up a number to a multiple of 32
    //
    unsigned int RoundUpTo32( unsigned int value );    

    //
    // Helper to find socket index from channel index.
    //
    unsigned int GetSocketIndex( unsigned int channelIndex );
    
    //
    // Helper to get a free buffer.
    //
    unsigned int GetBufferIndex( HioChannelControlBlock* pControlBlock );

    //
    // These data structures are used to access the shared memory and serialize
    // access to it when necessary.
    //
    HANDLE m_MutexHandle;
    
    //
    // Holds handle to shared memory object containing target information table.
    //
    HANDLE m_MemoryFileHandle;
    
    //
    // Holds the address of the shared memory image.
    //
    SharedMemoryMap*    m_pSharedData;

    //
    // Used to prevent more than one error from being displayed if we fail to 
    // intialize.
    //
    bool                m_ErrorDisplayed;
    
    //
    // The DLL module handle of the HIO DLL.
    //
    HMODULE             m_HioModule;

    //
    // The various functions we use form this DLL>
    //
    HIOInit             m_HioInit; 
    HIOTerm             m_HioTerm;
    HIOReadMailbox      m_HioReadMailbox;
    HIOWriteMailbox     m_HioWriteMailbox;
    HIORead             m_HioRead;
    HIOWrite            m_HioWrite;
    HIOReadStatus       m_HioReadStatus;

    //
    // Stuff used to maintain communication state over the i/o interface.
    //
    unsigned int        m_TimeOfLastContact;

    HioMemoryMap*       m_pHioMemory;

    //
    // This specifies the time the host will wait before disconnect from the target.
    //
    unsigned int        m_SocketConnectionTimeoutValue;

};

#endif 
