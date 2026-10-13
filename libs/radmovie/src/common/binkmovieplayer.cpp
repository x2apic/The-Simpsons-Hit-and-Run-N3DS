//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        binkmovieplayer.cpp
// Subsystem:	Foundation Technologies - Movie
//
// Description:	This is the Bink movie player.  It's good for pc, xbox, & gcn
//
// Date:    	August 20, 2002 RWS
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include "binkmovieplayer.hpp"
#include <radoptions.hpp>
#include <radmem.h>

#ifdef RAD_MOVIEPLAYER_USE_BINK

//
// We've only tested Bink version 2022.05
//
#if ( !defined BinkMajorVersion ) || ( BinkMajorVersion != 2022  ) || \
    ( !defined BinkMinorVersion ) || ( BinkMinorVersion != 5  )
    #error "Only Bink 2022.05 is supported."
#endif

#include <radsoundmath.hpp>
#include <radprofiler.hpp>
#include "binkradfile.hpp"

//=============================================================================
// Global variables
//=============================================================================

unsigned int const radMovie_NoAudioTrack = 0xFFFFFFFF;
static bool g_MovieInitialized = false;
static radMemoryAllocator g_CurrentMovieAllocator = RADMEMORY_ALLOC_DEFAULT;
static void* s_BinkAramBuffer = NULL;
static unsigned int s_AllocCount = 0;

#ifdef RAD_GAMECUBE
    #define BINK_ARAM_SIZE 0x0025800
#endif
//=============================================================================
// Local Functions
//=============================================================================

//
// These are functions used by all platforms to allocate the memory
// needed by bink.
//

//=============================================================================
// ::BinkAllocate
//=============================================================================

#ifndef RAD_GAMECUBE
static void * RADLINK BinkAllocate( U64 bytes )
#else
static void * RADLINK BinkAllocate( unsigned long bytes )
#endif
{
    ::radMemorySetAllocationName( "BinkAllocate" );
    return ::radMemoryAllocAligned( g_CurrentMovieAllocator, (unsigned int)bytes, radFileOptimalMemoryAlignment );   
}

//=============================================================================
// ::BinkFree
//=============================================================================

#ifndef RAD_GAMECUBE
static void RADLINK BinkFree( void * pBuffer )
#else
static void RADLINK BinkFree( void * pBuffer )
#endif
{
    ::radMemoryFreeAligned( pBuffer );
}

//
// These are gamecube only functions that are required by Bink to allocated
// memory from ARAM
// 

#ifdef RAD_GAMECUBE

//=============================================================================
// ::BinkAramAllocate
//=============================================================================

static void * BinkAramAllocate( unsigned long bytes )
{
    rTuneAssert( bytes <= BINK_ARAM_SIZE );

    s_AllocCount++;
    if ( s_AllocCount == 1 )
    {
        return s_BinkAramBuffer;
    }
    else
    {
        return NULL;
    }
}

//=============================================================================
// ::BinkAramFree
//=============================================================================

static void BinkAramFree( void * pBuffer )
{
    rAssert( pBuffer == s_BinkAramBuffer );
    s_AllocCount--;
}

#endif // RAD_GAMECUBE

//=============================================================================
// Static initialization
//=============================================================================

template<> radMoviePlayerBink * radLinkedClass< radMoviePlayerBink >::s_pLinkedClassHead = NULL;
template<> radMoviePlayerBink * radLinkedClass< radMoviePlayerBink >::s_pLinkedClassTail = NULL;

//=============================================================================
// radMoviePlayerBink::radMoviePlayerBink
//=============================================================================

radMoviePlayerBink::radMoviePlayerBink( void )
    :
    radRefCount( 0 ),
    m_refIRadMovieRenderLoop( NULL ),
    m_refIRadMovieRenderStrategy( NULL ),
    m_State( NoData ),
    m_BinkHandle( NULL ),
    m_Volume( 1.0f ),
    m_Pan( 0.0f ),
    m_AudioTrackIndex( 0 ),
    m_IsPresentationOutstanding( false ),
    m_CheckAudio( false )
{
    rAssert( g_MovieInitialized == true );
}

//=============================================================================
// radMoviePlayerBink::~radMoviePlayerBink
//=============================================================================

radMoviePlayerBink::~radMoviePlayerBink( void )
{
    // Just to be sure

    Unload( );
}

//=============================================================================
// radMoviePlayerBink::Initialize
//=============================================================================

void radMoviePlayerBink::Initialize( IRadMovieRenderLoop * pIRadMovieRenderLoop, IRadMovieRenderStrategy * pIRadMovieRenderStrategy )
{
    rAssert( m_refIRadMovieRenderLoop == NULL );
    rAssert( m_refIRadMovieRenderStrategy == NULL );

    m_refIRadMovieRenderLoop = pIRadMovieRenderLoop;
    m_refIRadMovieRenderStrategy = pIRadMovieRenderStrategy;
}

//=============================================================================
// radMoviePlayerBink::Render
//=============================================================================

bool radMoviePlayerBink::Render( void )
{
    rAssert( m_refIRadMovieRenderStrategy != NULL );
    m_IsPresentationOutstanding = false;

    return m_refIRadMovieRenderStrategy->Render( );
}

//=============================================================================
// radMoviePlayerBink::Load
//=============================================================================

void radMoviePlayerBink::Load( const char * pVideoFileName, unsigned int audioTrackIndex )
{
    rAssert( m_refIRadMovieRenderStrategy != NULL );
   
    m_IsPresentationOutstanding = false;

    //
    // We first tell Bink which soundtrack it should play
    //

    m_AudioTrackIndex = audioTrackIndex;

    if( audioTrackIndex != radMovie_NoAudioTrack )
    {
        // Override bink's allocation system

        ::RADSetMemory( BinkAllocate, BinkFree );

        // On the gcn we need to tell bink where they can put the sound 

        #ifdef RAD_GAMECUBE
        ::RADSetAudioMemory( BinkAramAllocate, BinkAramFree );
        #endif // RAD_GAMECUBE

        // Use direct sound on win32

        #ifdef RAD_WIN32
        #ifdef RAD_PC
        ::BinkSoundUseXAudio2WinDevice( NULL );
        #else
        ::BinkSoundUseOpenAL( 48000, 2 );
        #endif
        #endif // RAD_WIN32

        // We tell bink to play the one sound track
        // specified in the audioTrackIndex array

        ::BinkSetSoundTrack( 1, & m_AudioTrackIndex );
    }
    else
    {
        // We tell bink not to play any sound track at all

        ::BinkSetSoundTrack( 0, NULL );
    }

    //
    // Now tell bink to open the movie.  We specify that it
    // should use the sound track that we declared above.
    //
    // Also... Bink doesn't give use a way to identify which movie 
    // is asking for memory at what time.  This is kind of a lame 
    // way to work around the problem, but I'll assume that I
    // can set a global to store the current allocator.  All allocations
    // should happen here?
    //

    g_CurrentMovieAllocator = GetThisAllocator( );
    ::BinkSetIO( radBinkFileOpen );
    m_BinkHandle = ::BinkOpen( pVideoFileName, BINKSNDTRACK | BINKIOPROCESSOR | BINKNOTHREADEDIO | BINKNOFILLIOBUF );

    if ( m_BinkHandle != NULL )
    {
        //
        // Just in case, we'll make sure that the movie is paused
        //

        ::BinkPause( m_BinkHandle, true );

        // The old movie player overrode the default speaker
        // settings so I will too

        #ifdef RAD_XBOX

        unsigned long mixBins[2];
        mixBins[0] = DSMIXBIN_FRONT_LEFT;
        mixBins[1] = DSMIXBIN_FRONT_RIGHT;

        ::BinkSetMixBins( m_BinkHandle, m_AudioTrackIndex, mixBins, 2 );

        #endif // RAD_XBOX
    }
    else
    {
        rTunePrintf( "ERROR radMoviePlayerBink [%s] BinkOpen: %s\n", pVideoFileName, BinkGetError( ) );
        SetState( IRadMoviePlayer2::NoData );
        return;
    }

    //
    // Make sure that the audio will play back at the appropriate levels
    //

    float volume = GetVolume( );
    float pan = GetPan( );
    m_Volume = 1.0f;
    m_Pan = 0.0f;
    m_CheckAudio = false;
    SetVolume( volume );
    SetPan( pan );
    m_CheckAudio = true;

    //
    // Prepare the render strategy
    //

    m_refIRadMovieRenderStrategy->ChangeParameters( m_BinkHandle->Width, m_BinkHandle->Height );

    //
    // We're ready to play
    // 

    SetState( IRadMoviePlayer2::ReadyToPlay );
}

//=============================================================================
// radMoviePlayerBink::Unload
//=============================================================================

void radMoviePlayerBink::Unload( void )
{
    if( m_State != IRadMoviePlayer2::NoData )
    {
        rAssert( m_BinkHandle != NULL );

        // I'll print out a bink summary and then close the movie

        #ifndef RAD_RELEASE

        BINKSUMMARY sum;
        ::BinkGetSummary( m_BinkHandle, & sum );

        rTunePrintf( 
            "//===================================//\n"
            "//        RADMOVIE BINK SUMMARY      //\n"
            "//===================================//\n"
            "//                Width [%10d] //\n"
            "//               Height [%10d] //\n"
            "//            TotalTime [%10d] //\n"
            "//        FileFrameRate [%10f] //\n"
            "//            FrameRate [%10f] //\n"
            "//        TotalOpenTime [%10d] //\n"
            "//          TotalFrames [%10d] //\n"
            "//    TotalPlayedFrames [%10d] //\n"
            "//        SkippedFrames [%10d] //\n"
            "//         SkippedBlits [%10d] //\n"
            "//           SoundSkips [%10d] //\n"
            "//        TotalBlitTime [%10d] //\n"
            "//        TotalReadTime [%10d] //\n",
            sum.Width, sum.Height, sum.TotalTime, sum.FileFrameRate / ( float ) sum.FileFrameRateDiv,
            sum.FrameRate / ( float ) sum.FrameRateDiv, sum.TotalOpenTime, sum.TotalFrames, sum.TotalPlayedFrames,
            sum.SkippedFrames, sum.SkippedBlits, sum.SoundSkips, sum.TotalBlitTime, sum.TotalReadTime );

        rTunePrintf(
            "// TotalVideoDecompTime [%10d] //\n"
            "// TotalAudioDecompTime [%10d] //\n"
            "//    TotalIdleReadTime [%10d] //\n"
            "//    TotalBackReadTime [%10d] //\n"
            "//       TotalReadSpeed [%10d] //\n"
            "//     SlowestFrameTime [%10d] //\n"
            "//    Slowest2FrameTime [%10d] //\n"
            "//      SlowestFrameNum [%10d] //\n"
            "//     Slowest2FrameNum [%10d] //\n"
            "//      AverageDataRate [%10d] //\n"
            "//     AverageFrameSize [%10d] //\n"
            "//     HighestMemAmount [%10d] //\n"
            "//        TotalIOMemory [%10d] //\n"
            "//        HighestIOUsed [%10d] //\n"
            "//      Highest1SecRate [%10d] //\n"
            "//     Highest1SecFrame [%10d] //\n"
            "//===================================//\n",
            sum.TotalVideoDecompTime, sum.TotalAudioDecompTime, sum.TotalIdleReadTime, sum.TotalBackReadTime,
            sum.TotalReadSpeed, sum.SlowestFrameTime, sum.Slowest2FrameTime, sum.SlowestFrameNum,
            sum.Slowest2FrameNum, sum.AverageDataRate, sum.AverageFrameSize, sum.HighestMemAmount,
            sum.TotalIOMemory, sum.HighestIOUsed, sum.Highest1SecRate, sum.Highest1SecFrame );

        #endif

        ::BinkClose( m_BinkHandle );
        m_BinkHandle = NULL;

        SetState( IRadMoviePlayer2::NoData );
    }
}

//=============================================================================
// radMoviePlayerBink::Play
//=============================================================================

void radMoviePlayerBink::Play( void )
{
    if( m_State == IRadMoviePlayer2::ReadyToPlay )
    {
        rAssert( m_BinkHandle != NULL );

        //
        // Tell Bink to Play the movie
        //

        ::BinkPause( m_BinkHandle, false );

        SetState( IRadMoviePlayer2::Playing );
    }
    else if( m_State == IRadMoviePlayer2::Loading )
    {
        rAssert( false );  // for now
        SetState( IRadMoviePlayer2::LoadToPlay );
    }
}

//=============================================================================
// radMoviePlayerBink::Pause
//=============================================================================

void radMoviePlayerBink::Pause( void )
{
    if( m_State == IRadMoviePlayer2::Playing )
    {
        rAssert( m_BinkHandle != NULL );

        //
        // Tell Bink to Pause the movie
        //

        ::BinkPause( m_BinkHandle, true );

        SetState( IRadMoviePlayer2::ReadyToPlay );
    }
    else if( m_State == IRadMoviePlayer2::LoadToPlay )
    {
        rAssert( false );  // not possible yet
        SetState( Loading );
    }
}

//=============================================================================
// radMoviePlayerBink::SetPan
//=============================================================================

void  radMoviePlayerBink::SetPan( float pan )
{
    rAssert( pan >= -1.0f && pan <= 1.0f );

    if( m_Pan != pan )
    {
        // Remember this new pan

        m_Pan = pan;

        if( m_BinkHandle != NULL )
        {
            #ifdef RAD_XBOX

                // Turn analog into left and right using radsound

                float panL = 1.0f;
                float panR = 1.0f;

                if( pan > 0 )
                {
                    panL = 1.0f - 1.0f * pan;
                }
                else
                {
                    panR = 1.0f + 1.0f * pan;
                }

                panL = ::radSoundVolumeAnalogToAmplitude( panL );
                panR = ::radSoundVolumeAnalogToAmplitude( panR );

                unsigned long mixbins[ 2 ];
                mixbins[ 0 ] = DSMIXBIN_FRONT_LEFT;
                mixbins[ 1 ] = DSMIXBIN_FRONT_RIGHT;

                // Full volume for bink bins is 32768

                long volumes[ 2 ];
                volumes[ 0 ] = static_cast< long >( 32768 * panL );
                volumes[ 1 ] = static_cast< long >( 32768 * panR );

                //
                // In Bink 1.5s, there's an assert if the volume is between 1 and 32.
                //
                #if ( defined BINKSUBVERSION ) && ( BINKSUBVERSION == 19 )
                    if ( volumes[ 0 ] <= 32 && volumes[ 0 ] > 0 )
                    {
                        volumes[ 0 ] = 33;
                    }

                    if ( volumes[ 1 ] <= 32 && volumes[ 1 ] > 0 )
                    {
                        volumes[ 1 ] = 33;
                    }
                #endif

                ::BinkSetMixBinVolumes( m_BinkHandle, m_AudioTrackIndex, mixbins, volumes, 2 );

            #else

                // Pc and gamecube can set pan directly.
                //     0 is left
                // 32768 is centre
                // 65536 is right

                float binkPan = 65536.0f * ( pan + 1.0f ) / 2.0f;

                ::BinkSetPan( m_BinkHandle, m_AudioTrackIndex, ( long ) binkPan );        

            #endif // RAD_XBOX

        }
    }
}

//=============================================================================
// radMoviePlayerBink::GetPan
//=============================================================================

float radMoviePlayerBink::GetPan( void )
{
    return m_Pan;
}

//=============================================================================
// radMoviePlayerBink::SetVolume
//=============================================================================

void  radMoviePlayerBink::SetVolume( float volume )
{
    if( m_Volume != volume )
    {
        ::radSoundVerifyChangeThreshold( m_CheckAudio, "Volume", m_Volume, volume, radSoundVolumeChangeThreshold );

        // Remember this new volume

        m_Volume = volume;

	    if ( m_BinkHandle != NULL )
	    {
		    //
		    // Scale new Volume to 0-32768 value, compatible with Bink
		    //

		    int binkVol =
			    static_cast< unsigned int >(
				    radSoundVolumeAnalogToAmplitude( m_Volume ) * 32768.0f );

            //
            // Set the volume of the appropriate audio track
            //

		    BinkSetVolume( m_BinkHandle, m_AudioTrackIndex, binkVol );
	    }
    }
}

//=============================================================================
// radMoviePlayerBink::GetVolume
//=============================================================================

float radMoviePlayerBink::GetVolume( void )
{
    return m_Volume;
}

//=============================================================================
// radMoviePlayerBink::GetState
//=============================================================================

IRadMoviePlayer2::State radMoviePlayerBink::GetState( void )
{
    return m_State;
}

//=============================================================================
// radMoviePlayerBink::GetVideoFrameInfo
//=============================================================================

bool radMoviePlayerBink::GetVideoFrameInfo(VideoFrameInfo * pFrameInfo)
{
    rAssert( pFrameInfo != NULL );

    if( m_BinkHandle != NULL )
    {
        pFrameInfo->Height = m_BinkHandle->Width;
        pFrameInfo->Width  = m_BinkHandle->Height;
        return true;
    }
    else
    {
        return false;
    }
}

//=============================================================================
// radMoviePlayerBink::GetFrameRate
//=============================================================================

float radMoviePlayerBink::GetFrameRate( void )
{
    if( m_BinkHandle != NULL )
    {
        return( ( float ) m_BinkHandle->FrameRate / m_BinkHandle->FrameRateDiv );
    }
    else
    {
        return 0.0f;
    }
}

//=============================================================================
// radMoviePlayerBink::GetCurrentFrameNumber
//=============================================================================

unsigned int radMoviePlayerBink::GetCurrentFrameNumber( void )
{
    if( m_BinkHandle != NULL )
    {
        return m_BinkHandle->FrameNum;
    }
    else
    {
        return 0;
    }
}

//=============================================================================
// radMoviePlayerBink::Service
//=============================================================================

void radMoviePlayerBink::Service( void )
{
    State state = m_State;

    switch( state )
    {
    case NoData:
        break;
    case Loading:
        break;
    case LoadToPlay:
        break;
    case ReadyToPlay:
        break;
        case Playing:
        {
            rAssert( m_refIRadMovieRenderStrategy != NULL );
            rAssert( m_BinkHandle != NULL );

            //
            // Poll the BinkWait function until it's time to
            // render the next frame
            //

            if( BinkWait( m_BinkHandle ) == 0 )
            {
                //
                // Decompress the next frame
                //

                bool iterateLoop = false;

                if( m_IsPresentationOutstanding == false && ::BinkDoFrame( m_BinkHandle ) == 0 )
                {

                    // Reset the list of render destinations

                    m_refIRadMovieRenderStrategy->ResetDestinations( );

                    // Now go through the list destinations until we've filled 
                    // them all up with the freshly decoded data

                    IRadMovieRenderStrategy::LockedDestination dest;

                    while( m_refIRadMovieRenderStrategy->LockNextDestination( & dest ) > 0 )
                    {
                        // If one of these copies fail, we'll have to skip this frame
                        // and not iterate the loop

                        // BinkCopyToBufferRect handles pitch differently than pure3d and 
                        // always accesses buffers from the top.  So...

                        void * pDest = dest.m_pDest;

                        if( dest.m_DestPitch < 0 )
                        {
                            pDest = ( char * ) dest.m_pDest + S64( dest.m_Height - 1 ) * dest.m_DestPitch;
                        }

                        if( 0 == ::BinkCopyToBufferRect( m_BinkHandle, pDest,
                            dest.m_DestPitch, dest.m_Height, 0, 0, 
                            dest.m_SrcPosX, dest.m_SrcPosY, dest.m_Width, dest.m_Height, 0 ) )
                        {
                            iterateLoop = true;
                        }

                        m_refIRadMovieRenderStrategy->UnlockDestination( );
                    }

                    if( iterateLoop == false )
                    {
                        rTunePrintf( "radMoviePlayerBink: BinkCopyToBuffer skipped frame\n" );
                    }
                }
                else
                {
                    // The next frame was skipped.  Don't render.

                    rTunePrintf( "radMoviePlayerBink: BinkDoFrame skipped frame\n!" );
                }

                //
                // BinkNextFrame can't be called for the last frame in the
                // movie.  It needs to be called for all others though.
                //

		        if ( m_BinkHandle->FrameNum == ( m_BinkHandle->Frames - 1 ) )
		        {
			        //
			        // We're done
			        //
                    Unload( );
		        }
		        else
		        {
                    ::BinkNextFrame( m_BinkHandle );

                    if( iterateLoop == true )
                    {
                        m_IsPresentationOutstanding = true;

                        if( m_refIRadMovieRenderLoop != NULL )
                        {
                            m_refIRadMovieRenderLoop->IterateLoop( this );
                        }
                    }
                }
            }
            break;
        }
    default:
        rAssert( false );
        return;
    }
}

//=============================================================================
// radMoviePlayerBink::SetState
//=============================================================================

void radMoviePlayerBink::SetState( IRadMoviePlayer2::State state )
{
    m_State = state;
}

//=============================================================================
// radMoviePlayerCreate2
//=============================================================================

IRadMoviePlayer2 * radMoviePlayerCreate2( radMemoryAllocator alloc )
{
    return new( alloc )radMoviePlayerBink( );
}

//=============================================================================
// radMovieInitialize2
//=============================================================================

void radMovieInitialize2( radMemoryAllocator alloc )
{
    // I know that these functions aren't doing any work 
    // here.  But I want the interface behave the same
    // on all platforms

    rAssert( g_MovieInitialized == false );
    radBinkFileInitialize( alloc );

#ifdef RAD_GAMECUBE
    ::radMemorySetAllocationName( "BinkAramAllocate" );
    s_BinkAramBuffer = ::radMemorySpaceAlloc( radMemorySpace_Aram, RADMEMORY_ALLOC_DEFAULT, BINK_ARAM_SIZE );
#endif

    g_MovieInitialized = true;
}

//=============================================================================
// radMovieTerminate2
//=============================================================================

void radMovieTerminate2( void )
{
    // I know that these functions aren't doing any work 
    // here.  But I want the interface behave the same
    // on all platforms

    rAssert( g_MovieInitialized == true );
#ifdef RAD_GAMECUBE
    ::radMemorySpaceFree( radMemorySpace_Aram, radMemorySpace_Aram, s_BinkAramBuffer );
#endif
    radBinkFileTerminate( );
    g_MovieInitialized = false;
}

//=============================================================================
// radMovieService2
//=============================================================================

void radMovieService2( void )
{
    ref< radMoviePlayerBink > pNext = radMoviePlayerBink::GetLinkedClassHead( );

    while( pNext != NULL )
    {
        pNext->Service( );
        pNext = pNext->GetLinkedClassNext( );
    }
}


#endif // RAD_MOVIEPLAYER_USE_BINK
