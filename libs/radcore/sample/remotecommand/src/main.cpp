//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:   Radical Foundation Tech Remote Comamnd Sample Program
//
// Description:	This file contains code to illustrate how to register remote
//              commands. This file interacts with the AtgMayaRemote plug-in.
//
// Revisions:	Sept 30, 2002
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <raddebugcommunication.hpp>
#include <radremotecommand.hpp>
#include <radthread.hpp>
#include <radtime.hpp>
#include <radremotecommand.hpp>

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Forward References
//=============================================================================

HrcsResultCode Function1( int argc, char* argv[], void* userData );
HrcsResultCode Function2( int argc, char* argv[], void* userData );

//=============================================================================
// Static Data Defintions
//=============================================================================

static bool s_Running = true;

//=============================================================================
// Class Declarations
//=============================================================================

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
// Parameters:  
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
    rDebugString( "Foundation Tech - Remote Command Sample V1.00\n" );
    
    //
    // Do the system initialization.
    //
    radThreadInitialize( );
    radMemoryInitialize( );

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

    //    
    // Now the time system
    //
    radTimeInitialize( );

    //
    // Initialize the debug communication system
    //
    #ifdef RAD_PS2

        radDbgComTargetInitialize( Deci );

    #elif defined RAD_WIN32 || defined RAD_XBOX

        radDbgComTargetInitialize( WinSocket );

    #elif defined RAD_GAMECUBE
        
        radDbgComTargetInitialize( HostIO );

    #endif        

    //
    // Initialize the remote command system. Us macros that will vanish.
    //
    RcsRemoteCommandInitialize( );

    //
    // Lets add a couple of functions.
    //
    RcsRegisterRemoteFunction( "Function1", Function1, NULL );
    RcsRegisterRemoteFunction( "Function2", Function2, &s_Running );

    //
    // Enter main loop.
    //
    while( s_Running )
    {
        //
        // Must service communication system.
        //
        radDbgComService( );
    }

    //
    // Remove the registered functions.
    //
    RcsUnRegisterRemoteFunction( "Function1" );
    RcsUnRegisterRemoteFunction( "Function2" );

    //
    // Shut down the remote command system.
    //
    RcsRemoteCommandTerminate( );

    //
    // Finally clean up.
    //

    radDbgComTargetTerminate( );
    radTimeTerminate( );
    radPlatformTerminate( );
    radMemoryTerminate( );
    radThreadTerminate( );

    //
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
    #ifdef RAD_DEBUG
        radObject::DumpObjects( );    
    #endif

    rDebugString("Remote Command Sample - Terminates\n");
#ifndef RAD_GAMECUBE
    return( 0 );
#endif
}

//=============================================================================
// Function:    Function1 
//=============================================================================
// Description: Invoked when Maya issues this function from mel.
//
// Parameters:  argc, argv - command args
//              userData - not used
//
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------

HrcsResultCode Function1
( 
    int argc,
    char* argv[],
    void* userData
)
{
    //
    // Simply print the arguement count and arguments.
    //
    rDebugPrintf( "Function1 invoked with %d arguments\n", argc );
    for( int i = 0; i < argc ; i++ )
    {
        rDebugPrintf( "    Argument %d = %s\n", i, argv[ i ] );
    }     

    return( HrcsSuccess );

}

//=============================================================================
// Function:    Function2
//=============================================================================
// Description: Invoked when Maya issues this function from mel.
//
// Parameters:  argc, argv - command args
//              userData - address of boolean to terminate loop.
//
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------

HrcsResultCode Function2
( 
    int argc,
    char* argv[],
    void* userData
)
{
    //
    // Simply print the arguement count and arguments.
    //
    rDebugPrintf( "Function2 invoked with %d arguments\n", argc );
    for( int i = 0; i < argc ; i++ )
    {
        rDebugPrintf( "    Argument %d = %s\n", i, argv[ i ] );
    }     

    if( argc != 1 )
    {
        return( HrcsFail );
    }

    if( 0 == stricmp( "Stop", argv[ 0 ] ) )
    {
        *((bool*) userData) = false;   
    }

    return( HrcsSuccess );
}




