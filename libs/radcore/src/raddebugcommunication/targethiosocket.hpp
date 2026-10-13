//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        targetHIOsocket.hpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the target side definition of sockets using
//              the gamecubes Host I/O interfaces. It is not a true sockets 
//              implementation but meets the requirements of the Debug Communication
//              system. We make many assumptions about its use. If a more fully
//              functioning sockets implemetation is required, we will do so.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	May 24, 2001
//
//=============================================================================

#ifndef	TARGETHIOSOCKET_HPP
#define TARGETHIOSOCKET_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <dolphin.h>
#include <dolphin/hio.h>

#include "socket.hpp"
#include "hioshared.hpp"
#include <radmemory.hpp>

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declaration
//=============================================================================

//
// Definition for the target side socket implementation using the GameCube HostIO
// communcation system.
//
class CTargetHIOSocket : public radSocket
{
    public:

    CTargetHIOSocket( s32 channel = 1 );
    virtual ~CTargetHIOSocket( void );

    //
    // Implement the various socket functions. Note this is not a true sockets implementation.
    //
    virtual int socket( int af, int type, int protocol);
    virtual int closesocket( int socket );
    virtual int setsockopt( int socket, int level, int optname, const char* optval, int optlen);
    virtual int accept( int socket, struct sockaddr* addr, int* addrlen);
    virtual int bind( int socket, struct sockaddr* addr, int addrlen);
    virtual int listen( int socket, int backlog);
    virtual int connect( int sock, const struct sockaddr* addr, int addrlen );
    virtual int recv( int socket, char* buf, int len, int flags);
    virtual int send( int socket, const char* buf, int len, int flags);
    virtual int lasterror( int socket );

    private:

    //
    // Static function to obtain invoked when USB channels enumerated.
    //
    static BOOL HIOEnumerationCallback( s32 chan );
    
    //
    // Static used to signal completion of asynch reads and writes
    //
    static void ReadWriteCallback( void );

    //
    // Member to perform the host communication
    //
    void DoHIOCommunication( void );

    //
    // Members to read, write and resolve state.
    //
    bool ReadData( void );
    bool WriteData( void );
    void ResolveState( void );

    //
    // Helper to round up a number to a multiple of 32
    //
    unsigned int RoundUpTo32( unsigned int value );    

    public:

    void* operator new( size_t size, radMemoryAllocator alloc )
    {
        s_Allocator = alloc;
        return( radMemoryAllocAligned( alloc, size, 32 ) );
   }    

    void operator delete( void * pMemory )
    {
	    radMemoryFreeAligned( s_Allocator, pMemory );
    }

    //
    // Helper to find socket index from channel index.
    //
    unsigned int GetSocketIndex( unsigned int channelIndex );
    
    //
    // Helper to get a free buffer.
    //
    unsigned int GetBufferIndex( HioChannelControlBlock* pControlBlock );


    private:

    //
    // These data structure are used to manage socket information 
    // maintained in memory.
    //
    enum SocketState
    {
        Free,               // Socket is not in use
        Listen,             // Socket is allocated and listening
        Connecting,         // Socket is connecting ( not currently used on target )
        Connected,          // Socket is connected,
        PendingDisconnect,  // Waiting for data to be sent before disconnect.
        Disconnected,       // Local Disconnected
        RemoteDisconnect    // Remote Disconnect
    };

    //
    // This structure is cooked to keep alignment to 32 bytes.
    //
  	struct Socket
	{
		SocketState     m_State;                          // Maintains state of the connection.
        unsigned int    m_HioChannel;                     // Shared memory index
		unsigned int    m_BytesInTxBuffer;                // Bytes queued for transmision.
		unsigned int    m_BytesInRxBuffer;                // Bytes received
        unsigned int    m_Pad[ 4 ];                       // Keep 32 byte alignment.
		unsigned char   m_TxBuffer[ HIO_CHANNEL_SIZE ];   // Buffer for sending data
		unsigned char   m_RxBuffer[ HIO_CHANNEL_SIZE ];   // Buffer for receiving data.
	};

    Socket m_SocketArray[ HIO_CHANNEL_COUNT ] ATTRIBUTE_ALIGN( 32 );

    //
    // This is used to indentiy the socket we are waiting for a connection on.
    //
    unsigned int        m_ListenSocketIndex;

    //
    // Thise member indicates the time we last hear from the host.
    //
    unsigned int        m_TimeOfLastContact;
	unsigned int        m_TimeOfLastService;

    //
    // Static flag set when a asych read or write completes.
    //
    static bool         s_AsyncReadWriteComplete;

    //
    // Maintains a count of open sockets. Used to determine shutdown condition.
    //
    unsigned int        m_OpenSocketCount;

    //
    // Static obtained when HIO devices enumerated.
    //
    static  s32         s_Channel;

    //
    // This maintains the state of our communication state machine.
    //
    enum ComState
    {
		Init,
        Idle,
        ReadingControlBlock,
        ReReadingControlBlock,
        ReadingData,
        WritingData,
        WritingControlBlock
    };
    ComState            m_ComState;
    
    //
    // Need an index to determine where we are in the current read/write process.
    //
    unsigned int        m_ReadWriteIndex;  
    
    HioMemoryMap*       m_pHioMemory;
    
    //    
    // Controls which one gets next send buffer.
    //
    unsigned int    m_LastSendIndex;    
    unsigned int    m_CurrentSendIndex;                   

    //
    // This buffer is used when we read/write the HIO shared memory control block.
    //
    HioChannelControlBlock m_ChannelControlBlock ATTRIBUTE_ALIGN( 32 );

    //
    // Need to have a static for the allocator, since this is used to free the aligned memory.
    //
    static radMemoryAllocator s_Allocator;

};

#endif 
