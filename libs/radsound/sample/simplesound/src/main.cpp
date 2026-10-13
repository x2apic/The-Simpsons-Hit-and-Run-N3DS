//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:   Radical Sound Library - Sample Application
//
// Description: This file contains code to illustrate the correct usage
//              method of playing a simple clip file.
//
// Revisions:
//   November 13, 2001 -- breimer
//
//=============================================================================

void MemoryHackCallback() { }

//=============================================================================
// Include Files
//=============================================================================

#include <stdlib.h>
#include <string.h>
#include <raddebug.hpp>
#include <radfile.hpp>
#include <radplatform.hpp>
#include <radsound.hpp>
#include <radlinkedclass.hpp>
#include <radmemorymonitor.hpp>
#include <raddebugcommunication.hpp>
#include <radthread.hpp>

#ifdef RAD_WIN32
#include <radsound_win32.hpp>
#endif

#ifdef RAD_PS2
#include <radsound_ps2.hpp>
#endif

// On the PS2 we can either buffer data on the IOP, or Ee the sound system
// doesn't attemt to  hide this.

//
// The sound memory size, this memory is reserved for the lifetime of the game
// on the PS2 it must be zero because this memory is dedicated sound memory.
//

#ifdef RAD_PS2
    #define SOUND_MEMORY 0
#else
    #define BUFFER_MEMORY_SPACE radMemorySpace_Local
    #define SOUND_MEMORY ( 1024 * 1024 * 4 )
#endif

#define STREAM_BUFFER_SIZE 5000

//
// The number of auxillary (fx) sends.
//

#define AUX_SENDS 2

//=============================================================================
// Static Data Defintions
//=============================================================================

//
// The RSD file to play
//
const char ClipFileName[] = "sample.rsd";
const char StreamFileName[] = "stream.rsd";

bool g_Quit = false;

//=============================================================================
// Function:    radStartup
//=============================================================================
// Description: Startup all systems needed by this sample.
//
// Parameters:  banner - a banner to display
//
//------------------------------------------------------------------------------


void radStartup( const char* banner )
{
    rDebugString( banner );
    
    ::radMemoryInitialize( );
    ::radThreadInitialize( );

    #if defined RAD_PS2
	    ::radPlatformInitialize( "irx/", IOPMediaHost, GameMediaCD, NULL, RADMEMORY_ALLOC_DEFAULT );
    #elif defined RAD_WIN32
        ::radPlatformInitialize( NULL );
    #elif defined RAD_XBOX || defined RAD_GAMECUBE
        ::radPlatformInitialize( );
    #endif

    ::radTimeInitialize( );

    /*::radMemoryMonitorInitialize( 50 * 1024 );

    #if defined RAD_PS2
        radDbgComTargetInitialize( Deci );
    #elif defined RAD_WIN32 || defined RAD_XBOX
        radDbgComTargetInitialize( WinSocket );
    #elif defined RAD_GAMECUBE
        radDbgComTargetInitialize( HostIO );
    #endif */

    ::radFileInitialize( );
}

void radSleep( unsigned int time )
{
    static unsigned int nextTime = ::radTimeGetMilliseconds( );

    unsigned int now = ::radTimeGetMilliseconds( );

    if ( nextTime <= now )
    {
        for ( unsigned int t = 0; t < 10; t ++ )
        {
            ::radSoundHalSystemGet( )->Service( );
            ::radFileService( );
            /*::radDbgComService( );
            ::radMemoryMonitorService( );  */
            ::radSoundHalSystemGet( )->ServiceOncePerFrame( );
        }

        nextTime = now + 20;
    }
}

//=============================================================================
// Function:    radShutdown
//=============================================================================
// Description: Shutdown all systems used by this sample.
//
// Parameters:  banner - a banner to display
//
//------------------------------------------------------------------------------

void radShutdown( const char* banner )
{
    ::radFileTerminate( );
    /*::radMemoryMonitorTerminate( );
    radDbgComTargetTerminate( ); */
    ::radTimeTerminate( );
    ::radPlatformTerminate( );
    ::radThreadTerminate( );
    ::radMemoryTerminate( );
        
    //
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
#ifdef RAD_DEBUG
    radObject::DumpObjects( );    
#endif
    
    rDebugString( banner );
    rDebugString( "\n" );
}

void Go( void )
{
    //
    // Clip
    //

    ref< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource_Clip = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundRsdFileDataSource_Clip->InitializeFromFileName( ClipFileName, 0, 0, IRadSoundHalAudioFormat::Samples, NULL );

    ref< IRadSoundClip > refIRadSoundClip = ::radSoundClipCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundClip->Initialize( refIRadSoundRsdFileDataSource_Clip, radSoundHalSystemGet( )->GetRootMemoryRegion( ), false, "Test Clip" );
    ref< IRadSoundClipPlayer > refIRadSoundClipPlayer = ::radSoundClipPlayerCreate( RADMEMORY_ALLOC_DEFAULT );

    refIRadSoundClipPlayer->SetClip( refIRadSoundClip );
    refIRadSoundClipPlayer->Play( );
                
    while( ! g_Quit && refIRadSoundClipPlayer->IsPlaying( ) )
    {
        radSleep( 0 );
    }

    //
    // Reverb
    //

    ref< IRadSoundHalPositionalGroup > pPosGroup = ::radSoundHalPositionalGroupCreate(RADMEMORY_ALLOC_DEFAULT);
    refIRadSoundClipPlayer->SetPositionalGroup( pPosGroup );

    ref< IRadSoundHalEffectEAX2Reverb > pEffect = radSoundHalEffectEAX2ReverbCreate( RADMEMORY_ALLOC_DEFAULT );
    pEffect->SetEnvironmentDiffusion(1.0f);
    pEffect->SetRoom(-1000);
    pEffect->SetRoomHF(-1200);
    pEffect->SetDecayTime(1.49f);
    pEffect->SetDecayHFRatio(0.54f);
    pEffect->SetReflections(-370);
    pEffect->SetReflectionsDelay(0.007f);
    pEffect->SetReverb(1030);
    pEffect->SetReverbDelay(0.011f);
    ::radSoundHalSystemGet()->SetAuxEffect(0, pEffect);
    ::radSoundHalListenerGet()->SetEnvEffectsEnabled(true);
    refIRadSoundClipPlayer->Play( );
                
    while( ! g_Quit && refIRadSoundClipPlayer->IsPlaying( ) )
    {
        radSleep( 0 );
    }

    //
    // Stream
    //

    ref< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundRsdFileDataSource->InitializeFromFileName( StreamFileName, 0, 0, IRadSoundHalAudioFormat::Samples, NULL );

    ref< IRadSoundStreamPlayer > refIRadSoundStreamPlayer = ::radSoundStreamPlayerCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundStreamPlayer->InitializeAsync( STREAM_BUFFER_SIZE, IRadSoundHalAudioFormat::Milliseconds,
        ::radSoundHalSystemGet( )->GetRootMemoryRegion( ), "Test Stream Player" );
    refIRadSoundStreamPlayer->SetDataSource( refIRadSoundRsdFileDataSource );

    refIRadSoundStreamPlayer->Play( );

    while( ! g_Quit && refIRadSoundStreamPlayer->IsPlaying( ) )
    {
        radSleep( 0 );
    }
}

//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Function:    main
//=============================================================================
// Description: Main entry point. Platform specific entry.
//
// Parameters:  see Windows.
//
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_WIN32
    int main( int argc, char* argv[ ] )
#endif
#ifdef RAD_XBOX
    void _cdecl main (void)
#endif
#ifdef RAD_PS2
    int main( int argc, char* argv[ ] )
#endif
#ifdef RAD_GAMECUBE
    void main( void )
#endif
{
    //
    // Startup all necessary systems (this is a locally defined function)
    //
    ::radStartup( "Foundation Tech - Radical Sound Sample V1.00\n" );

    //
    // First, lets initialize the sound system.  (It should really be initialized
    // along with the file, debug, etc. systems.  For this example, however,
    // we will do it here).  Notice that the sound system relies on the timer
    // sytem being setup.
    //
    ::radSoundHalSystemInitialize( RADMEMORY_ALLOC_DEFAULT );

    IRadSoundHalSystem::SystemDescription desc;
    ::memset( ( void * ) & desc, 0, sizeof( desc ) );

    desc.m_MaxRootAllocations = 8;
    desc.m_NumAuxSends = AUX_SENDS;
    #ifndef RAD_PS2
    desc.m_ReservedSoundMemory = SOUND_MEMORY;
    #endif 
    #ifdef RAD_WIN32
    desc.m_SamplingRate = 48000;
    #endif

    ::radSoundHalSystemGet( )->Initialize( desc );

    // Run the test.

    Go( );

    //
    // Shutdown the sound system
    //
    ::radSoundHalSystemTerminate( );

    //
    // Shutdown the other systems (this is a local function)
    //
    ::radShutdown( "Sound Sample - Terminates" );

    #if !defined( RAD_GAMECUBE ) && !defined( RAD_XBOX )
        return( 0 );
    #endif
}

void LeakDetectionStart(void) {}
void LeakDetectionStop(void) {}
void LeakDetectionAddRecord(const void* pMemory, const unsigned int size, const radMemoryAllocator heap) {}
void LeakDetectionRemoveRecord(void* pMemory) {}
