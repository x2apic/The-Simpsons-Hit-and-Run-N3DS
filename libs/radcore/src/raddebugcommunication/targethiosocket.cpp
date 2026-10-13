//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        targethiosocket.cpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the an implementation of sockets using
//              the Game Cube Host I/O system. It in no way is a true
//              implementation of the socket definition. It makes many
//              assumptions about is use by the debug communication system.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	May 24, 2001
//
// Notes:       This file is intented to be built only for the Game Cube.
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <dolphin.h>
#include <dolphin/hio.h>

#include <string.h>
#include <stdio.h>
#include <raddebug.hpp>
#include <radtime.hpp>
#include <radplatform.hpp>
#include <radmemorymonitor.hpp>

#include "targethiosocket.hpp"

//=============================================================================
// Local Definitions
//=============================================================================

//
// This constant governs how long we will wait before assuming the host is 
// not longer present. Time is express in milliseconds
//
#define HOST_COMMUNICATION_TIMEOUT          3000  

//
// This constant is used to determine if the program is being debugged.
// If the DoHioCommunication is not called before this timeout interval, it is assumed
// that the program is being debugged.  Then, the host communication connection timeout
// is ignored.
//
#define DETECT_DEBUG_TIMEOUT                1000

//
// This constant is used to specify how many USB connection in a GameCube.
//
#define NUM_OF_USB_CONNECTION               2

//=============================================================================
// Statics
//=============================================================================

s32     CTargetHIOSocket::s_Channel = -1;
bool    CTargetHIOSocket::s_AsyncReadWriteComplete = false;
radMemoryAllocator CTargetHIOSocket::s_Allocator;

//=============================================================================
// Member Functions
//=============================================================================

//=============================================================================
// Function:    CTargetHIOSocket::CTargetHIOSocket
//=============================================================================
// Description: Initialize the host i/o target system. Future implentation
//              may take a EXTUSB number.
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

CTargetHIOSocket::CTargetHIOSocket( s32 channel )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "CTargetHIOSocket" );
    //
    // Initialized our local data structures to all free and indicate that 
    // we are not listening.
    //
    for( unsigned int i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
    {
        m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
        m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
        m_SocketArray[ i ].m_State = Free;
    }

    m_ListenSocketIndex = HIO_CHANNEL_COUNT + 1;
    m_OpenSocketCount = 0;
    m_LastSendIndex = 0;

    //
    // Initialize the Game Cube Host i/o system. Currently this call is not working if
    // adapter not present. Will fix later. Note. Do not use channel zero as it will 
    // interfer with serial communication. Bug acknowledged by Nintendo
    //
//    HIOEnumDevices( HIOEnumerationCallback );
//    
//    if( s_Channel == -1 )
//    {
//        rDebugString( "Warning: Failed to initialized Host I/O commumication\n");
//        return;
//    }

    s_Channel = 1;

    if( HIOInit(channel, NULL) )
    {
        s_Channel = channel;
        rDebugPrintf("Successfully initialized Host I/O communication with channel %d.\n", channel);
    }

    if( s_Channel == -1)
    {
        rDebugPrintf( "Warning: Failed to initialize Host I/O commumication with channel %d. Check connection.\n", channel);
    }

    //
    // Lets intialize the system. 
    //
    if( !HIOInit( s_Channel, NULL ) )
    {
        s_Channel = -1;     
        rDebugString( "Warning: Failed to initialized Host I/O commumication. Check connection.\n");
        return;
    }

    //
    // Not actively communicating.
    //
    m_ComState = Init;

    //
    // Set up pointer to access shared HIO memory.
    //
    m_pHioMemory = (HioMemoryMap*) 0;

    m_TimeOfLastContact = radTimeGetMilliseconds( );
    m_TimeOfLastService = radTimeGetMilliseconds();

}

//=============================================================================
// Function:    CTargetHIOSocket::~CTargetHIOSocket( void )
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

CTargetHIOSocket::~CTargetHIOSocket( void )
{
    //
    // Nothing
    //
}

//=============================================================================
// Function:    CTargetHIOSocket::HIOEnumerationCallback
//=============================================================================
// Description: Static function invoked by the HIO system to report Usb
//              devices.
//
// Parameters:  channel
//              
// Returns:     FALSE - we always use the first device.
//
// Notes:       
//------------------------------------------------------------------------------

BOOL CTargetHIOSocket::HIOEnumerationCallback( s32 channel )
{
    //
    // Save the channel.
    //
    s_Channel = channel;

    return( FALSE );
}        

//=============================================================================
// Function:    CTargetHIOSocket::ReadWriteCallback
//=============================================================================
// Description: Static function invoked by the HIO system an asych read or write
//              completes. Simply sets a completion flag.
//
// Parameters:  n/a
//              
// Returns:     n/a
//
// Notes:       
//------------------------------------------------------------------------------

void CTargetHIOSocket::ReadWriteCallback( void )
{
    //
    // Assert that things are in the correct state.
    //
    rAssert( !s_AsyncReadWriteComplete );

    s_AsyncReadWriteComplete = true;
}        

//=============================================================================
// Function:    CTargetHIOSocket::RoundUpTo32
//=============================================================================
// Description: Member to round number up to multiple of 32
//
// Parameters:  unsinged int
//              
// Returns:     converted usinged int.
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CTargetHIOSocket::RoundUpTo32
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
// Function:    CTargetHIOSocket::socket
//=============================================================================
// Description  Create a socket.
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------
    
int CTargetHIOSocket::socket
( 
    int     af, 
    int     type,
    int     protocol
)
{
    //
    // Nothing interesting happens when a socket is created. Just return
    // a socket id that is not one of out valid socket ids.
    //
    m_OpenSocketCount++;

    return( HIO_CHANNEL_COUNT + 1);
}
    
//=============================================================================
// Function:    CTargetHIOSocket::setsockopt
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::setsockopt
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
// Function:    CTargetHIOSocket::bind
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::bind
( 
    int     sock,
    struct  sockaddr* addr,
    int     addrlen
)
{
    //
    // Make sure the socket is our listening socket and that we have not already
    // openned our file used for communicaition.
    //
    rAssert( sock == HIO_CHANNEL_COUNT + 1 );
    
    return( 0 );    
};

//=============================================================================
// Function:    CTargetHIOSocket::listen
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::listen( int sock, int backlog)
{
    //
    // Make sure the socket is our listen socket and we are not already listening.
    //
    rAssert( sock == HIO_CHANNEL_COUNT + 1 );
    rAssert( m_ListenSocketIndex == HIO_CHANNEL_COUNT + 1);      
    
    //
    // Find a free socket and set state to listening.
    //
    for( unsigned int i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
    {
        //
        // Find a free socket.
        //
        if( m_SocketArray[ i ].m_State ==  Free )
        {
            //
            // We have found a free one. Set state to listening and 
            //
            m_SocketArray[ i ].m_State = Listen;
            m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
            m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
            m_ListenSocketIndex = i;
            return( 0 );

        }
    }
    return( -1 );
}

//=============================================================================
// Function:    CTargetHIOSocket::accept
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::accept( int sock, struct sockaddr* addr, int* addrlen)
{
    //
    // This routine is polled by the higher levels locking for a connection.
    // Lets use it to drive the host communicaiton.
    //
    DoHIOCommunication( );

    rAssert( sock == HIO_CHANNEL_COUNT + 1);
    rAssert( m_ListenSocketIndex != HIO_CHANNEL_COUNT + 1 );      
    
    //
    // Check if connected or still listening.
    //
    if( m_SocketArray[ m_ListenSocketIndex ].m_State == Listen )
    {
        return( INVALID_SOCKET );
    }
    else if( m_SocketArray[ m_ListenSocketIndex ].m_State == Connected )
    {
        //
        // Here we have acheived a connection. Return socket adjusted by one.
        //
        int connectedSocket = m_ListenSocketIndex + 1;
        m_ListenSocketIndex = HIO_CHANNEL_COUNT + 1;
        
        //
        // Listen on another socket.
        //
        CTargetHIOSocket::listen( sock, 0 );

        m_OpenSocketCount++;

        return( connectedSocket );
    }

    rAssert( false );
       
    return( INVALID_SOCKET );
}    

//=============================================================================
// Function:    CTargetHIOSocket::connect
//=============================================================================
// Description: Connect not allowed on target
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::connect( int sock, const struct sockaddr* addr, int addrlen )
{
    return( -1 );
}

//=============================================================================
// Function:    CTargetHIOSocket::closesocket
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::closesocket( int sock )
{
    m_OpenSocketCount--;

    //
    // Check if it is a socket we are listening with.
    //
    if( sock != HIO_CHANNEL_COUNT + 1)
    {
        //  
        // Adjust to compensate for value returned at accept.
        //
        sock--;

        //
        // Check if connected. If not don't worry.
        //
        if( m_SocketArray[ sock ].m_State == Connected )
        {
            //
            // Check it we have data to send. If so set pending disconnect.
            // Else set disconnect.
            //
            if( m_SocketArray[ sock ].m_BytesInTxBuffer == 0 )
            {
                m_SocketArray[ sock ].m_State = Disconnected;
            }
            else
            {
                m_SocketArray[ sock ].m_State = PendingDisconnect;
            }
        }
        else
        {
            m_SocketArray[ sock ].m_State = Free;
        }                   
    }
    else
    {
        //
        // Here we have the listen socket. Lets change state to free.
        //
        m_SocketArray[ m_ListenSocketIndex ].m_State = Free;
        m_ListenSocketIndex = HIO_CHANNEL_COUNT;
    }


    //
    // Check if we should shutdown. When the open socket count goes to zero,
    // this is our indication we are done.
    //
    if( m_OpenSocketCount == 0 )
    {
        //
        // Lets wait a few moments spinning to ensure the host sees the 
        // final state.
        //
        unsigned int startTime = radTimeGetMilliseconds( );
        while( radTimeGetMilliseconds( ) - startTime < 5000 )
        {
            DoHIOCommunication( );
        }
    }

    return( 0 );
}

//=============================================================================
// Function:    CTargetHIOSocket::lasterror
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::lasterror( int sock )
{
    //  
    // Adjust to compensate for value returned at accept.
    //
    sock--;

    //
    // This routine gets the last error. It assumes that only called when 
    // a send or receive fails. Return would block if connected.
    //
    if( m_SocketArray[ sock ].m_State == Connected )
    {
        return( WSAEWOULDBLOCK );
    }
    else
    {
        return( WSAENOTCONN );
    }
}

//=============================================================================
// Function:    CTargetHIOSocket::recv
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::recv( int sock, char* buf, int len, int flags)
{
    //  
    // Adjust to compensate for value returned at accept.
    //
    sock--;

    //
    // This routine is polled. Use it to drive HIO communication
    //
    DoHIOCommunication( );

    //
    // Make sure connecteded. If not return -1;
    //
    if( m_SocketArray[ sock ].m_State != Connected )
    {
        return( -1 );
    }
    
    //
    // See if any data. If so return as much as the caller asked for.
    //
    if( m_SocketArray[ sock ].m_BytesInRxBuffer == 0 )
    {
        return( 0 );
    }

    if( len >= (int) m_SocketArray[ sock ].m_BytesInRxBuffer )
    {
        len = m_SocketArray[ sock ].m_BytesInRxBuffer;
        memcpy( buf, m_SocketArray[ sock ].m_RxBuffer, len );
        m_SocketArray[ sock ].m_BytesInRxBuffer = 0;
    }
    else
    {
        m_SocketArray[ sock ].m_BytesInRxBuffer -= len;
        memcpy( buf, m_SocketArray[ sock ].m_RxBuffer, len );
        
        //
        // This is lazy, but will revisit if performance is bad.
        //
        memmove( m_SocketArray[ sock ].m_RxBuffer, &m_SocketArray[ sock ].m_RxBuffer[ len ], m_SocketArray[ sock ].m_BytesInRxBuffer );
    }
	return( len );
}

//=============================================================================
// Function:    CTargetHIOSocket::send
//=============================================================================
// Description:
//
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

int CTargetHIOSocket::send( int sock, const char* buf, int len, int flags)
{
    //  
    // Adjust to compensate for value returned at accept.
    //
    sock--;

    //
    // This routine is polled. Use it to drive host communication.
    //
    DoHIOCommunication( );

    //
    // Make sure connecteded. If not return -1;
    //
    if( m_SocketArray[ sock ].m_State != Connected )
    {
        return( -1 );
    }
    
    //
    // See if any data. If so return as much as the caller asked for.
    //
    if( m_SocketArray[ sock ].m_BytesInTxBuffer != 0 )
    {
        return( 0 );
    }
 
    if( len > HIO_CHANNEL_SIZE )
    {
        len = HIO_CHANNEL_SIZE;
    }

    memcpy( m_SocketArray[ sock ].m_TxBuffer, buf, len );
    m_SocketArray[ sock ].m_BytesInTxBuffer = len;
    return( len );
}

//=============================================================================
// Function:    CTargetHIOSocket::DoHIOCommunication 
//=============================================================================
// Description: This routine is invoked perfiodically to drive the communication
//              with the host. 
//              
// Parameters:  
//              
// Returns:     
//
// Notes:       
//------------------------------------------------------------------------------

void CTargetHIOSocket::DoHIOCommunication( void )
{
  	// Check if HIOInit() was successful.  If not, just return.
	if(s_Channel == -1)
	{
		return;
	}

	if( radTimeGetMilliseconds() - m_TimeOfLastService > DETECT_DEBUG_TIMEOUT)
	{
		// it is assumed that a breakpoint has been set if DoHIOCommunication didn't get call within 1.5 sec
		// therefore, set last contact to be the present time to avoid timeout.
		m_TimeOfLastContact = radTimeGetMilliseconds();
	}

	m_TimeOfLastService = radTimeGetMilliseconds();

	unsigned int i;

    //
    // Switch on our communication state. We read/write data asynchronously so we must run
    // this as a state machine.
    //
    switch( m_ComState )
    {
        case Init :
        {
            s_AsyncReadWriteComplete = false;

            for( i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
            {
                m_ChannelControlBlock.m_ChannelAllocationMap[ i ] = 0;
                m_ChannelControlBlock.m_HostTransmitCount[ i ] = 0;
                m_ChannelControlBlock.m_TargetTransmitCount[ i ] = 0;
                m_ChannelControlBlock.m_MessageMap[ i ] = HioChannelControlBlock::NullMessage;
            }

            for( i = 0 ; i < HIO_BUFFER_COUNT ; i++ )
            {
                m_ChannelControlBlock.m_HostTransitBufferOwner[ i ] = 0xffffffff;
                m_ChannelControlBlock.m_TargetTransitBufferOwner[ i ] = 0xffffffff;
            }


            //            
            // Set it to nobody so that if the Host has started first, the Host will time out and it will reinitialize
            // the Hio Memory block.
            //
            m_ChannelControlBlock.m_TargetOwnsMemory = HIO_TARGETOWNSMEMORY + HIO_HOSTOWNSMEMORY;
	        
            DCFlushRange(  &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ) );

            BOOL result = HIOWriteAsync( (u32) &m_pHioMemory->m_ChannelControlBlock,
                                &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ),
                                ReadWriteCallback );
            rAssert( result );
            m_ComState = Idle;
            break;
        }

        case Idle :
        {
            if( !s_AsyncReadWriteComplete )
            {
                return;
            }

            //
            // Here we need to read the control block to see if the shared memory has
            // be transfered to us.
            //
            s_AsyncReadWriteComplete = false;

        	DCInvalidateRange( &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ) );
            
            BOOL result = HIOReadAsync( (u32) &m_pHioMemory->m_ChannelControlBlock,
                                        &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ),
                                        ReadWriteCallback );    
            rAssert( result );

            m_ComState = ReadingControlBlock;

            break;
        }

        case ReadingControlBlock :
        {
            //
            // Here we are waiting for the control block async read to complete. If not done,
            // just return.
            //
            if( !s_AsyncReadWriteComplete )
            {
                return;
            }

            //
            // Here we have read the control block. Check if ownership of memory has been
            // transfered to us.
            //
            if( m_ChannelControlBlock.m_TargetOwnsMemory != HIO_TARGETOWNSMEMORY )
            {
                //
                // Here the host memory if not ours. Check the time of last contact.
                //
                m_ComState = Idle;

                if( radTimeGetMilliseconds( ) - m_TimeOfLastContact > HOST_COMMUNICATION_TIMEOUT )
                {
					//
                    // Here the host has vanished or is not present. Set the state of
                    // all our non-listening sockets to remotely disconnected.
                    //
                    for( i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
                    {
                        if( (m_SocketArray[ i ].m_State != Listen) && (m_SocketArray[ i ].m_State != Free) )
                        {
                            m_SocketArray[ i ].m_BytesInRxBuffer = 0;      
                            m_SocketArray[ i ].m_BytesInTxBuffer = 0;   
                            if( m_SocketArray[ i ].m_State == Connected )
                            {
                                m_SocketArray[ i ].m_State = RemoteDisconnect;
                            }
                            else
                            {
                                m_SocketArray[ i ].m_State = Free;
                            }
                        }
                    }
                }
                return;
            }
    
            //
            // Here the ownership of memory has been granted to us. Re-read the control
            // block to make sure we don't experience race conditions/non-atomic issues.
            //
            // Invalidate this area of memory prior to performing the read.
            //
            s_AsyncReadWriteComplete = false;

        	DCInvalidateRange( &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ) );
            
            BOOL result = HIOReadAsync( (u32) &m_pHioMemory->m_ChannelControlBlock,
                                        &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ),
                                        ReadWriteCallback );    
            rAssert( result );

            m_ComState = ReReadingControlBlock;

            break;
        }

        case ReReadingControlBlock :
        {
            //
            // Here we are waiting for the control block async read to complete. If not done,
            // just return.
            //
            if( !s_AsyncReadWriteComplete )
            {
                return;
            }

            //
            // Here we have the control block. First lets see if we have any data to read.
            //
            m_ReadWriteIndex = 0;

            if( ReadData( ) )
            {
                //
                // A read was initiated. Just return.
                //
                return;
            }

            m_CurrentSendIndex = m_LastSendIndex;

            //
            // Here there was not data to read. Try to see if we can send data.
            //
            if( WriteData( ) )
            {
                //
                // We had data to send. Just return.
                //
                return;
            }

            //
            // If we get here we cound not read or there was nothing to write. Lets resolve
            // state.
            //
            ResolveState( );

            break;
        }
    
        case ReadingData :
        {
            //
            // Here we have initiate the read of some data. Check if complete. If not just return.
            //
            if( !s_AsyncReadWriteComplete )
            {
                return;
            }
            
            //
            // Update our local info with the amount received and clear the control block.
            //
            unsigned int channelIndex = radPlatformEndian32( m_ChannelControlBlock.m_HostTransitBufferOwner[ m_ReadWriteIndex ] );
            m_ChannelControlBlock.m_HostTransitBufferOwner[ m_ReadWriteIndex ] = 0xffffffff;

            //
            // Get the local socket index. 
            //
            unsigned int socketIndex = GetSocketIndex( channelIndex );

            m_SocketArray[ socketIndex ].m_BytesInRxBuffer =
            radPlatformEndian32( m_ChannelControlBlock.m_HostTransmitCount[ channelIndex ] );
            m_ChannelControlBlock.m_HostTransmitCount[ channelIndex ] = 0;

            m_ReadWriteIndex++;

            //
            // Now see if we have more data to read.
            //
            if( ReadData( ) )
            {
                //
                // A read was initiated. Just return.
                //
                return;
            }

            //
            // Here there was not data to read. Try to see if we can send data.
            //
            m_ReadWriteIndex = 0;
            m_CurrentSendIndex = m_LastSendIndex;

            if( WriteData( ) )
            {
                //
                // We had data to send. Just return.
                //
                return;
            }

            //
            // If we get here we cound not read or there was nothing to write. Lets resolve
            // state.
            //
            ResolveState( );

            break;
        }

        case WritingData :
        {
            //
            // Here we have initiated the write of some data. Check if complete. If not just return.
            //
            if( !s_AsyncReadWriteComplete )
            {
                return;
            }

            //
            // Update the control block and zero our local transmit count.
            //
            m_ChannelControlBlock.m_TargetTransmitCount[ m_SocketArray[ m_CurrentSendIndex ].m_HioChannel ] = 
                            radPlatformEndian32( m_SocketArray[ m_CurrentSendIndex ].m_BytesInTxBuffer );
            m_SocketArray[ m_CurrentSendIndex ].m_BytesInTxBuffer = 0;
       
            m_CurrentSendIndex++;
            if( m_CurrentSendIndex == HIO_CHANNEL_COUNT )
            {
                 m_CurrentSendIndex = 0;
            }
            
            //
            // See if we have more data to send.
            //
            if( WriteData( ) )
            {
                //
                // We had data to send. Just return.
                //
                return;
            }

            ResolveState( );

            break;
        }

        case WritingControlBlock :
        {
            //
            // Wait for control block to be written. 
            //
            if( !s_AsyncReadWriteComplete )
            {
                return;
            }

            m_TimeOfLastContact = radTimeGetMilliseconds( );
            m_ComState = Idle;

            break;
        }
    }        
}

//=============================================================================
// Function:    CTargetHIOSocket::ReadData
//=============================================================================
// Description: This routine is invoked to initiate the receipt to data in
//              the from shared memory. 
//              
// Parameters:  n/a
//              
// Returns:     returns true is receice outstanding.
//                      false if done     
//
//------------------------------------------------------------------------------
     
bool CTargetHIOSocket::ReadData( void )
{
    //
    // Iterate through channels, seeing if connected and have room to take received data
    // and that there is data.
    //
    for( unsigned int i = m_ReadWriteIndex ; i < HIO_BUFFER_COUNT ; i++ )
    {
        if( m_ChannelControlBlock.m_HostTransitBufferOwner[ i ] != 0xffffffff )
        {
            //
            // Here there is some data in a target transit buffer. Get the channel index.
            //
            unsigned int channelIndex = radPlatformEndian32( m_ChannelControlBlock.m_HostTransitBufferOwner[ i ] );

            //
            // Get the local socket index. 
            //
            unsigned int socketIndex = GetSocketIndex( channelIndex );

            if( (socketIndex != HIO_CHANNEL_COUNT) && m_SocketArray[ socketIndex ].m_State == Connected )
            {
                unsigned int bytesInReceiver = radPlatformEndian32( m_ChannelControlBlock.m_HostTransmitCount[ channelIndex ] );
                   
                if( (bytesInReceiver != 0) && (m_SocketArray[ socketIndex ].m_BytesInRxBuffer == 0) )  
                {
					//
                    // Here we have some data to read and a place to put it.
                    // Initiate the read. We don't update state until the read completes. Make sure
                    // read is a multiple of 32, always rounding up as necessary.
                    //
                    m_ReadWriteIndex = i;
                    m_ComState = ReadingData;
                    s_AsyncReadWriteComplete = false;
                    //
                    // Invalidate the memory prior to issueing read 
                    //
        	        DCInvalidateRange( m_SocketArray[ socketIndex ].m_RxBuffer, RoundUpTo32( bytesInReceiver ) );

                    BOOL result = HIOReadAsync( (u32) &m_pHioMemory->m_HostTransmitBuffer[i][0],
                                    m_SocketArray[ socketIndex ].m_RxBuffer, RoundUpTo32( bytesInReceiver ),
                                    ReadWriteCallback );    
                
                    rAssert( result );
                     
                    return( true );
                }
            }
            else
            {
                m_ChannelControlBlock.m_HostTransitBufferOwner[ i ] = 0xffffffff;
            }         
        }
    }

    //
    // We are done. Return false.
    //
    return( false );
}    

//=============================================================================
// Function:    CTargetHIOSocket::WriteData
//=============================================================================
// Description: This routine is invoked to initiate the transmit of data into
//              shared memory. 
//              
// Parameters:  n/a
//              
// Returns:     returns true is write outstanding.
//                      false if done     
//
//------------------------------------------------------------------------------

bool CTargetHIOSocket::WriteData( void )
{
    for( unsigned int i = m_ReadWriteIndex ; i < HIO_CHANNEL_COUNT ; i++ )
    {
        if( (m_SocketArray[ m_CurrentSendIndex ].m_State == Connected) || (m_SocketArray[ m_CurrentSendIndex ].m_State == PendingDisconnect) )
        {
            unsigned int bytesInTransmitter = radPlatformEndian32( m_ChannelControlBlock.m_TargetTransmitCount[ m_SocketArray[ m_CurrentSendIndex ].m_HioChannel ] );
                   
            if( (bytesInTransmitter == 0) && (m_SocketArray[ m_CurrentSendIndex ].m_BytesInTxBuffer != 0) )
            {
                //
                // Here we have some data to write and a place to put it. Lets see if one of the shared buffers is
                // available.
                //
                unsigned int bufferIndex = GetBufferIndex( &m_ChannelControlBlock );
                if( bufferIndex != 0xffffffff )
                {
                    //
                    // Here we have some data to write and a place to put it.
                    // Initiate the write. 
                    //
                    m_ReadWriteIndex = i;
                    m_ComState = WritingData;
                    s_AsyncReadWriteComplete = false;

                    m_ChannelControlBlock.m_TargetTransitBufferOwner[ bufferIndex ] = radPlatformEndian32( m_SocketArray[ m_CurrentSendIndex ].m_HioChannel );
                    m_LastSendIndex = m_CurrentSendIndex;   

                    //
                    // Flush cache before writing the data.
                    //
                    DCFlushRange( m_SocketArray[ m_CurrentSendIndex ].m_TxBuffer, RoundUpTo32( m_SocketArray[ m_CurrentSendIndex ].m_BytesInTxBuffer ) );

                    BOOL result = HIOWriteAsync( (u32) &m_pHioMemory->m_TargetTransmitBuffer[ bufferIndex][ 0 ],
                                    m_SocketArray[ m_CurrentSendIndex ].m_TxBuffer, RoundUpTo32( m_SocketArray[ m_CurrentSendIndex ].m_BytesInTxBuffer ),
                                    ReadWriteCallback );    
                
                    rAssert( result );
                     
                    return( true );
                }
                else
                {
                    return( false );
                }
            }
        }

	    m_CurrentSendIndex++;
        if( m_CurrentSendIndex == HIO_CHANNEL_COUNT )
        {
		    m_CurrentSendIndex = 0;
        }
	}
    return( false );
}

//=============================================================================
// Function:    CTargetHIOSocket::ResolveState
//=============================================================================
// Description: This routine is used to resolve the state based on mesages
//              and messages we send.
//              
// Parameters:  n/a
//
//------------------------------------------------------------------------------

void CTargetHIOSocket::ResolveState( void )
{
    //
    // First interate over our local channels. We do not initaite connections,
    // so we our local state can only be free, listen, connected, pending disconnect, and
    // disconnect. Lets first deal with established connections.
    //
    for( unsigned int i = 0; i < HIO_CHANNEL_COUNT ; i++ )
    {
        if( (m_SocketArray[ i ].m_State == Connected) || 
            (m_SocketArray[ i ].m_State == PendingDisconnect) ||
            (m_SocketArray[ i ].m_State == Disconnected) )
        {
            //
            // Here we have a connection or are about to shut one down. We have 
            // a valid index into the message table. The only valid messages from the
            // host are disconnect or null message.
            //
            unsigned int channel = m_SocketArray[ i ].m_HioChannel;
            HioChannelControlBlock::ControlMessage message = 
            (HioChannelControlBlock::ControlMessage) radPlatformEndian32( m_ChannelControlBlock.m_MessageMap[ channel ] );
            rAssert( (message == HioChannelControlBlock::NullMessage) || (message == HioChannelControlBlock::DisconnectMessage ) );
 
            //
            // Lets let the host disconnect message take priority. Set our state to disconnected.
            //
            if( message == HioChannelControlBlock::DisconnectMessage )
            {
                //
                // Clear the message entry, free the channel, and set our local state. 
                // 
                m_ChannelControlBlock.m_MessageMap[ channel ] = (HioChannelControlBlock::ControlMessage) radPlatformEndian32( HioChannelControlBlock::NullMessage );
                m_ChannelControlBlock.m_ChannelAllocationMap[ channel ] = 0;
                if( m_SocketArray[ i ].m_State == Connected )
                {
                    //
                    // Set it disconnected. The client will free the entry,
                    //
                    m_SocketArray[ i ].m_State = RemoteDisconnect;
                }
                else
                {
                    m_SocketArray[ i ].m_State = Free; 
                }
            }
            else
            {
                //
                // No message from the host. Check our state. If connected, do nothing. If
                // disconnected, send disconnect message. If Pending disconnect, and tx buffer
                // empty, send disconnect message. Host free channel when sees message
                //
                if( (m_SocketArray[ i ].m_State == Disconnected) ||
                    ( (m_SocketArray[ i ].m_State == PendingDisconnect) && (m_SocketArray[ i ].m_BytesInTxBuffer == 0) ) )
                {
                    m_ChannelControlBlock.m_MessageMap[ channel ] = (HioChannelControlBlock::ControlMessage) radPlatformEndian32( HioChannelControlBlock::DisconnectMessage );
                    m_SocketArray[ i ].m_State = Free; 
                }                                
            }
        }  
    }

    //
    // Here we have dealt with all the established or disconnected sockets. Now lets deal with
    // the listening and connecting sockets. Lets iterate over all allocated shared memory channels.
    //
    for( unsigned int channel = 0 ; channel < HIO_CHANNEL_COUNT ; channel++ )
    {
        if( m_ChannelControlBlock.m_ChannelAllocationMap[ channel ] != 0 )
        {
            if( HioChannelControlBlock::ConnectRequestMessage == (HioChannelControlBlock::ControlMessage) radPlatformEndian32( m_ChannelControlBlock.m_MessageMap[ channel ] ) )
            {
                //
                // Here we have a connect request. Lets check if we have a listen. If so, accept it,
                // Otherwise reject it.
                //
                for( unsigned int i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
                {
                    if( m_SocketArray[ i ].m_State == Listen )
                    {
                        //
                        // Here we have a listen. Set state to connected, and update message.
                        //
                        m_SocketArray[ i ].m_State = Connected;
                        m_SocketArray[ i ].m_HioChannel = channel;
                        m_ChannelControlBlock.m_MessageMap[ channel ] = (HioChannelControlBlock::ControlMessage) radPlatformEndian32( HioChannelControlBlock::ConnectAcceptMessage );
                        break;  
                    }
                }
                    
                //
                // Check the message. If still connect request, did not find listen and reject the connect.
                //
                if( HioChannelControlBlock::ConnectRequestMessage == (HioChannelControlBlock::ControlMessage) radPlatformEndian32( m_ChannelControlBlock.m_MessageMap[ channel ] ) )
                {
                    m_ChannelControlBlock.m_MessageMap[ channel ] = (HioChannelControlBlock::ControlMessage) radPlatformEndian32( HioChannelControlBlock::ConnectRejectMessage );
                }
            }
        }                         
    }

    //
    // We are done. Set ownership flag back to the host.
    //
    m_ChannelControlBlock.m_TargetOwnsMemory = HIO_HOSTOWNSMEMORY;

    m_ComState = WritingControlBlock;
    s_AsyncReadWriteComplete = false;

    //
    // FLush the cache prior to writing.
    //
    DCFlushRange(  &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ) );

    BOOL result = HIOWriteAsync( (u32) &m_pHioMemory->m_ChannelControlBlock,
                                &m_ChannelControlBlock, sizeof( m_ChannelControlBlock ),
                                ReadWriteCallback );    
    rAssert( result );
}

//=============================================================================
// Function:    CTargetHIOSocket::GetSocketIndex
//=============================================================================
// Description: Helper to translate the channel index to a socket index.
//
// Parameters:  unsinged int
//              
// Returns:     converted usinged int.
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CTargetHIOSocket::GetSocketIndex( unsigned int channelIndex )
{
    for( unsigned int i = 0 ; i < HIO_CHANNEL_COUNT ; i++ )
    {
        if( m_SocketArray[ i ].m_HioChannel == channelIndex )
        {
            return( i );
        }
    }

    return( HIO_CHANNEL_COUNT );
}

//=============================================================================
// Function:    CTargetHIOSocket::GetBufferIndex( )
//=============================================================================
// Description: Helper to find a free buffer,
//
// Parameters:  unsinged int
//              
// Returns:     converted usinged int.
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int CTargetHIOSocket::GetBufferIndex( HioChannelControlBlock* pControlBlock )
{
    for( unsigned int i = 0; i < HIO_BUFFER_COUNT ; i++ )
    {
        if( pControlBlock->m_TargetTransitBufferOwner[ i ] == 0xffffffff )
        {
            return( i );
        }
    }
    return( 0xffffffff );
}
