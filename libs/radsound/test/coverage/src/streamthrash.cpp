//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "initradcore.hpp"
#include "initradsound.hpp"

#include <radtextdisplay.hpp>
#include <radsound.hpp>

#define NUM_STREAMS 7

#define USE_BUFFERED_DATA_SOURCE

#ifdef USE_BUFFERED_DATA_SOURCE

    #define STREAM_SIZE_IN_MS 1000
    #define BUFFER_SIZE_IN_MS 6000

    #if defined RAD_PS2
        #define BUFFERED_DATA_SOURCE_MEMORY_SPACE radMemorySpace_Iop
    #else
        #define BUFFERED_DATA_SOURCE_MEMORY_SPACE radMemorySpace_Local
    #endif

#else
    #define STREAM_SIZE_IN_MS 4000
#endif

#define GAME_LOOP_TIME_MS 30

const char * g_pMonoFileNames[] =
{
    "song0_mono_44100.rsd",
    NULL
};

const char * g_pStereoFileNames[] =
{
    "song1_stereo_44100.rsd",
    "song2_stereo_44100.rsd",
    NULL
};

static unsigned int g_CurrentStereoFile = 0;
static unsigned int g_CurrentMonoFile = 0;
static unsigned int g_GameLoops = 0;

struct Stream
{
    ref< IRadSoundStreamPlayer > m_refIRadSoundStreamPlayer;
    #ifdef USE_BUFFERED_DATA_SOURCE
        ref< IRadSoundBufferedDataSource > m_refIRadSoundBufferedDataSource;
    #endif
};

Stream g_Streams[ NUM_STREAMS ];

int streamThrashTest( int argc, char * argv[] )
{
    ::rstRadCoreInitialize( );
    ::rstRadSoundInitialize( );

    ref< IRadTextDisplay > refIRadTextDisplay;
    ::radTextDisplayGet( & refIRadTextDisplay, RADMEMORY_ALLOC_DEFAULT );
    refIRadTextDisplay->SetBackgroundColor( 0x000000FF );

    for( unsigned int i = 0; i < NUM_STREAMS; i ++ )
    {
        g_Streams[ i ].m_refIRadSoundStreamPlayer = ::radSoundStreamPlayerCreate( RADMEMORY_ALLOC_DEFAULT );
        g_Streams[ i ].m_refIRadSoundStreamPlayer->InitializeAsync(
            STREAM_SIZE_IN_MS,
            IRadSoundHalAudioFormat::Milliseconds,
            ::radSoundHalSystemGet( )->GetRootMemoryRegion( ),
            "Stream Player" );

        #ifdef USE_BUFFERED_DATA_SOURCE
            g_Streams[ i ].m_refIRadSoundBufferedDataSource = ::radSoundBufferedDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
            g_Streams[ i ].m_refIRadSoundBufferedDataSource->Initialize(
                BUFFERED_DATA_SOURCE_MEMORY_SPACE,
                ::radMemorySpaceGetAllocator( BUFFERED_DATA_SOURCE_MEMORY_SPACE, RADMEMORY_ALLOC_DEFAULT ),
                BUFFER_SIZE_IN_MS,
                IRadSoundHalAudioFormat::Milliseconds,
                NULL,
                "Buffered Data Source" );
        #endif // BUFFERED_DATA_SOURCE
    }

    unsigned int next = ::radTimeGetMilliseconds( );
    
    while( 1 )
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
        
        unsigned int index = rand() % NUM_STREAMS;

        Stream * pStream = & ( g_Streams[ index  ] );
        
        if ( pStream->m_refIRadSoundStreamPlayer->GetState( ) == IRadSoundStreamPlayer::NoSource )
        {
            const char * pFileName = NULL;

            if ( (index % 2) == 0 )
            {
                pFileName = g_pStereoFileNames[ ++g_CurrentStereoFile ];

                if ( pFileName == NULL )
                {
                    g_CurrentStereoFile = 0;
                    pFileName = g_pStereoFileNames[ 0 ];
                }
            }
            else
            {
                pFileName = g_pMonoFileNames[ ++g_CurrentMonoFile ];

                if ( pFileName == NULL )
                {
                    g_CurrentMonoFile = 0;
                    pFileName = g_pMonoFileNames[ 0 ];
                }   
            }

            ref< IRadSoundRsdFileDataSource > refIRsfds = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
            refIRsfds->InitializeFromFileName( 
                pFileName,
                0,
                IRadSoundHalAudioFormat::Milliseconds,
                NULL );

            #ifdef USE_BUFFERED_DATA_SOURCE
                pStream->m_refIRadSoundStreamPlayer->SetDataSource( NULL );
                pStream->m_refIRadSoundBufferedDataSource->SetInputDataSource( refIRsfds );
                pStream->m_refIRadSoundStreamPlayer->SetDataSource( pStream->m_refIRadSoundBufferedDataSource );
            #else
                pStream->m_refIRadSoundStreamPlayer->SetDataSource( refIRsfds );
            #endif
                pStream->m_refIRadSoundStreamPlayer->Play( );
        }

        /* for( unsigned int i = 0; i < NUM_STREAMS; i ++ )
        {
            IRadSoundStreamPlayer * pIRssp = g_Streams[ i ].m_refIRadSoundStreamPlayer;

            char buf[ 256];
            sprintf( buf, "Stream: [%d] State: [%d]\n", i, pIRssp->GetState( ) );
            refIRadTextDisplay->TextOutAt( buf, 0 , i);
        } */
    }

    ::rstRadSoundTerminate( );
    ::rstRadCoreTerminate( );
}
