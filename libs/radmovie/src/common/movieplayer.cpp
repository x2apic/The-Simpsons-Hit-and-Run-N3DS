//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        movieplayer.cpp
//
// Subsystem:	Foundation Technologies - Movie
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies movie player
//
// Date:    	May 16, 2002
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <radoptions.hpp>

#ifndef RAD_MOVIEPLAYER_USE_BINK

#include <raddebug.hpp>
#include <radmath/radmath.hpp>
#include "radmoviefile.hpp"
#include "movieplayer.hpp"
#include "audiodatasource.hpp"

//=============================================================================
// Constants
//=============================================================================

const char * radMovieDebugChannel2 = "radMovie";
const unsigned int radMovie_NoAudioTrack = 0xFFFFFFFF;

//=============================================================================
// Local Definitions
//=============================================================================

//
// Current implementation only plays movies of 640 * 480. May change in future. For
// PAL, we use  640 * 528
//
#ifndef PAL
#define IMAGE_WIDTH     640
#define IMAGE_HEIGHT    480    
#define VSYNC_PERIOD    16 
#else
#define IMAGE_WIDTH     640
#define IMAGE_HEIGHT    528    
#define VSYNC_PERIOD    20
#endif

//
// These are passed around as state identifiers in radFile callbacks
//

#define RAD_MOVIE_PLAYER_FIRST_ACCESS 0xffff0000
#define RAD_MOVIE_PLAYER_HEADER_READ  0xffff0002

//
// If the video of the movie player starts falling behind the audio,
// the movie player will not render frames until it catches up.  This
// value controls how many frames the video can fall behind before
// the catch-up process begins
//

#define RAD_MOVIE_PLAYER_VIDEO_LAG 0

//=============================================================================
// Static Members
//=============================================================================

radMoviePlayer * radLinkedClass< radMoviePlayer >::s_pLinkedClassHead = NULL;
radMoviePlayer * radLinkedClass< radMoviePlayer >::s_pLinkedClassTail = NULL;

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// radMoviePlayer::radMoviePlayer
//=============================================================================

radMoviePlayer::radMoviePlayer( void )
    :
    radRefCount( 0 ),
    m_refIRadSoundBufferedDataSource( NULL ),
    m_refIRadMovieRenderStrategy( NULL ),
    m_refIRadMovieVideoDecoder( NULL ),
    m_refIRadSoundStreamPlayer( NULL ),
    m_refIRadMovieRenderLoop( NULL ),
    m_refIRadStopwatch( NULL ),
    m_refIRadFile_Current( NULL ),
    m_refIRadString_PendingFileName( NULL ),
    m_State( IRadMoviePlayer2::NoData ),
    m_VideoFrameState( VideoFrame_Unlocked),
    m_AudioTrackIndex( 0 ), 
    m_PresentationTime( 0 ),
    m_SkippingFrames( false ),
    m_pVideoFrame( NULL ),
    m_pTempHeaderBuffer( NULL )
{
    radTimeCreateStopwatch( &m_refIRadStopwatch, radTimeUnit_Millisecond, GetThisAllocator( ) );

    for( unsigned int i = 0; i < 8; i++ )
    {
        m_pSoundClockFixupTaps[ i ] = 1.0f;
    }

    ::memset( & m_VideoFrameInfo, 0, sizeof( VideoFrameInfo ) );
}

//=============================================================================
// radMoviePlayer::~radMoviePlayer
//=============================================================================

radMoviePlayer::~radMoviePlayer( void )
{
    if( m_pTempHeaderBuffer != NULL )
    {
        ::radMemoryFreeAligned( m_pTempHeaderBuffer );
        m_pTempHeaderBuffer = NULL;
    }
}

//=============================================================================
// radMoviePlayer::Initialize
//=============================================================================

void radMoviePlayer::Initialize
( 
    IRadMovieRenderLoop * pIRadMovieRenderLoop, 
    IRadMovieRenderStrategy * pIRadMovieRenderStrategy,
    unsigned int maxWidthPixels, 
    unsigned int maxHeightPixels, unsigned int codedVideoBufferBytes, 
    unsigned int audioPrimaryBufferSize, unsigned int audioSecondaryBufferSize,
    IRadSoundHalAudioFormat::SizeType audioSizeType
)
{
    rAssert( pIRadMovieRenderStrategy != NULL );
    rAssert( m_refIRadMovieVideoDecoder == NULL );
    rAssert( m_refIRadSoundStreamPlayer == NULL );
    rAssert( m_refIRadSoundBufferedDataSource == NULL );

    // Hang on to the loop and strategy

    m_refIRadMovieRenderLoop = pIRadMovieRenderLoop;
    m_refIRadMovieRenderStrategy = pIRadMovieRenderStrategy;

    //
    // Initialize the video
    //

    m_refIRadMovieVideoDecoder = ::radMovieVideoDecoderCreate( GetThisAllocator( ) );
    m_refIRadMovieVideoDecoder->Initialize( maxWidthPixels, maxHeightPixels, codedVideoBufferBytes );

    //
    // Initialize the audio 
    //

    m_refIRadSoundBufferedDataSource = ::radSoundBufferedDataSourceCreate( GetThisAllocator( ) );
    m_refIRadSoundBufferedDataSource->Initialize( radMemorySpace_Local, ::radMemoryGetAllocator( GetThisAllocator( ) ), 
        audioPrimaryBufferSize, audioSizeType, NULL, "radMoviePlayer::m_refIRadSoundBufferedDataSource" );

    m_refIRadSoundStreamPlayer = ::radSoundStreamPlayerCreate( GetThisAllocator( ) );
    m_refIRadSoundStreamPlayer->InitializeAsync( audioSecondaryBufferSize, audioSizeType, 
        ::radSoundHalSystemGet( )->GetRootMemoryRegion( ), "radMoviePlayer::m_refIRadSoundStreamPlayer" );

    //
    // Print out some helpful information
    //
    rDebugPrintf( "\nradMoviePlayer Audio Allocations (In radsound units) \n" \
                  "     * PrimaryBuffer      [0x%x] unit[%d]\n" \
                  "     * SeocondaryBuffer   [0x%x] unit[%d]\n\n",
        audioPrimaryBufferSize, audioSizeType,
        audioSecondaryBufferSize, audioSizeType );

    //
    // Update the state
    //

    SetState( IRadMoviePlayer2::NoData );
}

//=============================================================================
// radMoviePlayer::Render
//=============================================================================

bool radMoviePlayer::Render( void )
{
    rAssert( m_refIRadMovieRenderStrategy != NULL );
    rAssert( m_pVideoFrame != NULL );
        
    if( m_pVideoFrame != NULL )
    {
        bool ret = m_refIRadMovieRenderStrategy->Render( m_pVideoFrame );

        //
        // The video frame has been used.  The next can be decoded
        //

        m_VideoFrameState = VideoFrame_Unlocked;

        m_refIRadMovieVideoDecoder->UnlockVideoFrame( );

        m_pVideoFrame = NULL;


        return ret;
    }
    else
    {
        rAssert( false );
        return false;
    }
}

//=============================================================================
// radMoviePlayer::Load
//=============================================================================

void radMoviePlayer::Load( const char * pVideoFileName, unsigned int audioTrackIndex )
{   
    rAssert( m_State == IRadMoviePlayer2::NoData );
    rAssert( pVideoFileName != NULL );

    m_refIRadStopwatch->Stop( );
    m_refIRadStopwatch->Reset( );

    //
    // Reset the variables
    //

    m_VideoFrameState = VideoFrame_Unlocked;
    m_AudioTrackIndex = audioTrackIndex;
    m_SkippingFrames = false;

    //
    // Remember the name of the new file.  We'll open it later at the correct moment
    //

    ::radStringCreate( & m_refIRadString_PendingFileName, RADMEMORY_ALLOC_TEMP );
    m_refIRadString_PendingFileName->Copy( pVideoFileName );

    // The buffered data source's input must be set before the stream player's

    SetState( IRadMoviePlayer2::Loading );

    Service( );
}

//=============================================================================
// radMoviePlayer::Unload
//=============================================================================

void radMoviePlayer::Unload( void )
{
    if( m_State != IRadMoviePlayer2::NoData )
    {
        m_refIRadString_PendingFileName = NULL;
        m_refIRadSoundStreamPlayer->Stop( );
        m_refIRadSoundStreamPlayer->SetDataSource( NULL );   
        m_refIRadSoundBufferedDataSource->SetInputDataSource( NULL );
        m_refIRadStopwatch->Stop( );
        m_refIRadStopwatch->Reset( );
        m_refIRadMovieVideoDecoder->Reset( );
        m_VideoFrameState = VideoFrame_Unlocked;

        if( m_refIRadFile_Current != NULL )
        {
            rAssert( m_refIRadFile_Stale == NULL );

            //
            // If we have a file currently open, it will have pending requests.
            // Hang on to the stale file until it's done
            //

            m_refIRadFile_Stale = m_refIRadFile_Current;
            m_refIRadFile_Stale->CancelAsync( );
        }

        m_refIRadFile_Current = NULL;

        SetState( IRadMoviePlayer2::NoData );
    }
}

//=============================================================================
// radMoviePlayer::Play
//=============================================================================

void radMoviePlayer::Play( void )
{
    if( m_State == IRadMoviePlayer2::ReadyToPlay )
    {
        SetState( IRadMoviePlayer2::Playing );
        InternalPlay( );
    }
    else if( m_State == IRadMoviePlayer2::Loading )
    {
        SetState( IRadMoviePlayer2::LoadToPlay );
    }
}

//=============================================================================
// radMoviePlayer::Pause
//=============================================================================

void radMoviePlayer::Pause( void )
{
    if( m_State == IRadMoviePlayer2::Playing )
    {
        m_refIRadSoundStreamPlayer->Stop( );
        m_refIRadStopwatch->Stop( );
        SetState( IRadMoviePlayer2::ReadyToPlay );
    }
    else if( m_State == IRadMoviePlayer2::LoadToPlay )
    {
        SetState( Loading );
    }
}

//=============================================================================
// radMoviePlayer::SetPan
//=============================================================================

void radMoviePlayer::SetPan( float pan )
{
    m_refIRadSoundStreamPlayer->SetPan( pan );
}

//=============================================================================
// radMoviePlayer::GetPan
//=============================================================================

float radMoviePlayer::GetPan( void )
{
    return m_refIRadSoundStreamPlayer->GetPan( );
}

//=============================================================================
// radMoviePlayer::SetVolume
//=============================================================================

void radMoviePlayer::SetVolume( float volume )
{
    m_refIRadSoundStreamPlayer->SetVolume( volume );
}

//=============================================================================
// radMoviePlayer::GetVolume
//=============================================================================

float radMoviePlayer::GetVolume( void )
{
    return m_refIRadSoundStreamPlayer->GetVolume( );
}

//=============================================================================
// radMoviePlayer::GetState
//=============================================================================

IRadMoviePlayer2::State radMoviePlayer::GetState( void )
{
    return m_State;
}

//=============================================================================
// radMoviePlayer::GetVideoFrameInfo
//=============================================================================

bool radMoviePlayer::GetVideoFrameInfo( VideoFrameInfo * frameInfo)
{
    rAssert( frameInfo != NULL );

    if( m_State == IRadMoviePlayer2::Playing ||
        m_State == IRadMoviePlayer2::ReadyToPlay )
    {
        ( * frameInfo ).Width = m_VideoFrameInfo.Width;
        ( * frameInfo ).Height = m_VideoFrameInfo.Height;
        return true;
    }
    else
    {
        return false;
    }
}

//=============================================================================
// radMoviePlayer::GetFrameRate
//=============================================================================

float radMoviePlayer::GetFrameRate( void )
{
    rAssert( m_refIRadMovieVideoDecoder != NULL );

    return ( float ) m_refIRadMovieVideoDecoder->GetFrameRate( );
}

//=============================================================================
// radMoviePlayer::GetCurrentFrameNumber
//=============================================================================

unsigned int radMoviePlayer::GetCurrentFrameNumber( void )
{
    return m_refIRadMovieVideoDecoder->GetCurrentFrameNumber( );
}

//=============================================================================
// radMoviePlayer::OnFileOperationsComplete
//=============================================================================

void radMoviePlayer::OnFileOperationsComplete( void * pUserData )
{
    if( m_refIRadFile_Stale != NULL )
    {
        rAssert( m_refIRadFile_Current == NULL );

        //
        // The stale file will have 1 outstanding request.  When 
        // the request comes in, we are done with it.
        //

        m_refIRadFile_Stale = NULL;

        if( ( unsigned int ) pUserData == RAD_MOVIE_PLAYER_HEADER_READ )
        {
            rAssert( m_pTempHeaderBuffer != NULL );

            //
            // This outstanding request was for the header to be read. That
            // means we need to free the header memory.
            //

            ::radMemoryFreeAligned( m_pTempHeaderBuffer );
            m_pTempHeaderBuffer = NULL;
        }
    }
    else if( ( unsigned int ) pUserData == RAD_MOVIE_PLAYER_FIRST_ACCESS )
    {
        if( m_refIRadFile_Current->IsOpen( ) )
        {
            //
            // Since the file is open, we can read the header
            //

            m_pTempHeaderBuffer = ::radMemoryAllocAligned( RADMEMORY_ALLOC_TEMP, RAD_MOVIE_FILE_DATA_POSITION, radFileOptimalMemoryAlignment );
            ::memset( m_pTempHeaderBuffer, 0xBF, RAD_MOVIE_FILE_DATA_POSITION );
        
            m_refIRadFile_Current->ReadAsync( m_pTempHeaderBuffer, RAD_MOVIE_FILE_DATA_POSITION );
            m_refIRadFile_Current->SetPositionAsync( 0 );
            m_refIRadFile_Current->AddCompletionCallback( this, ( void * ) RAD_MOVIE_PLAYER_HEADER_READ );
        }
        else
        {
            m_refIRadFile_Current = NULL;
        }
    }
    else if( ( unsigned int ) pUserData == RAD_MOVIE_PLAYER_HEADER_READ )
    {
        rAssert( m_pTempHeaderBuffer != NULL );

        //
        // We've just read in the file header
        //

        RadMovieFileHeader * pRadMovieFileHeader = ( RadMovieFileHeader * ) m_pTempHeaderBuffer;

        // Try to detect some corruption

        #ifdef RAD_PS2

        rAssertMsg( ::memcmp( pRadMovieFileHeader->m_Id, "rmvps212", 8 ) == 0, "Invalid rmv version, current version is 2.1.2." );

        #endif // RAD_PS2

        #ifdef RAD_GAMECUBE

        rAssert( ::memcmp( pRadMovieFileHeader->m_Id, "rmvgcn10", 8 ) == 0 );

        #endif // RAD_GAMECUBE

        rAssert( pRadMovieFileHeader->m_DataSize == m_refIRadFile_Current->GetSize( ) );

        m_VideoFrameInfo.Width  = pRadMovieFileHeader->m_Width;
        m_VideoFrameInfo.Height = pRadMovieFileHeader->m_Height;

        //
        // Initialize the render strategy & and pass the file to
        // the decoder to deal with
        //

        m_refIRadMovieRenderStrategy->SetParameters( pRadMovieFileHeader->m_Width, pRadMovieFileHeader->m_Height );
        m_refIRadMovieVideoDecoder->SetSource( m_refIRadFile_Current, m_VideoFrameInfo.Width, m_VideoFrameInfo.Height, pRadMovieFileHeader->m_DataPosition, pRadMovieFileHeader->m_FrameRate );

        //
        // Set up the audio if required
        //

        if( m_AudioTrackIndex != radMovie_NoAudioTrack )
        {

            if( m_AudioTrackIndex < pRadMovieFileHeader->m_NumAudioTracks )
            {
                // Open another Instance of the file
            
                ref< IRadFile > refIRadFile = NULL;
                ::radFileOpen( & refIRadFile, m_refIRadFile_Current->GetFilename( ) );
                rAssert( refIRadFile != NULL );

                // Give the file to radsound

                ref< radMovieAudioDataSource > refSoundSource = ::radMovieAudioDataSourceCreate( GetThisAllocator( ) );
                refSoundSource->Initialize( refIRadFile, 
                    pRadMovieFileHeader->m_pAudioTrackOffsets[ m_AudioTrackIndex ], 
                    pRadMovieFileHeader->m_pAudioTrackSizes[ m_AudioTrackIndex ] );

                m_refIRadSoundBufferedDataSource->SetInputDataSource( refSoundSource );
                m_refIRadSoundStreamPlayer->SetDataSource( m_refIRadSoundBufferedDataSource );

            }
            else
            {
                // Assert !
                rAssert( m_AudioTrackIndex < pRadMovieFileHeader->m_NumAudioTracks );
            }
        }
        
        //
        // We're done with the file and header
        //
        ::radMemoryFreeAligned( m_pTempHeaderBuffer );
        m_pTempHeaderBuffer = NULL;
        m_refIRadFile_Current = NULL;
    }
    else
    {
        // What are we doing here?

        rAssert( false );
    }
}

//=============================================================================
// radMoviePlayer::Service
//=============================================================================

void radMoviePlayer::Service( void )
{
    //
    // Start off by letting the decoder figure out where it is
    //

    m_refIRadMovieVideoDecoder->Service( );

    //
    // Now figure out where we are.
    //

    if( m_State == IRadMoviePlayer2::NoData )
    {
        // Nothing to do
    }
    else if( m_State == IRadMoviePlayer2::Loading || m_State == IRadMoviePlayer2::LoadToPlay )
    {
        // Sanity checks
        
        rAssert( ( ! ( m_refIRadFile_Current == NULL && m_refIRadFile_Stale != NULL && m_refIRadString_PendingFileName == NULL ) ) );
        rAssert( ( ! ( m_refIRadFile_Current != NULL && m_refIRadFile_Stale == NULL && m_refIRadString_PendingFileName != NULL ) ) );
        rAssert( ( ! ( m_refIRadFile_Current != NULL && m_refIRadFile_Stale != NULL && m_refIRadString_PendingFileName == NULL ) ) );
        rAssert( ( ! ( m_refIRadFile_Current != NULL && m_refIRadFile_Stale != NULL && m_refIRadString_PendingFileName != NULL ) ) );

        //
        // The substate of the load is a controlled by the state of the temporary reference
        // to the name of the movie file and and the reference to the movie file.
        //

        if( m_refIRadFile_Stale != NULL )
        {
            rAssert( m_refIRadFile_Current == NULL );
            rAssert( m_refIRadString_PendingFileName != NULL );

            //
            // If the stale file is not null, we'll just have to wait for
            // it to finish its callbacks at which time it will become null
            //
        }
        else if( m_refIRadString_PendingFileName != NULL )
        {
            rAssert( m_refIRadFile_Stale == NULL );
            rAssert( m_refIRadFile_Current == NULL );

            //
            // If the pending file name is not null and there is
            // no stale file, we are safe to open another.
            //

            ::radFileOpen( & m_refIRadFile_Current, m_refIRadString_PendingFileName->GetChars( ) );
            rAssert( m_refIRadFile_Current != NULL );
            m_refIRadString_PendingFileName = NULL;

            //
            // Find out if the file is open by adding a callback.  This will initiate the reading
            // of the header in the OnFileOperationsComplete callback
            //
    
            m_refIRadFile_Current->AddCompletionCallback( this, ( void * ) RAD_MOVIE_PLAYER_FIRST_ACCESS );
        }
        else if( m_refIRadFile_Current != NULL )
        {
            rAssert( m_refIRadFile_Stale == NULL );
            rAssert( m_refIRadString_PendingFileName == NULL );

            // The current file will be set to null once the header has been read.
        }
        else
        {
            //
            // Now wait until the stream player and the video decoder have moved
            // into their ready states
            //

            if
            ( 
                m_refIRadMovieVideoDecoder->GetState( ) == IRadMovieVideoDecoder::Ready && 
                ( m_refIRadSoundStreamPlayer->GetState( ) == IRadSoundStreamPlayer::Paused || m_AudioTrackIndex == radMovie_NoAudioTrack )
            )
            {
                if( m_State == IRadMoviePlayer2::Loading )
                {
                    SetState( IRadMoviePlayer2::ReadyToPlay );
                }
                else
                {
                    SetState( IRadMoviePlayer2::Playing );
                    InternalPlay( );
                }
            }
        }
    }
    else if( m_State == IRadMoviePlayer2::Playing )
    {
        //
        // If the video frame is unlocked, we can issue the next decode request.
        //

        if( m_VideoFrameState == VideoFrame_Unlocked )
        {
            //
            // Keep an eye on the states of things to detect the end of the movie
            //

            IRadMovieVideoDecoder::State videoDecoderState = m_refIRadMovieVideoDecoder->GetState( );
            IRadSoundStreamPlayer::State streamPlayerState = m_refIRadSoundStreamPlayer->GetState( );
    
            if( ( m_AudioTrackIndex == radMovie_NoAudioTrack || streamPlayerState == IRadSoundStreamPlayer::Playing ) &&
                ( videoDecoderState == IRadMovieVideoDecoder::Ready || videoDecoderState == IRadMovieVideoDecoder::Decoding ) )

            {
                //
                // Try locking the next video frame
                //
                                    
                m_pVideoFrame = m_refIRadMovieVideoDecoder->GetLockedVideoFrame( & m_PresentationTime );

                if( m_pVideoFrame != NULL )
                {
                    m_VideoFrameState = VideoFrame_Locked;
                }
            }
            else
            {
                rAssert( streamPlayerState != IRadSoundStreamPlayer::Playing && m_AudioTrackIndex != radMovie_NoAudioTrack || 
                     videoDecoderState == IRadMovieVideoDecoder::NoData ||
                     videoDecoderState == IRadMovieVideoDecoder::Error );
                
                rDebugChannelPrintf( radMovieDebugChannel2, "radMoviePlayer: Out of data at [%dms] m_CurrentFrameNumber [%d]\n", GetCurrentTime( ) );

                // We've hit the end of the movie.  Unload!

                Unload( );
            }
        }

        if( m_VideoFrameState == VideoFrame_Locked )
        {
            //
            // We will render the frame at the appropriate time.  We know that the 
            // image will be presented on a vsync.  As long as the time between now
            // and the presentation time is less than a vsync period, we can begin
            // rendering
            //

            if( m_refIRadMovieRenderLoop != NULL )
            {
                // If the client gave us a render loop, initiate a render of the new frame.
                // It's okay for us to fall a little behind the presentation time.  We might
                // catch up without dropping frames.  If we do fall far enough behind, throw
                // away frames until we're all the way caught up

                unsigned int currentTime = GetCurrentTime( );

                if( ( m_SkippingFrames == false && 
                      currentTime + VSYNC_PERIOD > m_PresentationTime && 
                      currentTime < m_PresentationTime + RAD_MOVIE_PLAYER_VIDEO_LAG ) ||
                    ( m_SkippingFrames == true &&
                      currentTime + VSYNC_PERIOD > m_PresentationTime &&
                      currentTime <= m_PresentationTime ) )
                {
                    m_SkippingFrames = false;
                    m_refIRadMovieRenderLoop->IterateLoop( this );
                }
                else if( currentTime + VSYNC_PERIOD > m_PresentationTime )
                {
                    rTunePrintf( "radMoviePlayer: NOT RENDERING THIS FRAME (must catch up)\n" );
                    rTunePrintf( "current time[ %d ] expected presentation time[%d]\n", currentTime, m_PresentationTime );

                    //
                    // Not rendering will save us up to a vsync period this round.
                    // Eventually we're bound to catch up
                    //
                    
                    m_SkippingFrames = true;
                    m_VideoFrameState = VideoFrame_Unlocked;
                    m_refIRadMovieVideoDecoder->UnlockVideoFrame( );
                    m_pVideoFrame = NULL;
                }
            }
        }
    }
}

//=============================================================================
// radMoviePlayer::GetCurrentTime
//=============================================================================

unsigned int radMoviePlayer::GetCurrentTime( void )
{
    //
    // The audio clock can vary a little.  It may run a little slower
    // than real time.  It's values may jump around a touch.  We'll
    // smooth things out by keeping a running average.  This can be 
    // removed once sound starts providing better values.
    //

    if( m_AudioTrackIndex != radMovie_NoAudioTrack && m_refIRadSoundStreamPlayer->GetFormat( ) != NULL )
    {
        //
        // Slide the fixup factors over then grab the newest value
        //

        memcpy( m_pSoundClockFixupTaps, m_pSoundClockFixupTaps + 1, 7 );

        unsigned int samples = m_refIRadSoundStreamPlayer->GetPlaybackTimeInSamples( );

        if( samples > 0 )
        {
            m_pSoundClockFixupTaps[ 7 ]  = m_refIRadSoundStreamPlayer->GetFormat( )->SamplesToMilliseconds( samples ) / ( float ) m_refIRadStopwatch->GetElapsedTime( );
        }
    }

    //
    // Return the current time 
    //

    return rmt::FtoL( m_refIRadStopwatch->GetElapsedTime( ) * ( m_pSoundClockFixupTaps[ 0 ] + m_pSoundClockFixupTaps[ 1 ] + m_pSoundClockFixupTaps[ 2 ] + m_pSoundClockFixupTaps[ 3 ] + m_pSoundClockFixupTaps[ 4 ] + m_pSoundClockFixupTaps[ 5 ] + m_pSoundClockFixupTaps[ 6 ] + m_pSoundClockFixupTaps[ 7 ] )  ) / 8;
}

//=============================================================================
// radMoviePlayer::SetState
//=============================================================================

void radMoviePlayer::SetState( IRadMoviePlayer2::State state )
{
    if( state != m_State )
    {
        rDebugChannelPrintf( radMovieDebugChannel2, "radMoviePlayer::SetState [%d]\n", state );
        m_State = state;
    }
}

//=============================================================================
// radMoviePlayer::InternalPlay
//=============================================================================

void radMoviePlayer::InternalPlay( void )
{
    m_refIRadSoundStreamPlayer->Play( );
    m_refIRadStopwatch->Start( );
    Service( );
}

//=============================================================================
// Function:    radMoviePlayerCreate2
//=============================================================================

IRadMoviePlayer2 * radMoviePlayerCreate2( radMemoryAllocator alloc )
{
	return new( alloc )radMoviePlayer( );
}

//=============================================================================
// Function:    radMovieInitialize2
//=============================================================================

void radMovieInitialize2( radMemoryAllocator alloc )
{
    ::radMovieVideoDecoderInitialize( alloc );
}

//=============================================================================
// Function:    radMovieTerminate2
//=============================================================================

void radMovieTerminate2( void )
{
    ::radMovieVideoDecoderTerminate( );
}

//=============================================================================
// Function:    radMovieService2
//=============================================================================

void radMovieService2( void )
{
    ref< radMoviePlayer > refRadMoviePlayer = radMoviePlayer::GetLinkedClassHead( );

    while( refRadMoviePlayer != NULL )
    {
        refRadMoviePlayer->Service( );

        refRadMoviePlayer = refRadMoviePlayer->GetLinkedClassNext( );
    }
}

#endif // ! RAD_MOVIEPLAYER_USE_BINK