//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Host.cpp
//
// Subsystem:	Foundation Technologies - Debug Com Host Sample Program
//
// Description:	This file contains code to test debug communications.
//				This demo consists on two components: the host application 
//				and the target application. When run the two will echo information
//				back and forth on as many chanels with as many echos as set by the 
//				define statemnts NUM_PACKETS and NUM_CHANNELS is each file (host.cpp
//				and target.cpp) Data sent is based on the packet received by the other
//				side and vice versa.  Data should count the number of transmits done
//				by the host side, for a particular channle (starting from 0).
//
// Date:    	Jan 8th 2002	
//
// Revised by:	Wilfred , Christopher Vienneau
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================
#include <windows.h>
#include <stdio.h>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <raddebugcommunication.hpp>

#define NUM_PACKETS 3
#define NUM_CHANNELS 3
//=============================================================================
// Global Data Declarations
//=============================================================================

//
// Pointers to classes and structures used to direct testing.
//
//
// This variable is used to terminate our main game loop.
//
unsigned short g_Done = 0;

//=============================================================================
// Class Declarations/Implementation
//=============================================================================


//
// This class is used to field status callbacks from the host channel.
// Currently all it does is echo the stuff to the debug console window.
//
class ChannelTest :		public IRadDbgComChannelStatusCallback,
						public IRadDbgComChannelSendCallback,
						public IRadDbgComChannelReceiveCallback
{
    public:
		ChannelTest( radDbgComTargetName name, unsigned short protocol );
		~ChannelTest( void );
		virtual void OnStatusChange( IRadDbgComChannel::ConnectionState connectionState, const char* pMessage );
		virtual void OnSendComplete( bool Successful );
		virtual void OnReceiveComplete( bool Successful, unsigned int bytesReceived );
			
		//
		// Member functions implemented for IRefCound
		//
		virtual void AddRef( void );
		virtual void Release( void ); 

	private:

		char   m_RxBuffer[ 8 ];
		char   m_Terminator1;
		char   m_TxBuffer[ 8 ];
		char   m_Terminator2;


		unsigned int m_RoundTrips;
		unsigned int m_RxCount;
		unsigned int m_RefCount;
		IRadDbgComChannel* m_pChannel;		
		bool m_WaitingRxComplete;
		bool m_WaitingTxComplete;
		unsigned short m_protocol;

		IRadDbgComChannel::ConnectionState m_ConnectionState;

		bool SendIfReady();
};



//=============================================================================
// Function:    ChannelTest::ChannelTest
//=============================================================================
// Description: Constructor for InputCallback
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
ChannelTest::ChannelTest( radDbgComTargetName name, unsigned short protocol )
: m_WaitingRxComplete(false), 
  m_WaitingTxComplete(false)
{
	m_Terminator1 = NULL;
	m_Terminator2 = NULL;
	m_RefCount = 1;
	m_protocol = protocol;
	
	radDbgComHostCreateChannel( name, protocol, &m_pChannel);
	m_pChannel->RegisterStatusCallback( this );
	m_pChannel->Attach();
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
ChannelTest::~ChannelTest( void )
{
	m_pChannel->Detach();
	m_pChannel->Release();
	m_pChannel = NULL;
}

void ChannelTest::AddRef( void )
{
    m_RefCount++;
}

void ChannelTest::Release( void )
{
    m_RefCount--;
    if( m_RefCount == 0 )
    {
		delete this;
    }   
}

//=============================================================================
// Function:    ChannelTest::OnStatusChange
//=============================================================================
// Description: Member invoked when we get a status change.
//
// Parameters:  connectionState, message
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void ChannelTest::OnStatusChange( IRadDbgComChannel::ConnectionState connectionState, const char* pMessage )
{
	char outbuf[ 200 ];
		
	m_ConnectionState = connectionState;
    if( connectionState == IRadDbgComChannel::Detached )
    {
		sprintf( outbuf, "State: [Detached], Message: [%s]\n",pMessage );

		g_Done++;
    }
    else if( connectionState == IRadDbgComChannel::Attaching )
    {
		sprintf( outbuf, "State: [Attaching], Message: [%s]\n",pMessage );
    }   
    else if( connectionState == IRadDbgComChannel::Detaching )
    {
		sprintf( outbuf, "State: [Detaching], Message: [%s]\n",pMessage );
    }   
    else if( connectionState == IRadDbgComChannel::Attached )
    {
		sprintf( outbuf, "State: [Attached], Message: [%s]\n",pMessage );

		//
		// We are attached. Issue a receive.
		//								
        m_pChannel->ReceiveAsync( (void*) m_RxBuffer, sizeof( m_RxBuffer ) , this );
	    m_RoundTrips = 0;
		m_RxCount = 0;
		
		//
		// Issue a send of some data. we are sending data that is the packet number
		// The other side will send this back, then we will send it back again +1 (new packer).
		// and so on till the number of packets set for a particular channel is expent.

		(void) SendIfReady();
    }   

	rDebugString( outbuf );

}

//=============================================================================
// Function:    ChannelTest::OnSendComplete
//=============================================================================
// Description: Member invoked when send completeed or failed.
//
// Parameters:  successful - true/false 
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void ChannelTest::OnSendComplete( bool Successful )
{    
	char tmpBuf[100];
	if( Successful )
    {
		//sprintf(tmpBuf, "protocol %d: Send Completed\n", m_protocol);
		//rDebugString(tmpBuf);
		//rDebugString("Send Completed\n" );
	}
    else
    {
		sprintf(tmpBuf, "protocol %d: Send Failed\n", m_protocol);
		rDebugString(tmpBuf);

    }
	
	m_WaitingTxComplete = false;
	(void) SendIfReady();
}

//=============================================================================
// Function:    ChannelTest::OnReceiveComplete
//=============================================================================
// Description: Member invoked when receive completeed or failed.
//
// Parameters:  successful - true/false 
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void ChannelTest::OnReceiveComplete( bool Successful, unsigned int bytesReceived )
{
	char tmpBuf[100];
	if( Successful )
    {
		//sprintf(tmpBuf, "protocol %d: Receive Completed\n", m_protocol);
		//rDebugString(tmpBuf);

		if(m_ConnectionState == IRadDbgComChannel::Attached)
		{
		}

        m_RxCount += bytesReceived;
        if( m_RxCount == sizeof( m_TxBuffer ) )
        {
			m_RoundTrips++;
			sprintf(tmpBuf, "Protocol %d received Packet   #: %d Containing %s \n",m_protocol, m_RoundTrips -1, m_RxBuffer);
			rDebugString(tmpBuf);


            if( m_RoundTrips == NUM_PACKETS )
            {
				m_pChannel->Detach( );
				return;
			}

			m_RxCount = 0;
			
			m_WaitingRxComplete = false;
            (void) SendIfReady();			
        }
		m_pChannel->ReceiveAsync( (void*) m_RxBuffer, sizeof( m_RxBuffer ) , this );
	}
    else
    {
		sprintf(tmpBuf, "protocol %d: Receive Failed\n", m_protocol);
		rDebugString(tmpBuf);
//        rDebugString("Receive Failed\n" );
    }
}

//=============================================================================
// Function:    ChannelTest::SendIfReady
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
bool ChannelTest::SendIfReady()
{
	char tmpBuf[100];
	bool result = false;		// initialize it to false i.e not ready
	if( (m_WaitingTxComplete == false) && (m_WaitingRxComplete == false) )
	{
		if(m_ConnectionState == IRadDbgComChannel::Attached)
		{
			for(unsigned int i = 0; i < sizeof(m_TxBuffer); i++)
			{
				if (i==0)
				{
					m_TxBuffer[i] = m_RoundTrips + 48;
				}
				else
				{
					m_TxBuffer[i] = 48;
				}
			}
		}
		sprintf(tmpBuf, "\n \n protocol %d: Transmit buffer contains: %s \n", m_protocol, m_TxBuffer);
		rDebugString(tmpBuf);

		m_pChannel->SendAsync( m_TxBuffer, sizeof( m_TxBuffer ), this );
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
    // Create a status callback to monitor for connections status and to drive sends and receives
    //
	unsigned short protocol = 0xee11;

	ChannelTest* ChannelTestArray[NUM_CHANNELS];
	for( unsigned int i = 0; i < NUM_CHANNELS; i++)
	{
		ChannelTestArray[i] = new ChannelTest(DefaultName,protocol++);
	}

	//
	// While not complete, service windows messages.
	//
	while(g_Done < NUM_CHANNELS)
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
	}
		
	//
	// Clean up references to the objects still left in memory
	// that we don't need anymore. 
	//


	for( i = 0; i < NUM_CHANNELS; i++)
	{
		ChannelTestArray[i]->Release();
		ChannelTestArray[i]=NULL;
	}

	//
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
    #ifdef RAD_DEBUG
    radObject::DumpObjects( );    
    #endif

	rDebugString( "Stopped Debug Communication Host Sample Progam V1.00\n" );

	return 0;
}
