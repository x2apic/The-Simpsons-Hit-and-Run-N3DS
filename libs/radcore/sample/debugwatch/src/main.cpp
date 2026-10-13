//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:   Radical Foundation Tech Debug Watch Sample Program
//
// Description:	This file contains code to illustrate how process variabls
//              can be added to the Watcher ( perviously known as the Game Tuner.
//
// Revisions:	Apr 9, 2001
//
//=============================================================================

//
// This must be defined or teh debug watch functions vanish.
//

#ifndef RAD_RELEASE
#define DEBUGWATCH    // This is now more properly handled by radoptions.hpp
#endif // RAD_RELEASE

//=============================================================================
// Include Files
//=============================================================================

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <raddebugcommunication.hpp>
#include <raddebugwatch.hpp>
#include <radtime.hpp>

//=============================================================================
// Local Defintions
//=============================================================================

enum Speed
{
    Fast,
    Medium,
    Slow
};

//
// Need to pass eumeraiton information to watch point/.
//
IRadDebugWatch::EnumerationElement EnumInfo[ ] = { {"Fast", Fast }, {"Medium", Medium }, {"Slow",Slow } };


//=============================================================================
// Forward References
//=============================================================================

void TimerChange( void* userData );
void UpdateVector( void* userData );

//=============================================================================
// Static Data Defintions
//=============================================================================

static unsigned int TimeRate = 500;
static char String[ 100 ];

static IRadTimerList*   s_pTimerList = NULL;
static IRadTimer*       s_pTimer = NULL;
static class CTimerCallback* s_pTimerCallback;

//=============================================================================
// Class Declarations
//=============================================================================

//
// This sammple uses a timer. This object is used to field the callback. Simply
// updates the static string varibale.
//
class CTimerCallback : public IRadTimerCallback
{
    public:

    void OnTimerDone( unsigned int elapsedTime, void * pUserData )      
    {
        //
        // Lets update the string with the elapsed time.
        //
        sprintf( String, "Time elapsed = %d ms", elapsedTime );
    }
};

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
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    main
//=============================================================================
// Description: Main entry point.
//
// Parameters:  on PS2, pass IP address if you want to use TCPIP via USB.
//
// Returns:     0
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
    // Display a banner message.
    //
    rDebugString( "Foundation Tech - Debug Watch Sample V1.00\n" );
    
    //
    // Must initialize memory first thing
    //

    radMemoryInitialize( );

    //
    // First initialized the platform. Each platform has specific initialization,
    // PS2 requires path to where IRXs reside and where to load them form. We
    // also intialize the debug communication system. This is required if the
    // Radical File Server will be used.
    //
    #ifdef RAD_PS2

        radPlatformInitialize( "usr\\local\\sce\\iop\\modules\\", IOPMediaHost );

    #elif defined RAD_WIN32

        //
        // Windows initialization requires instance and main window. This program does
        // not use a main window, so just set it to null,
        //
        radPlatformInitialize( NULL, hInstance );

    #elif defined RAD_GAMECUBE || defined RAD_XBOX

        radPlatformInitialize( );

    #endif
    
    // Now the time system

    radTimeInitialize( );

    //
    // Initialize the debug communication system
    //

    #ifdef RAD_PS2

        //
        // For the PS2, we accept a command line arguement. If one is present, we set
        // up debug communicaiton system for USBTcpip/
        //

        if( argc == 2 )
        {
            //
            // When using TCPIP via USB, must provide address info.
            //
    	    radDbgComUsbTcpIpInitInfo info;

    	    strcpy( info.m_IPAddress, argv[ 1 ] );
	        strcpy( info.m_SubMask, "255.255.0.0" );
	        strcpy( info.m_Gateway, "10.1.1.14" );
    
            radDbgComTargetInitialize( UsbTcpIp, radDbgComDefaultPort, &info );
        }
        else
        {
            //
            // Initialize using DECI as the communication avenue.
            //
            radDbgComTargetInitialize( Deci );
        }

    #elif defined RAD_WIN32 || defined RAD_XBOX

        radDbgComTargetInitialize( WinSocket );

    #elif defined RAD_GAMECUBE
        
        radDbgComTargetInitialize( HostIO );

    #endif        

    //
    // Lets initialize the watch point system. Place all process variables under the
    // root "\"
    //
	radDbgWatchInitialize_Macro( ( "\\" ) );

    //
    // Lets added to variables relecting to the time. A string and a value. When the
    // value of the timer changes we stop the time and start a new one with the new
    // value. Need a timer list to do this.
    //
    radTimeCreateList( &s_pTimerList, 1 );
    s_pTimerCallback = new CTimerCallback( );
    s_pTimerList->CreateTimer( &s_pTimer, TimeRate, s_pTimerCallback );
        
    radDbgWatchAddString_Macro( ( String, sizeof( String ), "ElaspedTime", "\\Game\\Timer", NULL, NULL, true ) );
    radDbgWatchAddUnsignedInt_Macro( ( &TimeRate, "Rate","\\Game\\Timer\\Rate" ,TimerChange, NULL, 10, 10000 ) );

    //
    // This stuff is to test the XML saving
    //
    radDbgWatchAssociateFile("\\game\\fog", "fog.xml");

    bool    fogEnable = false;;
    unsigned int  fogRed = 1;
    unsigned int  fogGreen = 2;
    unsigned int  fogBlue = 3;
    unsigned int fogtype  = 4;
  
    radDbgWatchAddBoolean_Macro( ( &fogEnable, "enable", "\\game\\fog\\enable" ) );
    radDbgWatchAddUnsignedInt_Macro( ( &fogRed, "red", "\\game\\fog\\color" ) );
    radDbgWatchAddUnsignedInt_Macro( ( &fogGreen, "green", "\\game\\fog\\color" ) );
    radDbgWatchAddUnsignedInt_Macro( ( &fogBlue, "blue", "\\game\\fog\\color" ) );

    radDbgWatchAddUnsignedInt_Macro( ( &fogtype, "type", "\\game\\fog\\parameter" ) );

    //
    // Lets monitor a bool variable. We will use this variable to terminate 
    // the gamne loop as well.
    //
    bool    gameRunning = true;

    radDbgWatchAddBoolean_Macro( ( &gameRunning, "Game Running" ) );

    //
    // Some more variables.
    //
    unsigned short FrameCount = 0;
    unsigned int SpeedCount = 0;
    float Vector[ 3 ];
    Vector[0] = 0.1f;
    Vector[1] = 0.2f;
    Vector[2] = 0.3f;

    radDbgWatchAddUnsignedShort_Macro( ( &FrameCount, "Frame Count", NULL, NULL, NULL, 0, 10000, true ) );
    radDbgWatchAddUnsignedInt_Macro( ( &SpeedCount, "Frame Rate", NULL, NULL, NULL, 0, 100, true ) );
    radDbgWatchAddVector_Macro( ( Vector, "Postion","\\Game\\Vectors" ) );

    //
    // Add a function which will update the vector.
    //
    radDbgWatchAddFunction_Macro( ( "Update Vector", UpdateVector, &Vector ) );

    //
    // Send a binary blob of data. Current host game tuner does not respond to this.
    // Future revisions may. It is provided for special tool used by Snowboarding.
    //
    unsigned char Binary[ 100 ];
    radDbgWatchIssueHostRequest( 5, Binary, sizeof( Binary ) );

    //
    // Lets add an enumeration.
    //
    Speed   gameSpeed = Slow;
    
    radDbgWatchAddEnumeration_Macro( ( (int*) &gameSpeed, EnumInfo, 3, "Speed") );
   
    unsigned int mod;

    //
    // Enter our game loop. This just updates some varibles, It also services
    // the debugcommunication system and our timer list.
    //
    while( gameRunning )
    {

#ifdef RAD_WIN32
        Sleep( 3 );
#endif // RAD_WIN32
        radDbgComService( );
    
        s_pTimerList->Service( );

        FrameCount++;

        if( gameSpeed == Slow )
        {
            mod = 1000;
        }
        else if( gameSpeed == Medium )  
        {
            mod = 100;
        }
        else
        {
            mod = 10;
        }
        
        if( FrameCount % mod == 0 )
        {
            SpeedCount++;
        }    
        
        if( SpeedCount > 100000 )
        {
            SpeedCount = 0;
        }
    }

    //
    // When we are done, Kill the timer stuff.
    //
    s_pTimer->Release( );
    s_pTimerList->Release( );
    delete s_pTimerCallback;
    
    //
    // Unregister our watch points and terminate the watch system.
    //

    radDbgWatchDelete_Macro( ( &fogEnable ) );
    radDbgWatchDelete_Macro( ( &fogRed ) );
    radDbgWatchDelete_Macro( ( &fogGreen ) );
    radDbgWatchDelete_Macro( ( &fogBlue ) );
    radDbgWatchDelete_Macro( ( &fogtype ) );

    radDbgWatchDelete_Macro( ( UpdateVector ) );
    radDbgWatchDelete_Macro( ( &gameRunning ) );
    radDbgWatchDelete_Macro( ( &FrameCount ) );
    radDbgWatchDelete_Macro( ( &SpeedCount ) );
    radDbgWatchDelete_Macro( ( &gameSpeed ) );
    radDbgWatchDelete_Macro( ( String ) );
    radDbgWatchDelete_Macro( ( &TimeRate ) );
    radDbgWatchDelete_Macro( ( Vector ) );


    //
    // ensure all changes are transmitted over
    //
    for ( int i = 0; i < 20; i ++ )
    {
#ifdef RAD_WIN32
        Sleep( 3 );
#endif // RAD_WIN32
        radDbgComService( );
    }


    radDbgWatchTerminate_Macro( ( ) );

    //
    // Finally clean up.
    //

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

    rDebugString("Watch Point Sample - Terminates\n");
#ifndef RAD_GAMECUBE
    return( 0 );
#endif
}

//=============================================================================
// Function:    TimerChange
//=============================================================================
// Description: Invoked when watchpoint value is alterd.
//
// Parameters:  userData - data supplied when console created
//
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------

void TimerChange( void* userData )
{
    s_pTimer->Release( );
    s_pTimerList->CreateTimer( &s_pTimer, TimeRate, s_pTimerCallback );
}


//=============================================================================
// Function:    UpdataVector
//=============================================================================
// Description: Invoked when user invokes function
//
// Parameters:  userData - data supplied
//
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------

void UpdateVector( void* userData )
{
    //
    // Just play with the numbers.
    //
    float* pVector = (float*) userData;
        
    float temp;

    temp = pVector[ 0 ];
    pVector[ 0 ] = pVector[ 1 ];
    pVector[ 1 ] = pVector[ 2 ];
    pVector[ 2 ] = temp;
}


