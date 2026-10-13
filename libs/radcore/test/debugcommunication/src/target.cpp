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
//              with the host application.  Please check all output on the Host side,
//				as it is more detailed.
//
// Date:    	April 20, 2001
//
// Revised by:	CV
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <radplatform.hpp>
#include <raddebug.hpp>
#include <raddebugcommunication.hpp>


//=============================================================================
// Local Defintions
//=============================================================================

#define NUM_CHANNELS 3
#define NUM_PACKETS_PER_CHANEL 3
#define TOTAL_PACKETS NUM_CHANNELS * NUM_PACKETS_PER_CHANEL

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

unsigned short g_Done = 0;

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
class ChannelTest :    public IRadDbgComChannelStatusCallback,
                       public IRadDbgComChannelSendCallback,
                       public IRadDbgComChannelReceiveCallback
{
    public:
		ChannelTest( unsigned short protocol );
		virtual ~ChannelTest( void );
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

		unsigned int m_TxCount;
		unsigned int m_RxCount;
		unsigned int m_RefCount;
		unsigned short m_protocol;
		
		IRadDbgComChannel* m_pChannel;
		
		bool m_WaitingRxComplete;
		bool m_WaitingTxComplete;

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
ChannelTest::ChannelTest( unsigned short protocol )
: m_WaitingRxComplete(false), 
  m_WaitingTxComplete(false)
{
	m_Terminator1 = NULL;
	m_Terminator2 = NULL;
	m_RefCount = 1;
	m_protocol = protocol;
	// create a channel for this protocol i.e. application
    radDbgComTargetCreateChannel( protocol, &m_pChannel );
    m_pChannel->RegisterStatusCallback( this );
    
	//
	// Attach to the target
	//
    m_pChannel->Attach( );

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
void ChannelTest::AddRef( void )
{
    m_RefCount++;
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
    }
    else if( connectionState == IRadDbgComChannel::Detaching )
    {
        sprintf( outbuf, "State: [Detaching], Message: [%s]\n",pMessage );
    }   
    else if( connectionState == IRadDbgComChannel::Attaching )
    {
        sprintf( outbuf, "State: [Attaching], Message: [%s]\n",pMessage );
    }   
    else if( connectionState == IRadDbgComChannel::Attached )
    {
		sprintf( outbuf, "State: [Attached], Message: [%s]\n",pMessage );
        m_RxCount = 0;
		m_TxCount = 0;
        m_pChannel->ReceiveAsync( m_RxBuffer, sizeof( m_RxBuffer ), this );
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
		m_TxCount++;

		sprintf(tmpBuf, "Protocol %d: Send Success\n", m_protocol);
		rDebugString(tmpBuf);
            
    }
    else
    {
		sprintf(tmpBuf, "Protocol %d: Send Failed\n", m_protocol);
		rDebugString(tmpBuf);
    }

	m_WaitingTxComplete = false;
	(void) SendIfReady();	
}

//=============================================================================
// Function:    ChannelTest::OnSendComplete
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
        m_RxCount += bytesReceived;
        {
			m_WaitingRxComplete = false;
			(void) SendIfReady();
            m_RxCount = 0;
        }
		rDebugString("\n\n");
		rDebugString("Receive Buffer:  ");
		rDebugString(m_RxBuffer);
		rDebugString("\n");
		rDebugString("Transmit Buffer: ");
		rDebugString(m_TxBuffer);
		rDebugString("\n\n");
		g_Done++;


        m_pChannel->ReceiveAsync( (void*) m_RxBuffer, sizeof( m_RxBuffer ), this );
    }
    else
    {
		sprintf(tmpBuf, "Protocol %d: Receive Failed\n", m_protocol);
		rDebugString(tmpBuf);
   
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
	bool result = false;		// initialize it to false i.e not ready
	if( (m_WaitingTxComplete == false) && (m_WaitingRxComplete == false) )
	{
		//FILL Transmit buffer
		if(m_ConnectionState == IRadDbgComChannel::Attached)
		{
			for(unsigned int i = 0; i < sizeof(m_TxBuffer); i++)
			{
				m_TxBuffer[i] = m_RxBuffer[i];

			}
		}

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

	//
    // First initialized the platform. Each platform has specific initialization,
    // PS2 requires path to where IRXs reside and where to load them form. We
    // also intialize the debug communication system. This is required if the
    // Radical File Server will be used.
    //
#ifdef RAD_PS2
    radPlatformInitialize( "\\usr\\local\\sce\\iop\\modules\\", IOPMediaHost );

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
#endif // PS2EE

#ifdef RAD_WIN32
#ifndef RAD_XBOX
    //
    // Windows initialization requires instance and main window. This program does
    // not use a main window, so just set it to null,
    //
	radMemoryInitialize();
    radPlatformInitialize( NULL, hInstance );
    radDbgComTargetInitialize( WinSocket );
#else
    //
    // XBOX does not need any info.
    //
    radPlatformInitialize( );
    radDbgComTargetInitialize( WinSocket );
#endif //WIN32
#endif //XBOX

#ifdef RAD_GAMECUBE
    radPlatformInitialize( );
    radDbgComTargetInitialize( HostIO );
#endif

	unsigned short protocol = 0xee11;
	ChannelTest* ChannelTestArray[NUM_CHANNELS];
	for( unsigned int i = 0; i < NUM_CHANNELS; i++)
	{
		ChannelTestArray[i] = new ChannelTest(protocol++);
	}
	
	while(g_Done < TOTAL_PACKETS)
    {
         radDbgComService();         
    }

	rDebugString("Received expected number of packets, now terminating!!!\n");
	//
	// Clean up references to the objects still left in memory
	// that we don't need anymore. 
	//

	for( unsigned int j = 0; j < NUM_CHANNELS; j++)
	{
		ChannelTestArray[j]->Release();
		ChannelTestArray[j]=NULL;
	}
	
	radDbgComTargetTerminate();
	radPlatformTerminate();
	radMemoryTerminate();

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
