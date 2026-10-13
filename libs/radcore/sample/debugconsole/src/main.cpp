//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:	Foundation Technologies - Debug Console
//
// Description:	This file contains the sample program to test the Foundation
//				Technology debug console.
//
// Date:    	V1.00	July 24, 2000		Creation
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#ifdef RAD_WIN32	// RAD_WIN32 only
#include <windows.h>
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <raddebug.hpp>
#include <raddebugconsole.hpp>
#include <radplatform.hpp>
#include <raddebugcommunication.hpp>
#include <radtime.hpp>

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
// Local Defintions
//=============================================================================

//
// The InputCallback class registers callbacks for character
// keypresses and mouse clicks.
//
class InputCallback
	:
	public IRadDebugConsoleKeyboardInputCallback,
	public IRadDebugConsolePointerInputCallback
{
	public:
		InputCallback( IRadDebugConsole * pConsole );
		~InputCallback( void );
		void OnChar( int asciiKey );
        void OnVKey( int virtualKey, bool ctrl, bool shift, bool alt );
        void OnButtonClick( int xPixels, int yPixels, int xScreenPixels, int yScreenPixels, bool ctrl, bool shift, bool alt );
        void OnButtonDown( int xPixels, int yPixels, int xScreenPixels, int yScreenPixels, bool ctrl, bool shift, bool alt );
        void OnButtonUp( int xPixels, int yPixels, int xScreenPixels, int yScreenPixels, bool ctrl, bool shift, bool alt );
        void OnButtonMove( int xPixels, int yPixels, int xScreenPixels, int yScreenPixels, bool ctrl, bool shift, bool alt, bool bottondown );

		void Terminate( void );
	private:
		int m_KeyCol;
        ref< IRadDebugConsole > m_pConsole;
};


//=============================================================================
// Global Data Declarations
//=============================================================================

//
// Pointers to classes and structures used to direct testing.
//
static IRadDebugConsole*    g_pDebugConsole1 = NULL;
static IRadDebugConsole*    g_pDebugConsole2 = NULL;

//
// This variable is used to terminate our main game loop.
//
static bool g_Done = false;
static bool g_DoPrint = false;


//=============================================================================
// Function:    InputCallback::InputCallback
//=============================================================================
// Description: Constructor for InputCallback
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

InputCallback::InputCallback
(
	IRadDebugConsole * pConsole
)
	:
	m_KeyCol( 0 ),
    m_pConsole( pConsole )
{
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

InputCallback::~InputCallback
(
	void
)
{
}

//=============================================================================
// Function:    InputCallback::OnChar
//=============================================================================
// Description: Action to take when received input from keyboard.
//
// Parameters:  asciiKey received
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void InputCallback::OnChar
(
	int asciiKey
)
{
	rAssert( m_pConsole != NULL );

	char line[ 80 ];
	sprintf( line, "%c", asciiKey );
	m_pConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
	m_pConsole->TextOutAt( line, m_KeyCol, 24 );
	++m_KeyCol;
	if( m_KeyCol >= 80 )
	{
		m_KeyCol = 0;
	}

	if( asciiKey == 'Z' )
	{
		Terminate();
	}
    
    if ( asciiKey == 'X' )
    {
        int nMax = ::rand( ) % 300 + 1000;
        for( int i = 0; i < nMax; i++ )
        {
            m_pConsole->TextOut( "Testing 123 blah..." );
            radDbgComService( );
            radDebugConsoleService( );
        }
    }
}

//=============================================================================
// Function:    InputCallback::OnChar
//=============================================================================
// Description: Action to take when received input from keyboard.
//
// Parameters:  virtualKey
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void InputCallback::OnVKey( int virtualKey, bool ctrl, bool shift, bool alt )
{
	rAssert( m_pConsole != NULL );

    switch ( virtualKey )
    {
        case VK_LEFT :
        {
            // do stuff with left key pressed
            rDebugString( "Left key pressed\n" );
        }
        break;

        case VK_RIGHT:
        {
            // do stuff with right key pressed
            rDebugString( "Right key pressed\n" );
        }
        break;
    };
}


//=============================================================================
// Function:    InputCallback::OnClick
//=============================================================================
// Description: 
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void InputCallback::OnButtonClick( int xPixels, int yPixels, int xScreenPixels, int yScreenPixels, bool ctrl, bool shift, bool alt )
{
	rAssert( m_pConsole != NULL );

	char line[ 80 ];
	sprintf( line, "Click:tx=%d,ty=%d,sx=%d,sy=%d\n", xPixels, yPixels, xScreenPixels, yScreenPixels );
	m_pConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
	m_pConsole->TextOutAt( line, xPixels, yPixels );

	if( (xPixels == 0) && (yPixels == 0) )
	{
		Terminate();
	}
}

void InputCallback::OnButtonDown( int xPixels, int yPixels, int xScreenPixels, int yScreenPixels, bool ctrl, bool shift, bool alt )
{
	rAssert( m_pConsole != NULL );

	char line[ 80 ];
	sprintf( line, "LBDn:tx=%d,ty=%d,sx=%d,sy=%d\n", xPixels, yPixels, xScreenPixels, yScreenPixels );
	m_pConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
	m_pConsole->TextOutAt( line, xPixels, yPixels );

	if( (xPixels == 0) && (yPixels == 0) )
	{
		Terminate();
	}
}

void InputCallback::OnButtonUp( int xPixels, int yPixels, int xScreenPixels, int yScreenPixels, bool ctrl, bool shift, bool alt )
{
	rAssert( m_pConsole != NULL );

	char line[ 80 ];
	sprintf( line, "LBUp:tx=%d,ty=%d,sx=%d,sy=%d\n", xPixels, yPixels, xScreenPixels, yScreenPixels );
	m_pConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
	m_pConsole->TextOutAt( line, xPixels, yPixels );

	if( (xPixels == 0) && (yPixels == 0) )
	{
		Terminate();
	}
}

void InputCallback::OnButtonMove( int xPixels, int yPixels, int xScreenPixels, int yScreenPixels, bool ctrl, bool shift, bool alt, bool bottondown )
{
	rAssert( m_pConsole != NULL );

	char line[ 80 ];
	sprintf( line, "Move:tx=%d,ty=%d,sx=%d,sy=%d\n", xPixels, yPixels, xScreenPixels, yScreenPixels );
	m_pConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
	m_pConsole->TextOutAt( line, 0, 4 );
}

//=============================================================================
// Function:    InputCallback::Terminate
//=============================================================================
// Description: Removes all callbacks and stops the dispatcher.
//
// Parameters:  n/a
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------

void InputCallback::Terminate( void )
{
	g_Done = true;
}


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    main
//=============================================================================
// Description: main program to dispatch using command line parameters.
//
// Parameters:  argc		number of string parameters on command line.
//				argv		pointer to array of strings from command line.
//
// Returns:     0 means no errors
//				!0 means errors
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_WIN32	// RAD_WIN32 only
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#endif

#if defined( RAD_PS2) || defined( RAD_XBOX )
int main( int argc, char* argv[ ] )
#endif

#ifdef RAD_GAMECUBE
void main( void )
#endif
{
    //
    // Display banner message.
    //
    rDebugString( "Debug Console Sample Program V1.00\n" );

    radMemoryInitialize( );

	//
	// Initialte Platform stuff.
	//

    #ifdef RAD_WIN32
        //
        // Windows requires the game provide the main window handle and the module
        // instance.
        //
        radPlatformInitialize( NULL, hInstance );
    #endif
    #ifdef RAD_XBOX
        //
        // XBOX initialization requires no parameters
        //
        radPlatformInitialize(  );
    #endif
    #ifdef RAD_PS2
        //
        // On the PS2, provide the root path for IRXs. 
        // 
        radPlatformInitialize( "\\usr\\local\\sce\\iop\\modules\\",IOPMediaHost );
    #endif 

    #ifdef RAD_GAMECUBE
        radPlatformInitialize( );
    #endif

    radTimeInitialize( );

    //
    // Initialize the communication system. Systems like the console, or debug watch system
	// or file system rely on this being initialized by the game.
    //
#if defined (RAD_WIN32) || defined (RAD_XBOX)
	radDbgComTargetInitialize( WinSocket );
#endif

#ifdef RAD_PS2
	radDbgComTargetInitialize( Deci );
#endif

#ifdef RAD_GAMECUBE
  	radDbgComTargetInitialize( HostIO );
#endif
	
	//
	// Get interface pointer to debug console for testing.
	//
	// AP - alloc = RADMEMORY_ALLOC_DEFAULT
	radDebugConsoleCreate( &g_pDebugConsole1, RADMEMORY_ALLOC_DEFAULT, true );
	rAssert( g_pDebugConsole1 != NULL );
	radDebugConsoleCreate( &g_pDebugConsole2, RADMEMORY_ALLOC_DEFAULT, true );
	rAssert( g_pDebugConsole2 != NULL );

	//
	// Invoke members of bIDebugConsole interface.
	//
    g_pDebugConsole1->SetTitle( "My Test Console 1" );
    g_pDebugConsole1->SetBackgroundColor( RGB( 0, 0, 0x188 ) );  
    g_pDebugConsole1->Clear();

    g_pDebugConsole2->SetTitle( "My Test Console 2" );
    g_pDebugConsole2->SetBackgroundColor( RGB( 0, 0x188, 0x188 ) );  
    g_pDebugConsole2->Clear();

    //
	// Set the initial output location to the top-left corner
	// of the console screen and output in white some text.
	//
    g_pDebugConsole1->SetCursorPosition( 0, 0 );
	g_pDebugConsole1->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
    g_pDebugConsole1->TextOutAt( "Click on position 0,0 or press 'Z' key to end", 0, 0 );
    g_pDebugConsole1->TextOutAt( "press 'X' key to print out lots of message", 0, 1 );

    g_pDebugConsole1->SetCursorPosition( 10, 10 );
    g_pDebugConsole1->TextOut( "Testing" );

    g_pDebugConsole2->SetCursorPosition( 0, 0 );
	g_pDebugConsole2->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
    g_pDebugConsole2->TextOutAt( "Click on position 0,0 or press 'Z' key to end", 0, 0 );
    g_pDebugConsole2->TextOutAt( "press 'X' key to print out lots of message", 0, 1 );

    g_pDebugConsole2->SetCursorPosition( 10, 10 );
    g_pDebugConsole2->TextOut( "Testing" );
	//
	// Set the colour to purple and output hello at the location
	// 20, 20.
	//
	g_pDebugConsole1->SetTextColor( RGB( 0xFF, 0, 0xFF ) );
	g_pDebugConsole1->TextOutAt( "hello", 20, 20 );

    g_pDebugConsole1->SetCursorPosition( 10, 10 );
	//
  	// Register the callbacks of the debug console so that we
  	// can do something interesting when the user clicks or
  	// types in text.
  	//
  	InputCallback* pInputCallback1 = new InputCallback( g_pDebugConsole1 );
	g_pDebugConsole1->SetKeyboardInputCallback( pInputCallback1 );
	g_pDebugConsole1->SetPointerInputCallback( pInputCallback1 );

    InputCallback* pInputCallback2 = new InputCallback( g_pDebugConsole2 );
	g_pDebugConsole2->SetKeyboardInputCallback( pInputCallback2 );
	g_pDebugConsole2->SetPointerInputCallback( pInputCallback2 );

    //
	// Now enter our game loop. Make sure we service the console or it will
	// not operate correctly.
	//
    ::srand( ::radTimeGetMilliseconds( ) );


    unsigned int LastTime = ::radTimeGetMilliseconds( );

	while( !g_Done )
	{
        //
        // simulate game loop
        //
        if( ( ::radTimeGetMilliseconds( ) - LastTime ) > 33 )
        {
            radDebugConsoleService( );
	        radDbgComService( );
#ifdef RAD_WIN32
            Sleep( 3 );     // not take entire CPU cycle on PC
#endif
        }
        else
        {
#ifdef RAD_WIN32
            Sleep( 3 );     // not take entire CPU cycle on PC
#endif
        }
	}		

	//
	// Output a message once the dispatcher stops.
	//
	rDebugString( "Stopped");

	//
	// Clean up references to the objects still left in memory
	// that we don't need anymore. 
	//
 	delete pInputCallback1;
	g_pDebugConsole1->Release( );
	g_pDebugConsole1 = NULL;

 	delete pInputCallback2;
    g_pDebugConsole2->Release( );
	g_pDebugConsole2 = NULL;

	radDbgComTargetTerminate( );
    radTimeInitialize( );
	radPlatformTerminate( );
    radMemoryTerminate( );

    #ifdef RAD_DEBUG
        radObject::DumpObjects( );
    #endif

    #ifndef RAD_GAMECUBE
	    return( 0 );
    #endif
}

 