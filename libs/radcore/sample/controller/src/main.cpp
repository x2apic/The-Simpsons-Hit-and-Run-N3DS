//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:   Radical Controller - Sample 
//
// Description: This file contains code to illustrate the use of the Radical
//              controller system.
//
// Revisions:   V1.00   Apr 10, 2001        Creation
//
// Usage:       This program uses the Foundation Tech debug console to 
//              display contoller output. Make sure the debug console
//              server is run.
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#ifdef RAD_WIN32
#include "window.hpp"
#endif

#ifdef RAD_XBOX
#include <xtl.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <raddebug.hpp>
#include <raddebugconsole.hpp>
#include <raddebugcommunication.hpp>
#include <radcontroller.hpp>
#include <radplatform.hpp>
#include <radtime.hpp>

//=============================================================================
// Public Functions
//=============================================================================

//============================================================================
// Globals
//============================================================================

#if defined RAD_PS2
#define PLATFORM_CONTROLLER_LOCATION "Port0\\Slot0"
#else
#ifndef RAD_XBOX
#define PLATFORM_CONTROLLER_LOCATION "Joystick0"
#else
#define PLATFORM_CONTROLLER_LOCATION "Port0\\Slot0"
#endif
#endif

//=============================================================================
// Local Defintions
//=============================================================================

//
// The InputCallback class registers callbacks for character
// keypresses and controller inputs.
//
class InputCallback
    :
    public IRadDebugConsoleKeyboardInputCallback,
    public IRadControllerConnectionChangeCallback,
    public IRadControllerInputPointCallback
{
    public:

        void OnChar( int asciiKey );
        void OnVKey( int virtualKey, bool ctrl, bool shift, bool alt );
        void OnControllerInputPointChange( unsigned int userData, float newValue );
        void OnControllerConnectionStatusChange( IRadController * pIController );
        void Terminate( void );

        void * operator new ( size_t size );
        void operator delete( void * pMemory );
};

//============================================================================
// Prototypes
//============================================================================

void UpdateScreen( void );


//=============================================================================
// Global Data Declarations
//=============================================================================

//
// Pointers to classes and structures used to direct testing.
//
static IRadDebugConsole*            g_pDebugConsole = NULL;
static IRadControllerOutputPoint*   g_pOutputPoint[ 20 ];
static IRadControllerInputPoint*    g_pInputPoint[ 128 ];
static IRadController*              g_pIController = NULL;

//
// This variable is true if the input points points have been registered
//
static bool g_InputPointsRegistered = false;

//
// This variable is used to terminate our main game loop.
//
bool g_Done = false;

// 
// These variables are used to set the virtual time of the controller
//
unsigned int    g_Latency = 500;
float           g_Min = 0.0f;
float           g_Max = 1.0f;
float           g_Tolerance = 0.00f;
unsigned int    g_BufferTime = 500;
unsigned int    g_CaptureRate = 16;

// 
// This variable is used to "fetch" all of the
// input points associated with the controller.
//
int             g_NumInputPoints = 0;
float           g_InputPointValues[ 50 ];

// 
// These variable is used to "set" all of the 
// outputs associated with the controller
//
int             g_NumOutputPoints = 0;
float           g_OutputPointValues[ 20 ];

//
// This string is used to output information to debug console
//
char            g_Text[ 255 ];


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    main
//=============================================================================
// Description: main program to dispatch using command line parameters.
//
// Parameters:  argc        number of string parameters on command line.
//              argv        pointer to array of strings from command line.
//
// Returns:     0 means no errors
//              !0 means errors
//
// Notes:
//------------------------------------------------------------------------------
#ifdef RAD_WIN32
    int WINAPI WinMain(
      HINSTANCE hInstance,
      HINSTANCE hPrevInstance,
      LPSTR lpCmdLine,
      int nCmdShow)
#elif defined RAD_XBOX
    int _cdecl main (void)
#elif defined RAD_PS2 || defined RAD_GAMECUBE
    int main( int argc, char* argv[ ] )
#endif // RAD_PS2

{
    //
    // Display banner message.
    //
    rDebugString( "Controller Sample Program V1.00\n" );


    //
    // Initialize Platform stuff.
    //
    radMemoryInitialize();

    #if defined RAD_WIN32

        //
        // Windows requires the game provide the main window handle and the module
        // instance.
        //

        HWND mainwnd;
        mainwnd = CreateMainWindow( hInstance, true );
        radPlatformInitialize( mainwnd, hInstance );

    #elif defined RAD_XBOX || defined RAD_GAMECUBE

        //
        // RAD_XBOX initialization requires no parameters
        //
        radPlatformInitialize(  );

    #elif defined RAD_PS2

        //
        // On the PS2, provide the root path for IRXs.
        //
        radPlatformInitialize( "\\usr\\local\\sce\\iop\\modules\\",IOPMediaHost );

    #endif

    ::radTimeInitialize( );

    //
    // Initialize the communication system. Systems like the console, or debug watch system
    // or file system rely on this being initialized by the game.
    //
    #if defined (RAD_WIN32) || defined (RAD_XBOX)
        radDbgComTargetInitialize( WinSocket );
    #elif defined RAD_PS2
        radDbgComTargetInitialize( Deci );
    #elif defined RAD_GAMECUBE
        radDbgComTargetInitialize( HostIO );
    #endif

    // 
    //  Create a debug console 
    //
    radDebugConsoleCreate( &g_pDebugConsole );
    rAssert( g_pDebugConsole != NULL );

    //
    // Invoke members of IDebugConsole interface.
    //
    g_pDebugConsole->SetTitle( "Controller Test" );
    g_pDebugConsole->SetBackgroundColor( RGB( 0, 0, 188 ) );  
    g_pDebugConsole->Clear();

    //
    // Set the initial output location to the top-left corner
    // of the console screen and output in white some text.
    //
    
    #if defined RAD_WIN32
        //
        // RAD_WIN32 message
        //
        sprintf( g_Text, "Set your focus on 'The Hello Program' window to see change to controller input" );
    #else
        sprintf( g_Text, "Press on the controller to see its output" );
    #endif 

    g_pDebugConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
    g_pDebugConsole->TextOutAt( g_Text, 0, 0 );

    //
    // Initialize the controller system.  This is done asynchronously, so
    // controllers may not show up as available immediately.  Also, a callback
    // is given (registeration and unregistration is automattic).  This is so that,
    // when a controller first becomes available, the callback is called immediately.
    // Our callback is used to outpup connection status messages to the debug console.
    //
    InputCallback* pInputCallback = new InputCallback( );
    radControllerInitialize( pInputCallback );

    //
    // Register the callbacks of the debug console so that we
    // can update the debug console when the user types in 
    // text.
    //
    g_pDebugConsole->SetKeyboardInputCallback( pInputCallback );

    //
    // Display virtual time information onto debug console
    //
    g_pDebugConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
    g_pDebugConsole->TextOutAt( "[v]   - Toggle VTime  [l/L] - VLatency     [m/M/x/X] - Min/Max", 0, 7 );
    g_pDebugConsole->TextOutAt( "[t/T] - Tolerance     [b/B] - Buffer Time  [c/C]     - Capture Rate", 0, 8 );
    g_pDebugConsole->TextOutAt( "[q]   - QUIT", 0, 9 );

    //
    // Refresh the screen.
    //
    UpdateScreen( );  

    //
    // Services loop
    //
    while ( !g_Done )
    {
        radDbgComService( );
        radControllerSystemService();
        
        #ifdef RAD_WIN32 // RAD_WIN32 only

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

        #endif
    }
    
    //
    // Unregister all controller input point callbacks if they have been registered
    //

    if ( g_pIController != NULL )
    {
        g_pIController->Release( );
    }

    if( g_InputPointsRegistered )
    {
        for ( int count = 0; count < g_NumInputPoints; count++ )
        {
            g_pInputPoint[ count ]->UnRegisterControllerInputPointCallback( static_cast< IRadControllerInputPointCallback* >( pInputCallback ) );
        }
    }

    //
    // Release reference to g_pDeubgConsole
    //
    g_pDebugConsole->Release( );
    g_pDebugConsole = NULL;
    delete pInputCallback;

    //
    // Terminates all associated objects
    //
    radControllerTerminate( );
    radDbgComTargetTerminate( );
    radTimeTerminate( );
    radPlatformTerminate( );
    radMemoryTerminate( );

    //
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
    #ifdef RAD_DEBUG
    radObject::DumpObjects( );    
    #endif

    rDebugString( "Stopped Controller Sample Progam V1.00\n" );

    return 0;
}

//=============================================================================
// Function: InputCallback::new/delete
//=============================================================================
//
// Must never use global new and delete, not supported on the gamecube.

void * InputCallback::operator new ( size_t size )
{
    return radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, size );
}

void InputCallback::operator delete( void * pMemory )
{
    radMemoryFree( RADMEMORY_ALLOC_DEFAULT, pMemory );
}

//=============================================================================
// Function:    InputCallback::OnControllerConnectionStatusChange
//=============================================================================
// Description: Message output to debug console when a controller 
//              is plug/unplugged.
//
// Parameters:  pIController 
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void InputCallback::OnControllerConnectionStatusChange( IRadController * pIController )
{
    bool controllerConnected = pIController->IsConnected( );

    //
    // Output controller connect state
    //

    if ( g_pIController != NULL )
    {
        g_pIController->Release( );
    }

    g_pIController = pIController;
    g_pIController->AddRef( );

    if ( controllerConnected == false )
    {
        sprintf( g_Text, "Controller is unplugged.  Please plug controller back into: %s", PLATFORM_CONTROLLER_LOCATION);
    }
    else 
    {
        sprintf( g_Text, "Controller is connected" );
    }

    g_pDebugConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
    g_pDebugConsole->TextOutAt("                                                                              ", 0, 5 );
    g_pDebugConsole->TextOutAt( g_Text, 0, 5 );
    rDebugString( g_Text );
    rDebugString( "\n" );

    //
    // Output the number of existing controllers
    //
    unsigned int numController;
    numController = ::radControllerSystemGet( )->GetNumberOfControllers( );
    
    sprintf( g_Text, "Number of available controllers = [%i]", numController );
    g_pDebugConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
    g_pDebugConsole->TextOutAt( g_Text, 0, 2 );
    
    //
    // Lets check to make sure the controller is connected to the above location
    //
    sprintf( g_Text, "Controller is connected to [%s]", pIController->GetLocation( ) );
    g_pDebugConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
    g_pDebugConsole->TextOutAt( g_Text, 0, 3 );
    
    
    //
    // Unregister the old global input points.  If the controller is disconnected,
    // this must be done because there are no input points anymore.  If the
    // being connected, we have to unregister the old global input points so that
    // we can register the new ones.
    //

    int i;
    if( g_InputPointsRegistered )
    {
        //
        // Unregister the callbacks for the input points
        //
        for( i = 0; i < g_NumInputPoints; i++ )
        {
            g_pInputPoint[ i ]->UnRegisterControllerInputPointCallback( static_cast< IRadControllerInputPointCallback* >( this ) );
        }

        //
        // Set the global flag indicating that the input points have been registered
        //
        g_InputPointsRegistered = false;
    }
    
    
    //
    // Now, if a controller is being plugged in, register the new
    // global input and output points
    //
    if( controllerConnected )
    {
        //
        // Get the number of input points available for this controller
        // Display the value onto debug console
        //
        g_NumInputPoints = pIController->GetNumberOfInputPoints( );
        sprintf( g_Text, "Number of input points = [%i]", g_NumInputPoints );
        g_pDebugConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
        g_pDebugConsole->TextOutAt( g_Text, 0, 4 );

        //
        // Get a reference to all input points
        //
        for( i = 0; i < g_NumInputPoints; i++ )
        {
            //
            // If the input point was already registered
            g_pInputPoint[ i ] = pIController->GetInputPointByIndex( i );
            
            //
            // Register the callback of the controller input points so
            // we can get the latest value when the user pressed on 
            // the controller.
            //
            g_pInputPoint[ i ]->RegisterControllerInputPointCallback( static_cast< IRadControllerInputPointCallback* >( this ), (unsigned int)i );
            
        }
        
        //
        // Set the global flag indicating that the input points have been registered
        //
        g_InputPointsRegistered = true;

        //
        // Get all of the output points
        //
        g_NumOutputPoints = pIController->GetNumberOfOutputPoints();
        
        int j;
        for( j = 0; j < g_NumOutputPoints; j++ )
        {
            g_pOutputPoint[ j ] = pIController->GetOutputPointByIndex( j );
        }
    }

    //
    // Get controller type and classification
    // Display the results onto debug console
    //
    g_pDebugConsole->SetTextColor( RGB( 0xFF, 0xFF, 0xFF ) );
    sprintf( g_Text, "Type: [%s] Classification: [%s]", pIController->GetType(), pIController->GetClassification() );
    g_pDebugConsole->TextOutAt( g_Text, 0, 11 );

    //
    // Refresh the screen.
    //
    UpdateScreen( );  
}

//=============================================================================
// Function:    InputCallback::OnVKey
//=============================================================================
// Description: Does nothing
//
// Parameters:  
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void InputCallback::OnVKey( int virtualKey, bool ctrl, bool shift, bool alt )
{
    return;
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
void InputCallback::OnChar( int asciiKey )
{   
    rAssert( g_pDebugConsole != NULL );

    switch ( (char) asciiKey )
    {
        case 'v':
        {
            g_BufferTime = g_BufferTime == 0 ? g_Latency : 0;
            if ( g_BufferTime == 0 )
            {
                g_Latency = 0;
            }

            break;
        }
        case 'l':
        {
            g_Latency -= 10;
            break;
        }
        case 'L':
        {
            g_Latency += 10;
            break;
        }
        case 'm':
        {
            g_Min -= 0.1f;
            break;
        }
        case 'M':
        {
            g_Min += 0.1f;
            break;
        }
        case 'x':
        {
            g_Max -= 0.1f;
            break;
        }
        case 'X':
        {
            g_Max += 0.1f;
            break;
        }
        case 't':
        {
            g_Tolerance -= 0.01f;
            break;
        }
        case 'T':
        {
            g_Tolerance += 0.01f;
            break;
        }
        case 'b':
        {
            g_BufferTime -= 1;
            break;
        }
        case 'B':
        {
            g_BufferTime += 1;
            break;
        }
        case 'c':
        {
            g_CaptureRate--;
            break;
        }
        case 'C':
        {
            g_CaptureRate++;
            break;
        }            
        case 'q':
        case 'Q':
        {
            Terminate();
            break;
        }
        default:
        {
            break;
        }
    }

    if ( g_Latency > 2000 )
    {
        g_Latency = 2000;
    }

    if ( g_BufferTime > 2000 )
    {
        g_BufferTime = 2000;
    }

    if ( g_Tolerance < 0.0f )
    {
        g_Tolerance = 0.0f;
    }

    if ( g_Tolerance > 1.0f )
    {
        g_Tolerance = 10.f;
    }

    if ( g_Latency > g_BufferTime )
    {
        g_BufferTime = g_Latency;
    }

    if ( g_CaptureRate > 1000 )
    {
        g_CaptureRate = 1000;
    }

    if ( g_CaptureRate == 0 )
    {
        g_CaptureRate = 1;
    }

    UpdateScreen( );
}

//=============================================================================
// Function:    InputCallback::OnControllerInputPointChange
//=============================================================================
// Description: Action to take when received input from controller.
//
// Parameters:  userData - input point index value
//              newValue - new input value of this index
//
// Returns:     n/a
//
// Notes:
//------------------------------------------------------------------------------
void InputCallback::OnControllerInputPointChange( unsigned int userData, float newValue )
{
    int point = userData;

    g_InputPointValues[point] = newValue;
    
    if ( g_pInputPoint[point] != NULL )
    {
        sprintf
        (
            g_Text,
            "Ip Name: [%3d][%18s] Type: [%18s] Value: [%.8f]",
            point,
            g_pInputPoint[point]->GetName(),
            g_pInputPoint[point]->GetType(),
            g_InputPointValues[ point ]
        );
                
        g_pDebugConsole->SetTextColor( RGB( 255, 0,  0 ) );
        g_pDebugConsole->TextOutAt("                                                                              ", 0, point + 12 );
        g_pDebugConsole->TextOutAt( g_Text, 0, point + 12 );
    }
    
    //
    // Check to see if there are any output points for this controller
    // 
    if ( g_NumOutputPoints == 0 )
    {
        sprintf( g_Text, "OutputPoint not supported" );
        g_pDebugConsole->TextOutAt("                                                                              ", 0, 12 + g_NumInputPoints );
        g_pDebugConsole->TextOutAt( g_Text, 0, 12 + g_NumInputPoints);
    }
    else 
    {
        int shock_mode = 0;
        float shock_value = 0.0f;
        const char* pLeftStickX = "LeftStickX";
        const char* pRightStickX = "RightStickX";

        if ( strcmp( g_pInputPoint[point]->GetName(), pLeftStickX ) == 0 )
        {
            shock_mode = 0;// small motor, digital
                        
            if ( shock_mode < g_NumOutputPoints )
            {
                shock_value = g_pInputPoint[point]->GetCurrentValue();
                shock_value = (newValue > 0.6f) ? 1.0f : 0.0f;
                g_pOutputPoint[shock_mode]->SetGain( shock_value );
            }
        }
        
        else if ( strcmp( g_pInputPoint[point]->GetName(), pRightStickX ) == 0 )
        {
            shock_mode = 1; // large motor, analog
            
            if ( shock_mode < g_NumOutputPoints )
            {
                shock_value = g_pInputPoint[point]->GetCurrentValue();
                shock_value = (shock_value > 0.6f) ? shock_value : 0.0f;
                g_pOutputPoint[shock_mode]->SetGain( shock_value );
            }
        }

        if ( shock_mode < g_NumOutputPoints )
        {
            sprintf
            (
                    g_Text,
                    "Op Name: [%18s] Type: [%18s] Value: [%.8f]",
                    g_pOutputPoint[shock_mode]->GetName(),
                    g_pOutputPoint[shock_mode]->GetType(),
                    g_pOutputPoint[shock_mode]->GetGain()
            
            );
            
            g_pDebugConsole->SetTextColor( RGB( 255, 0,  0 ) );
            g_pDebugConsole->TextOutAt("                                                                              ", 0, point + 12 + g_NumInputPoints );
            g_pDebugConsole->TextOutAt( g_Text, 0, shock_mode + 12 + g_NumInputPoints);
        }
    }

}


//=============================================================================
// Function:    InputCallback::Terminate
//=============================================================================
// Description: Stops all objects in service.
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
// Function:    InputCallback::UpdateScreen
//=============================================================================
// Description: Update the controller system with new virtual/nonvirtual info
//
// Parameters:  global virtual data variables 
//
// Returns:     none - output new info to debug console
//
// Notes:
//------------------------------------------------------------------------------
void UpdateScreen( void )
{
    //radControllerSystemGet()->SetCaptureRate( g_CaptureRate );
    //radControllerSystemGet()->MapVirtualTime( 0, g_Latency );
    //radControllerSystemGet()->SetBufferTime( g_BufferTime );
   
    for ( int point = 0; point < g_NumInputPoints; point ++ )
    {
        g_pInputPoint[point]->SetTolerance( g_Tolerance );
        g_pInputPoint[point]->SetRange( g_Min, g_Max );
    }

    sprintf
    (
        g_Text,
        "VT:[%s] Lat:[%d] Min:[%.2f] Max:[%.2f] Tol:[%.2f] Buf:[%d] CRate:[%d]      ",
        g_BufferTime > 0 ? "on" : "off",
        g_Latency,
        g_Min,
        g_Max,
        g_Tolerance,
        g_BufferTime,
        g_CaptureRate
    );
    
    g_pDebugConsole->SetTextColor( RGB( 255, 0,  0 ) );
    g_pDebugConsole->TextOutAt( g_Text, 0, 6 );
   
}
