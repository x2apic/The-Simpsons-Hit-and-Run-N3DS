//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Host.cpp
//
// Subsystem:	Foundation Technologies - Debug Com Host Sample Program
//
// Description:	This file contains sample code to illustrate communication
//              with the target application. This demo consists on two
//				components: the host application and the target application.
//				The target application simply waits for a connection, then echos
//				a message back and forth for ten times, then disconnects.
//
// Date:    	April 19, 2001	
//
// Revised by:	AP
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================
#include <windows.h>
#include <stdio.h>
#include <radtime.hpp>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <raddebugcommunication.hpp>

//=============================================================================
// Global Data Declarations
//=============================================================================
class StatusCallback;

//
// Pointers to classes and structures used to direct testing.
//
const unsigned int CHANNEL_COUNT = 5;
static IRadDbgComChannel* g_Channels[ CHANNEL_COUNT ] = { NULL };
static StatusCallback * g_StatusCallback[ CHANNEL_COUNT ] = { NULL };

//
// This variable is used to terminate our main game loop.
//
bool g_Done = false;

//=============================================================================
// Class Declarations/Implementation
//=============================================================================


//
// This class is used to field status callbacks from the host channel.
// Currently all it does is echo the stuff to the debug console window.
//
class StatusCallback : public IRadDbgComChannelStatusCallback,
                       public IRadDbgComChannelSendCallback,
                       public IRadDbgComChannelReceiveCallback
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

    protected:

        //
        // helper function to send ack to the target as we have recieve a
        // completed packet and ready for more.
        //
        void SendAck( );

	private:
		char   m_RxBuffer[ 100 * 1024 ];
		char   m_TxBuffer[ 100 * 1024 ];

		unsigned int m_RoundTrips;
        unsigned int m_RxPacketSize;
		unsigned int m_RxCount;
		unsigned int m_RefCount;
		IRadDbgComChannel* m_pChannel;		
		bool m_WaitingRxComplete;
		bool m_WaitingTxComplete;
        bool m_Attached;
        unsigned int m_PacketCount;

		bool SendIfReady();
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
    m_WaitingRxComplete(false), 
    m_WaitingTxComplete(false),
    m_RxPacketSize( 0 ),
    m_Attached( false ),
    m_PacketCount( 0 )
{
	m_RefCount = 1;
	m_pChannel = g_pChannel;
	m_pChannel->AddRef();
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
	m_pChannel->Release();
}

void StatusCallback::AddRef( void )
{
    m_RefCount++;
}

void StatusCallback::Release( void )
{
    m_RefCount--;
    if( m_RefCount == 0 )
    {
		delete this;
    }   
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
	char outbuf[ 200 ];
		
    if( connectionState == IRadDbgComChannel::Detached )
    {
        //wsprintf( outbuf, "State: [Detached], Message: [%s]\n",pMessage );
		sprintf( outbuf, "State: [Detached], Message: [%s]\n",pMessage );

		g_Done = true;	
        m_Attached = false;
    }
    else if( connectionState == IRadDbgComChannel::Attaching )
    {
        //wsprintf( outbuf, "State: [Attaching], Message: [%s]\n",pMessage );
		sprintf( outbuf, "State: [Attaching], Message: [%s]\n",pMessage );
        m_Attached = false;
    }   
    else if( connectionState == IRadDbgComChannel::Detaching )
    {
        //wsprintf( outbuf, "State: [Detaching], Message: [%s]\n",pMessage );
		sprintf( outbuf, "State: [Detaching], Message: [%s]\n",pMessage );
        m_Attached = false;
    }   
    else if( connectionState == IRadDbgComChannel::Attached )
    {
        m_Attached = true;
        //wsprintf( outbuf, "State: [Attached], Message: [%s]\n",pMessage );
		sprintf( outbuf, "State: [Attached], Message: [%s]\n",pMessage );

		//
		// We are attached. Issue a receive.
		//								
        m_pChannel->ReceiveAsync( (void*) m_RxBuffer, sizeof( m_RxBuffer ) , this );
	    m_RoundTrips = 0;
		m_RxCount = 0;
        m_PacketCount = 0;
		
		//
		// Issue a send of some data. We don't care what the data is, just send it.
		//
		SendIfReady();
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
	//
	// Just print message when send completes.
	//
	if( Successful )
    {
		rDebugString("Send Completed\n" );
	}
    else
    {
        rDebugString("Send Failed\n" );
    }
	
	m_WaitingTxComplete = false;
	(void) SendIfReady();
}

//=============================================================================
// Function:    StatusCallBack::OnReceiveComplete
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
	if( Successful )
    {
        //
        // if m_RxPacketSize is 0, we are ready to recieve a new packet
        //
        if ( m_RxPacketSize == 0 )
        {
            rAssert( bytesReceived >= 4 );
            m_RxPacketSize = *(unsigned int*)( m_RxBuffer );
            rAssert( m_RxPacketSize <= sizeof( m_RxBuffer ) );
            unsigned int PacketNumber = *(unsigned int*)((unsigned int)(m_RxBuffer) + 4 );
            rAssert( m_PacketCount == PacketNumber );
            m_PacketCount++;
			rDebugPrintf( "Receiving [%u] bytes, Count [%u] from \"%s\". Trips = %d\n", m_RxPacketSize, PacketNumber, (char*)((unsigned int)(m_RxBuffer) + 4 + 4 ), m_RoundTrips );
        }

        m_RxCount += bytesReceived;

        if( m_RxCount == m_RxPacketSize )
        {
			m_RoundTrips++;
            if( m_RoundTrips == 10000 )
            {
				m_pChannel->Detach( );
				return;
			}

            //
            // we have recieved a complete packet
            //
            m_RxPacketSize = 0;

            m_RxCount = 0;
			
			m_WaitingRxComplete = false;

            (void) SendIfReady();			
    		rDebugString("Receive Completed\n" );
        }
		m_pChannel->ReceiveAsync( (void*) m_RxBuffer, sizeof( m_RxBuffer ) , this );
	}
    else
    {
        rDebugString("Receive Failed\n" );
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
	bool result = false;		// initialize it to false i.e not ready
	if( (m_Attached == true) && (m_WaitingTxComplete == false) && (m_WaitingRxComplete == false) )
	{
        int size = rand( ) % ( sizeof( m_TxBuffer ) );

        //
        // if ever 0 bytes, just send the entire buffer
        //
        if ( size <= 3 )
        {
            size = sizeof( m_TxBuffer );
        }

        //
        // embeding size of the packet as the first item in the buffer
        //
        int * pHeader = (int*)( m_TxBuffer );
        *pHeader = size;
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
// Parameters:  see windows.
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	//
    // Display banner message.
    //
    rDebugString( "Debug Communication Host Sample Program V1.00\n" );


    ::radTimeInitialize( );
    //
    // randomize
    //
    ::srand( ::radTimeGetMilliseconds( ) );

	//
	// Get the debug communication host target definition table
	//
	IRadDbgComTargetTable* pTargetTable;


	radDbgComHostGetTargetTable( &pTargetTable );
	rAssert( pTargetTable != NULL );

	//
    // Get a default target from the target table.
	// Check to make sure there is one exist.
    //
	radDbgComTargetName	DefaultName;
	if ( !pTargetTable->GetDefaultTarget( DefaultName ) )
 	{
		rAssertMsg( false, "Must use TargetEditor to define a default target");
	}		

    //
    // Get the communication host
    //
    for ( int i = 0; i < CHANNEL_COUNT; i++ )
    {
        radDbgComHostCreateChannel( DefaultName, 0xee10+i, &g_Channels[ i ] );
        rAssert( g_Channels[ i ] != NULL );

        g_StatusCallback[ i ] = new StatusCallback( g_Channels[ i ] );
        g_Channels[ i ]->RegisterStatusCallback( g_StatusCallback[ i ] );
        g_Channels[ i ]->Attach( );
    }

	//
	// While not complete, service windows messages.
	//
	while( !g_Done )
    {
		MSG msg;

        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) 
        {
            TranslateMessage(&msg); 

            //
            // We have a message. Normally, we would check for a quit message. We
            // use our own termination system instead.
            //
            DispatchMessage(&msg); 
        } 
        Sleep( 5 );
	}
		
	//
	// Clean up references to the objects still left in memory
	// that we don't need anymore. 
	//
    for ( int j = 0; i < CHANNEL_COUNT; i++ )
    {
        g_Channels[ i ]->RegisterStatusCallback( NULL );
        g_Channels[ i ]->Detach( );
        g_Channels[ i ]->Release( );
        g_Channels[ i ] = NULL;

        g_StatusCallback[ i ]->Release( );
        g_StatusCallback[ i ] = NULL;
    }


	//
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
    #ifndef NDEBUG
    radObject::DumpObjects( );    
    #endif

	rDebugString( "Stopped Debug Communication Host Sample Progam V1.00\n" );

	return 0;
}
