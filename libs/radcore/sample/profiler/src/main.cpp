//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:   Radical Foundation Tech radProfiler Sample Program
//
// Description:	
//
// Revisions:	now
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <raddebugcommunication.hpp>
#include <radmemorymonitor.hpp>
#include <radmemory.hpp>
#include <radtime.hpp>
#include <radthread.hpp>
#include <radstacktrace.hpp>
#include <radfile.hpp>
#include <radprofiler.hpp>
#include <raddebugwatch.hpp>

#if defined RAD_WIN32
#include "window.hpp"
#endif

//=============================================================================
// Macros and defines
//=============================================================================

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Forward References
//=============================================================================

//=============================================================================
// Static Data Defintions
//=============================================================================
bool g_Done;

//=============================================================================
// Class Declarations
//=============================================================================

//=============================================================================
// Public Functions
//=============================================================================

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
int main( int argc, char* argv[ ] )
#endif

#ifdef RAD_GAMECUBE
void main( void )
#endif

{
    //
    // Display a banner message.
    //
    rDebugPrintf( "Foundation Tech - radProfiler Sample\n" );

    ::radThreadInitialize( );
    ::radMemoryInitialize( );
    ::radTimeInitialize( );

    //
    // First initialized the platform. Each platform has specific initialization,
    // PS2 requires path to where IRXs reside and where to load them form. We
    // also intialize the debug communication system. This is required if the
    // Radical File Server will be used.
    //
    #ifdef RAD_PS2
    ::radPlatformInitialize( "\\usr\\local\\sce\\iop\\modules\\", IOPMediaHost );
	#endif // RAD_PS2

    #ifdef RAD_WIN32
    //
    // Windows initialization requires instance and main window. This program does
    // not use a main window, so just set it to null,
    //
	::radPlatformInitialize(
		::CreateMainWindow( hInstance, SW_SHOWNORMAL ),
			hInstance, RADMEMORY_ALLOC_DEFAULT );
    #endif

    #ifdef RAD_XBOX
    //
    // XBOX does not need any info.
    //
    ::radPlatformInitialize( );
    #endif

    //
    // Game Cube
    //
    #ifdef RAD_GAMECUBE
    ::radPlatformInitialize( );
    #endif

#if defined (RAD_WIN32) || defined (RAD_XBOX)
	radDbgComTargetInitialize( WinSocket );
#endif

#ifdef RAD_PS2
	radDbgComTargetInitialize( Deci );
#endif

#ifdef RAD_GAMECUBE
  	radDbgComTargetInitialize( HostIO );
#endif

    //========================================================================
    // Test code
    //========================================================================
    radDbgWatchInitialize( "\\", 50 * 1024 );
    ::radProfilerInitialize( );
    
    //
    // unit test for begin/end profile
    //
    for ( unsigned int j = 0; j < 1000000; j++ )
    {
        radDbgComService( );
        radProfilerBeginProfile( "Root1" );

            radProfilerBeginProfile( "L0-0-0" );
                radProfilerBeginProfile( "L1-0-0" );
                    radProfilerBeginProfile( "L2-0-0" );
                    radProfilerEndProfile( "L2-0-0" );

                    radProfilerBeginProfile( "L2-0-1" );
                    radProfilerEndProfile( "L2-0-1" );

                    radProfilerBeginProfile( "L2-0-2" );
                    radProfilerEndProfile( "L2-0-2" );
                radProfilerEndProfile( "L1-0-0" );

            radProfilerEndProfile( "L0-0-0" );

            radProfilerBeginProfile( "L0-0-1" );
            radProfilerEndProfile( "L0-0-1" );

            radProfilerBeginProfile( "L0-0-2" );
            radProfilerEndProfile( "L0-0-2" );

            radProfilerBeginProfile( "L0-0-3" );
                radProfilerBeginProfile( "L1-0-3" );
                radProfilerEndProfile( "L1-0-3" );
                radProfilerBeginProfile( "L1-0-4" );
                radProfilerEndProfile( "L1-0-4" );
            radProfilerEndProfile( "L0-0-3" );

        radProfilerEndProfile( "Root1" );

        radProfilerBeginProfile( "Root2" );
        radProfilerEndProfile( "Root2" );

    }

    ::radProfilerTerminate( );
    radDbgWatchTerminate( );

    //
    // Finally clean up.
    //
    ::radPlatformTerminate( );
    ::radTimeTerminate( );
    ::radMemoryTerminate( );
    ::radThreadTerminate( );

    //
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
    #ifdef RAD_DEBUG
    radObject::DumpObjects( );    
    #endif

    rDebugString("radProfiler Sample - Terminates\n");
#ifndef RAD_GAMECUBE
    return( 0 );
#endif
}
