//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "initradcore.hpp"
#include "initradsound.hpp"

#include <radtextdisplay.hpp>
#include <radsound.hpp>
#include <radmemorymonitor.hpp>

#define NUM_CLIP_PLAYERS 100
#define NUM_CLIPS 10

#define GAME_LOOP_TIME_MS 40

#define TEST_PRIORITY
    #define SINGLE_CLIP_PRIORITY 10

static const char * g_pClipFileNames[] =
{
    "1000Hz_22050.rsd",
    "1000Hz_44100.rsd",
    "100Hz_44100.rsd",
    "10kHz_44100.rsd",
    "2500Hz_22050.rsd",
    "2500Hz_44100.rsd",
    "5000Hz_44100.rsd",
//    "clip0_mono_22050.rsd",
//    "dummy_mono_44100.rsd",
    "heli.rsd"
};

static unsigned int g_CurrentClipFile = 0;
static unsigned int g_GameLoops = 0;

static ref< IRadSoundClip > g_Clips[ NUM_CLIPS ];
static ref< IRadSoundClipPlayer > g_ClipPlayers[ NUM_CLIP_PLAYERS ];

int clipThrashTest( int argc, char * argv[] )
{
    ::rstRadCoreInitialize( );
    ::rstRadSoundInitialize( );

    ref< IRadTextDisplay > refIRadTextDisplay;
    ::radTextDisplayGet( & refIRadTextDisplay, RADMEMORY_ALLOC_DEFAULT );
    refIRadTextDisplay->SetBackgroundColor( 0x000000FF );

    unsigned int numFiles = sizeof( g_pClipFileNames ) / sizeof( char * );

    ref< IRadSoundHalPositionalGroup > refIGroup = ::radSoundHalPositionalGroupCreate( RADMEMORY_ALLOC_DEFAULT );

    radSoundVector vector( 10.0f, 0.0f, 0.0f );
    refIGroup->SetPosition( & vector );

    // High priority clip

    ref< IRadSoundRsdFileDataSource > refDataSource = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
//    refDataSource->InitializeFromFileName( "clip1_stereo_44100.rsd", 0, IRadSoundHalAudioFormat::Milliseconds, NULL );
    refDataSource->InitializeFromFileName( "heli.rsd", 0, IRadSoundHalAudioFormat::Milliseconds, NULL );
    ref< IRadSoundClipPlayer > refPlayer = ::radSoundClipPlayerCreate( RADMEMORY_ALLOC_DEFAULT );
    ref< IRadSoundClip > refClip = ::radSoundClipCreate( RADMEMORY_ALLOC_DEFAULT );
    refClip->Initialize( refDataSource , ::radSoundHalSystemGet( )->GetRootMemoryRegion( ), true, "priority clip" );
    refPlayer->SetClip( refClip );
    refPlayer->SetPriority( SINGLE_CLIP_PRIORITY );
    refPlayer->Play( );
    refDataSource = NULL;
    refClip = NULL;

    for( unsigned int i = 0; i < NUM_CLIPS; i++ )
    {
        ref< IRadSoundRsdFileDataSource > refFileDs = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
        refFileDs->InitializeFromFileName( g_pClipFileNames[ ( i % numFiles ) ], 0, IRadSoundHalAudioFormat::Milliseconds, NULL );
        g_Clips[ i ] = ::radSoundClipCreate( RADMEMORY_ALLOC_DEFAULT );
        g_Clips[ i ]->Initialize( refFileDs, ::radSoundHalSystemGet( )->GetRootMemoryRegion( ), false, g_pClipFileNames[ ( i % numFiles ) ] );
        
        while( g_Clips[ i ]->GetState( ) == IRadSoundClip::Initializing )
        {
            ::rstRadCoreService( );
            ::rstRadSoundService( );       
        }
    }

    for( unsigned int j = 0; j < NUM_CLIP_PLAYERS; j++ )
    {
        g_ClipPlayers[ j ] = ::radSoundClipPlayerCreate( RADMEMORY_ALLOC_DEFAULT );
        g_ClipPlayers[ j ]->SetClip( g_Clips[ j % NUM_CLIPS ] );
        g_ClipPlayers[ j ]->Play( );

        g_ClipPlayers[ j ]->SetPositionalGroup( refIGroup );

        if( j == 0 )
        {
            g_ClipPlayers[ j ]->SetPriority( 0 );
        }
    }
  
    unsigned int next = ::radTimeGetMilliseconds( );
    unsigned int startTime = next;
    
    bool isDone = false;
    unsigned int robert = 0;

    while( isDone == false )
    {
        unsigned int now = radTimeGetMilliseconds( );

        if ( next >= now )
        {
            continue;
        }

        g_GameLoops++;

        next = now + GAME_LOOP_TIME_MS;        
        
        ::rstRadCoreService( );
        ::rstRadSoundService( );       
        
        unsigned int index = rand() % NUM_CLIP_PLAYERS;

        ref< IRadSoundClipPlayer > refClipPlayer = g_ClipPlayers[ index ];
        
        if ( refClipPlayer->GetState( ) == IRadSoundClipPlayer::Stopped && ( robert  ) <= 30000 )
        {
            if( ( now % 500 ) > 400 )
            {
                refClipPlayer->Play( );
            }
            else
            {
                refClipPlayer->SetClip( g_Clips[ ( index + g_GameLoops ) % NUM_CLIPS ] );
//                refClipPlayer->Play( );
            }
        }
        else 
        {
            if( ( now % 500 ) > 450 || index == 0 )
            {
                refClipPlayer->SetClip( NULL );
            }
        }

        static unsigned int positionChangeTime = 0;
        if( now - positionChangeTime > 1000 )
        {
            positionChangeTime = now;
            vector.m_x = 100.0f * sinf( ( float ) now );
            refIGroup->SetPosition( & vector );
        }
    }

    for( unsigned int j = 0; j < NUM_CLIP_PLAYERS; j++ )
    {
        g_ClipPlayers[ j ] = NULL;
    }
    for( unsigned int i = 0; i < NUM_CLIPS; i++ )
    {
        g_Clips[ i ] = NULL;
    }

    radMemoryMonitorSuspend( );

    refPlayer = NULL;
    refIGroup = NULL;

    refIRadTextDisplay = NULL;

    ::rstRadSoundTerminate( );
    ::rstRadCoreTerminate( );

    return 1;
}
