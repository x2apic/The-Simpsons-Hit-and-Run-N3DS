//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        targetdecichannel.Hpp
//
// Subsystem:	Radical Debug Communication
//
// Description:	This file contains all definitions and classes relevant to
//              the debug communication DECI target Channel.
//
// Revisions:	08-Nov-2000 Creation
//
// Notes:       This component is only available under PS2
//
//=============================================================================

#ifndef	TARGETDECICHANNEL_HPP
#define TARGETDECICHANNEL_HPP
#ifdef RAD_PS2

//=============================================================================
// Include Files
//=============================================================================

#include <raddebugcommunication.hpp>              
#include <radtime.hpp>
#include <raddispatch.hpp>
#include "targetx.hpp"
#include "decitypes.h"
#include "deci2x.h"
#include "protocol.hpp"

//=============================================================================
// Forward Class Declarations
//=============================================================================


//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Class Declarations
//=============================================================================

//
// This class implements the debug target channel interface. This implementation
// is for the PS2 DECI system
//
class rDbgComDECITargetChannel : public IRadDbgComChannel,
                                 public IRadTimerCallback,
                                 public IRadDispatchCallback,
                                 public radObject
{
    public:

    //
    // Contructor. Receives pointer to parent host object.
    //
    rDbgComDECITargetChannel( rDbgComTarget* pTarget, unsigned short protocol );

    //
    // Destruction done through release.
    //
    ~rDbgComDECITargetChannel( void );
  

    //
    // Mmeber functions as a result of the bIDebugChannel interface.
    //
    virtual void RegisterStatusCallback( IRadDbgComChannelStatusCallback* pCallback );
    virtual void Attach( void );
    virtual void Detach( void );
    virtual void GetStatus( ConnectionState* pConnectionState, char* pMessage );
    virtual void SendAsync( void* pBuffer, unsigned int numBytes, IRadDbgComChannelSendCallback* callback );
    virtual void ReceiveAsync( void* pBuffer, unsigned int numBytes, IRadDbgComChannelReceiveCallback* callback );

    //
    // Members implemented for the timer event handler
    //
    virtual void OnTimerDone( unsigned int elapsedTime, void * pUserData );

    //
    // Member implemented for the dispatch event handler
    //
    virtual void OnDispatchCallack( void* userData );

    //
    // Member functions implemented for bIObject
    //
    virtual void AddRef( void );
    virtual void Release( void );

    #ifdef RAD_DEBUG
    virtual void Dump( char* pStringBuffer, unsigned int bufferSize );
    #endif

    private:
    
    //
    // Helper to set the state and invoke state change callback if it exists.
    //
    enum InternalState
    {
        Idle,                           // Not connected, 
        WaitingForConnectResponse,      // Waiting for our connect response
        Connected,                      // OK. Can send and receive
        WaitToSendDisconnectMessage     // Waiting for disconnect to be sent.    
    };

    void SetState( ConnectionState state, InternalState internalState, const char* msg );

    //
    // Static used to field DECI events.
    //
    static void DeciEventHandler( int event, int param, void* Object );

    //
    // These constants are used to signal events from the interrupt handler into 
    // the context of the main loop,
    //
    #define EventTxComplete     0x0001      // Send completed.
    #define EventRxConnectReply 0x0002      // Receive connect reply 
    #define EventRxPacket       0x0004      // Receive client data packet
    #define EventError          0x0008      // Error occurred.

    //
    // This member is used to signal an event occured.
    //
    void SignalEvent( unsigned int event );

    //
    // Member to clear the event mask in interrupt safe manner.
    //
    void ClearMask( unsigned int );
    
    //
    // Helpers to build up packets.
    //
    unsigned int MakeInternalConnectPacket( void );
    unsigned int MakeDataPacket( unsigned int bytesToSend, unsigned char* pData );
    unsigned int MakeInternalDisconnectPacket( void );

    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // Maintain protocol
    //
    unsigned short m_Protocol;

    //
    // Holds a reference to the parent target
    //
    rDbgComTarget* m_ParentTarget;    

    //
    // Maintains our connection state.
    //
    ConnectionState m_ConnectionState;

    //
    // Holds callback to invoke anytime status change occurs.
    //
    IRadDbgComChannelStatusCallback*  m_StatusCallback;

    //  
    // Socket used for communicaiton
    //
    SOCKET  m_Socket;

    //
    // This state is our internal state as apposed to our connection state.
    // 
    InternalState m_InternalState;

    //
    // This buffer is used to textual information to the cleint about the
    // state of things.
    //
    char m_TextInfoBuffer[ 200 ];   

    //
    // These variables are used to receive data.We allocate more space 
    // than required to ensure we keep the buffer alignned to 64 and a multiple
    // of 64.
    //
    unsigned char   m_RxBufferSpace[ sizeof(DECI2_HDR) +  sizeof(rDbgComDataPacket) + 48 + 64];
    unsigned char*  m_RxBuffer;
    unsigned int    m_CurrentBytesToRead;
    unsigned char*  m_CurrentReadAddress;

    //
    // This holds info about the client read async request. Non-zero if receive added.
    //
    IRadDbgComChannelReceiveCallback* m_ClientReceiveCallback;
    unsigned int                    m_ClientReceiveSize;
    unsigned char*                  m_ClientReceiveBuffer;
    
    //
    // This is the buffer we use for sending. It must be large enough for our
    // largest packet which is a data packet. We allocate more space 
    // than required to ensure we keep the buffer alignned to 64 and a multiple
    // of 64.
    //
    unsigned char   m_TxBufferSpace[ sizeof(DECI2_HDR) +  sizeof(rDbgComDataPacket) + 48 + 64];
    unsigned char*  m_TxBuffer;
    unsigned int    m_CurrentBytesToWrite;
    unsigned char*  m_CurrentWriteAddress;
    
    //
    // This holds info about the client send async request. Non-zero if send added.
    //
    IRadDbgComChannelSendCallback*    m_ClientSendCallback;
    unsigned int                    m_ClientSendSize;
    unsigned char*                  m_ClientSendBuffer;

    //
    // This timer is used for check for reply timeouts.
    //
    IRadTimer*            m_Timer;

    //
    // Holds events signal from the isr.
    //
    volatile unsigned int m_EventMask;

    //
    // This variable holds a flag to indicate that we are currently
    // executing under interrupts. We use it to proteect our reference count.
    //
    bool                m_UnderInterrupt;

};

#endif
#endif


