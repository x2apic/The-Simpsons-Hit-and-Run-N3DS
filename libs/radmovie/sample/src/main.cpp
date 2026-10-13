//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        main.cpp
//
// Subsystem:   Foundation Technologies - Movie
//
// Description: The simple movie sample program
//
// Date:        June 10, 2002
//
// Note:        If use the movie player in your game like this, everything
//              should work just fine.
// 
//=============================================================================

//=============================================================================
// Included Files
//=============================================================================

#ifdef RAD_XBOX
#include <XTL.h>
#endif

#include "util.hpp"

#include <radplatform.hpp>
#include <radfile.hpp>
#include <radtime.hpp>
#include <radmemory.hpp>
#include <radsound.hpp>
#include <raddebug.hpp>
#include <radcontroller.hpp>
#include <radmovie2.hpp>
#include <p3d/pure3d.hpp>
#include <radmath/radmath.hpp>
#include <radthread.hpp>
#include <radmemorymonitor.hpp>
#include <raddebugcommunication.hpp>
#include <raddebugwatch.hpp>
#include <radprofiler.hpp>

//=============================================================================
// Definitions
//=============================================================================

#define MOVIE_MAX_WIDTH 640
#define MOVIE_MAX_HEIGHT 480
#define MOVIE_ENCODED_VIDEO_BUFFER_SIZE ( 768 * 1024 )
#define MOVIE_PRIMARY_AUDIO_BUFFER_SIZE 2000
#define MOVIE_SECONDARY_AUDIO_BUFFER_SIZE 1000
#define MOVIE_AUDIO_BUFFER_SIZE_TYPE IRadSoundHalAudioFormat::Milliseconds

#ifdef RAD_PS2
    #define VIDEO_FILE "sample.rmv"

    #define PLAY_BUTTON "X"
    #define PAUSE_BUTTON "Circle"
    #define RELOAD_BUTTON "Square"
    #define RELOAD_BUTTON_1 "Triangle"
    #define QUIT_BUTTON "Start"
    #define VOLUME_AXIS "LeftStickY"
    #define PAN_AXIS "RightStickX"
#endif // RAD_PS2

#if defined RAD_XBOX || RAD_WIN32
    #ifdef RAD_XBOX

        #ifdef RAD_MOVIEPLAYER_USE_BINK
            #define VIDEO_FILE "d:\\sample_bink.rmv"
        #else
            #define VIDEO_FILE "d:\\sample_xmv.rmv"
        #endif //RAD_MOVIEPLAYER_USE_BINK

    #else

        #define VIDEO_FILE "sample_bink.rmv"

    #endif // RAD_XBOX

    #ifdef RAD_XBOX
        #define PLAY_BUTTON "A"
        #define PAUSE_BUTTON "B"
        #define RELOAD_BUTTON "X"
        #define RELOAD_BUTTON_1 "Y"
        #define QUIT_BUTTON "Start"
        #define VOLUME_AXIS "LeftStickY"
        #define PAN_AXIS "RightStickX"
    #endif // RAD_XBOX

    #ifdef RAD_WIN32
        #define PLAY_BUTTON "??"
        #define PAUSE_BUTTON "??"
        #define RELOAD_BUTTON "??"
        #define RELOAD_BUTTON_1 "??"
        #define QUIT_BUTTON "??"
        #define VOLUME_AXIS "??"
        #define PAN_AXIS "??"
    #endif // RAD_XBOX

#endif // RAD_XBOX || RAD_WIN32

#ifdef RAD_GAMECUBE
    #ifdef RAD_MOVIEPLAYER_USE_BINK
        #define VIDEO_FILE "sample_bink.rmv"
    #else
        #define VIDEO_FILE "sample.rmv"
    #endif 

    #define PLAY_BUTTON "A"
    #define PAUSE_BUTTON "X"
    #define RELOAD_BUTTON "B"
    #define RELOAD_BUTTON_1 "Y"
    #define QUIT_BUTTON "Menu"
    #define VOLUME_AXIS "LeftStickY"
    #define PAN_AXIS "RightStickX"

#endif // RAD_GAMECUBE

enum InputPointIDs
{
    PLAY_POINT = 0,
    PAUSE_POINT,
	RELOAD_POINT,
	RELOAD_POINT_1,
    QUIT_POINT,
    VOLUME_POINT,
    PAN_POINT,
    NUM_POINTS
};

#define AUDIO_TRACK_0 radMovie_NoAudioTrack
#define AUDIO_TRACK_1 0

//=============================================================================
// Forward Declarations
//=============================================================================

struct ControllerCallBack;
struct MovieRender;

//=============================================================================
// Global Declarations
//=============================================================================

bool g_Done = false;
ref< MovieRender > g_refMovieRender = NULL;

//=============================================================================
// Struct MovieRender
//=============================================================================

struct MovieRender 
    : 
    public IRadMovieRenderLoop,
    public radRefCount
{
    IMPLEMENT_REFCOUNTED( "MovieRender" )

    MovieRender( void );
    virtual ~MovieRender( void );

    void Initialize( void );
    void Terminate( void );

    void DoRender( void );
    void IterateLoop( IRadMoviePlayer2 * pIRadMoviePlayer );

    void Load( unsigned int audioTrackIndex ) { m_refIRadMoviePlayer->Load( VIDEO_FILE, audioTrackIndex ); }
    void Unload( void );
    void Play( void ) { m_refIRadMoviePlayer->Play( ); }
    void Pause( void ) { m_refIRadMoviePlayer->Pause( ); }
    void SetPan( float pan ) { m_refIRadMoviePlayer->SetPan( pan ); }
    void SetVolume( float volume ) { m_refIRadMoviePlayer->SetVolume( volume ); }

    ref< IRadMoviePlayer2 > m_refIRadMoviePlayer;
    bool m_IsFrameReady;
};

//=============================================================================
// Struct InputHandler
//=============================================================================

struct InputHandler : public IRadControllerInputPointCallback, radRefCount
{
    IMPLEMENT_REFCOUNTED( "ControllerCallBack" )

    InputHandler( void ) : radRefCount( 0 ), m_refIRadController( NULL ) { for( unsigned int i = 0; i < NUM_POINTS; i++ ) m_refInputs[ i ] = NULL; }

    void Initialize( void );
    void Terminate( void );
    void OnControllerInputPointChange( unsigned int userData, float newValue);

    ref< IRadControllerInputPoint > m_refInputs[ NUM_POINTS ];
    ref< IRadController > m_refIRadController;
};

//=============================================================================
// MovieRender::MovieRender
//

MovieRender::MovieRender( void )
    :
    radRefCount( 0 ),
    m_refIRadMoviePlayer( NULL ),
    m_IsFrameReady( false )
{
}

//=============================================================================
// MovieRender::~MovieRender
//

MovieRender::~MovieRender( void )
{
}

//=============================================================================
// MovieRender::Initialize
//

void MovieRender::Initialize( void )
{
    //
    // RENDER LOOP INTERFACE
    //
   
    ref< IRadMovieRenderLoop > refIRadMovieRenderLoop = this;

    //
    // MOVIE RENDER STRATEGY
    //
    
    ref< IRadMovieRenderStrategy > refIRadMovieRenderStrategy = ::radMovieSimpleFullScreenRenderStrategyCreate( RADMEMORY_ALLOC_DEFAULT );

    //
    // MOVIE PLAYER
    //
    
    m_refIRadMoviePlayer = ::radMoviePlayerCreate2( RADMEMORY_ALLOC_DEFAULT );
    rAssert( m_refIRadMoviePlayer != NULL );

    // Initialization...

    #if ( defined RAD_XBOX || defined RAD_WIN32 || defined RAD_MOVIEPLAYER_USE_BINK )

    m_refIRadMoviePlayer->Initialize(
        refIRadMovieRenderLoop,
        refIRadMovieRenderStrategy );

    #elif RAD_PS2

    m_refIRadMoviePlayer->Initialize( 
        refIRadMovieRenderLoop,
        refIRadMovieRenderStrategy,
        MOVIE_MAX_WIDTH, MOVIE_MAX_HEIGHT, 
        MOVIE_ENCODED_VIDEO_BUFFER_SIZE, 
        MOVIE_PRIMARY_AUDIO_BUFFER_SIZE, 
        MOVIE_SECONDARY_AUDIO_BUFFER_SIZE, 
        MOVIE_AUDIO_BUFFER_SIZE_TYPE );

    #elif RAD_GAMECUBE

    m_refIRadMoviePlayer->Initialize( 
        refIRadMovieRenderLoop,
        refIRadMovieRenderStrategy,
        MOVIE_MAX_WIDTH, MOVIE_MAX_HEIGHT, 
        MOVIE_ENCODED_VIDEO_BUFFER_SIZE, 
        MOVIE_PRIMARY_AUDIO_BUFFER_SIZE, 
        MOVIE_SECONDARY_AUDIO_BUFFER_SIZE, 
        MOVIE_AUDIO_BUFFER_SIZE_TYPE );

    #endif 
}

//=============================================================================
// MovieRender::Terminate
//

void MovieRender::Terminate( void )
{
    if( m_refIRadMoviePlayer != NULL )
    {
        m_refIRadMoviePlayer->Unload( );
    }
    m_refIRadMoviePlayer = NULL;
}

//=============================================================================
// MovieRender::DoRender
//

void MovieRender::DoRender( void )
{
    //
    // This the currently recommended practice for driving the movie
    // is to make your render call something like this...
    // 

    IRadMoviePlayer2::State state = m_refIRadMoviePlayer->GetState( );

    if( m_refIRadMoviePlayer != NULL )
    {
        IRadMoviePlayer2::State state = m_refIRadMoviePlayer->GetState( );
       
        if( state != IRadMoviePlayer2::NoData )
        {
            while( m_IsFrameReady == false && state != IRadMoviePlayer2::NoData && g_Done == false )
            {
                ::utilATGLibService();
                ::radFileService( );
                ::radSoundHalSystemGet( )->Service( );
                ::radSoundHalSystemGet( )->ServiceOncePerFrame( );
                ::radMovieService2( );
                ::radControllerSystemService( );

                state = m_refIRadMoviePlayer->GetState( );
            }

            m_IsFrameReady = false;
        }
    }
}

//=============================================================================
// MovieRender::IterateLoop
//

void MovieRender::IterateLoop( IRadMoviePlayer2 * pIRadMoviePlayer )
{
    //
    // Tell the movie player to render here and signal that 
    // its been done.
    //

    m_refIRadMoviePlayer->Render( );
    m_IsFrameReady = true;
}

//=============================================================================
// MovieRender::Unload
//

void MovieRender::Unload( void )
{
    //
    // Call unload on the movie player
    //

    m_refIRadMoviePlayer->Unload( );
	p3d::pddi->Clear(PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH);
}

//=============================================================================
// InputHandler::Initialize
//

void InputHandler::Initialize( void )
{
    #ifndef RAD_WIN32
    m_refIRadController = ::radControllerSystemGet( )->GetControllerByIndex( 0 );
	rAssert( m_refIRadController != NULL );
	rAssert( m_refIRadController->IsConnected() );
    m_refInputs[ PLAY_POINT ]     = m_refIRadController->GetInputPointByName( PLAY_BUTTON );
    m_refInputs[ PAUSE_POINT ]    = m_refIRadController->GetInputPointByName( PAUSE_BUTTON );
    m_refInputs[ RELOAD_POINT ]   = m_refIRadController->GetInputPointByName( RELOAD_BUTTON );
    m_refInputs[ RELOAD_POINT_1 ] = m_refIRadController->GetInputPointByName( RELOAD_BUTTON_1 );
    m_refInputs[ QUIT_POINT ]     = m_refIRadController->GetInputPointByName( QUIT_BUTTON );
    m_refInputs[ VOLUME_POINT ]   = m_refIRadController->GetInputPointByName( VOLUME_AXIS );
    m_refInputs[ PAN_POINT ]      = m_refIRadController->GetInputPointByName( PAN_AXIS );
    #else
    m_refIRadController = ::radControllerSystemGet( )->GetControllerAtLocation( "Port0\\Slot0" );
	rAssert( m_refIRadController != NULL );
	rAssert( m_refIRadController->IsConnected() );
    m_refInputs[ PLAY_POINT ]     = m_refIRadController->GetInputPointByTypeAndIndex( "Button", 0 );
    m_refInputs[ PAUSE_POINT ]    = m_refIRadController->GetInputPointByTypeAndIndex( "Button", 1 );
    m_refInputs[ RELOAD_POINT ]   = m_refIRadController->GetInputPointByTypeAndIndex( "Button", 2 );
    m_refInputs[ RELOAD_POINT_1 ] = m_refIRadController->GetInputPointByTypeAndIndex( "Button", 3 );
    m_refInputs[ QUIT_POINT ]     = m_refIRadController->GetInputPointByTypeAndIndex( "Button", 4 );
    m_refInputs[ VOLUME_POINT ]   = m_refIRadController->GetInputPointByTypeAndIndex( "YAxis", 0 );
    m_refInputs[ PAN_POINT ]      = m_refIRadController->GetInputPointByTypeAndIndex( "XAxis", 0 );
    #endif 
    for( unsigned int i = 0; i < NUM_POINTS; i++ ) { rAssert( m_refInputs[ i ] != NULL ); }
    m_refInputs[ PLAY_POINT ]->RegisterControllerInputPointCallback( this, PLAY_POINT );
    m_refInputs[ PAUSE_POINT ]->RegisterControllerInputPointCallback( this, PAUSE_POINT );
    m_refInputs[ RELOAD_POINT ]->RegisterControllerInputPointCallback( this, RELOAD_POINT );
    m_refInputs[ RELOAD_POINT_1 ]->RegisterControllerInputPointCallback( this, RELOAD_POINT_1 );
    m_refInputs[ QUIT_POINT ]->RegisterControllerInputPointCallback( this, QUIT_POINT );
    m_refInputs[ VOLUME_POINT ]->RegisterControllerInputPointCallback( this, VOLUME_POINT );
    m_refInputs[ PAN_POINT ]->RegisterControllerInputPointCallback( this, PAN_POINT );

    rReleasePrintf( 
      "\n///////////////////////////////////\n" \
        "//  MOVIEPLAYER SAMPLE CONTROLS  //\n" \
        "///////////////////////////////////\n" \
        "// PLAY:           [%11s] //\n"       \
        "// PAUSE:          [%11s] //\n"       \
        "// RELOAD NO AUDIO:[%11s] //\n"       \
        "// RELOAD AUDIO 1: [%11s] //\n"       \
        "// VOLUME:         [%11s] //\n"       \
        "// PAN:            [%11s] //\n"       \
        "// QUIT:           [%11s] //\n"       \
        "///////////////////////////////////\n",
            m_refInputs[ PLAY_POINT ]->GetName( ),
            m_refInputs[ PAUSE_POINT ]->GetName( ),
            m_refInputs[ RELOAD_POINT ]->GetName( ),
            m_refInputs[ RELOAD_POINT_1 ]->GetName( ),
            m_refInputs[ VOLUME_POINT ]->GetName( ),
            m_refInputs[ PAN_POINT ]->GetName( ),
            m_refInputs[ QUIT_POINT ]->GetName( ) );


}

//=============================================================================
// InputHandler::Initialize
//

void InputHandler::OnControllerInputPointChange( unsigned int userData, float newValue)
{
    if( userData == VOLUME_POINT || userData == PAN_POINT )
    {
		switch(userData)
		{
            case VOLUME_POINT:
                // rDebugPrintf( "DEBUG: (Volume) input[%d] val[%f]\n", userData, newValue);
                g_refMovieRender->SetVolume( newValue );
                break;
            case PAN_POINT:
                // rDebugPrintf( "DEBUG: (Pan) input[%d] val[%f]\n", userData, newValue);
                g_refMovieRender->SetPan( newValue * 2 - 1 );
                break;
        }
    }
    else
    {
        if( newValue > 0.5f )
        {
            switch( userData )
            {
                case PLAY_POINT:
                    // rReleasePrintf( "DEBUG: (Play) input[%d] val[%f]\n", userData, newValue);
                    g_refMovieRender->Play( );
                    break;
                case PAUSE_POINT:
                    // rReleasePrintf( "DEBUG: (Pause) input[%d] val[%f]\n", userData, newValue);
                    g_refMovieRender->Pause( );
                    break;
                case RELOAD_POINT:
                    // rReleasePrintf( "DEBUG: (Unload-Load) input[%d] val[%f]\n", userData, newValue);
                    g_refMovieRender->Unload( );
                    g_refMovieRender->Load( AUDIO_TRACK_0 );
                    break;
                case RELOAD_POINT_1:
                    //rReleasePrintf( "DEBUG: (Unload-Load) input[%d] val[%f]\n", userData, newValue);
                    g_refMovieRender->Unload( );
                    g_refMovieRender->Load( AUDIO_TRACK_1 );
                    break;
                case QUIT_POINT:
                    // rReleasePrintf( "DEBUG: (Unload) input[%d] val[%f]\n", userData, newValue);
                    g_Done = true;
                    break;
            }
        }
    }
}

//=============================================================================
// InputHandler::Terminate
//

void InputHandler::Terminate( void )
{
    for( unsigned int i = 0; i < NUM_POINTS; i++ ) {  m_refInputs[ i ]->UnRegisterControllerInputPointCallback( this ); }
}

//=============================================================================
// Main
//=============================================================================

#ifdef RAD_PS2
int main( int argc, char** argv )
#else
int main( void )
#endif //RAD_WIN32
{
    rReleaseString("===========================\n" );
    rReleaseString("Movie Player Test Start\n" );
    rReleaseString("===========================\n" );

    ::utilATGLibInit( 
        #ifdef RAD_PS2
        argc, argv
        #endif 
        );

    ::radMovieInitialize2( RADMEMORY_ALLOC_DEFAULT );

    ::utilPure3DInit( );

    g_refMovieRender = new( RADMEMORY_ALLOC_DEFAULT ) MovieRender( );
    g_refMovieRender->Initialize( );

    ref< InputHandler > refInputHandler = new ( RADMEMORY_ALLOC_DEFAULT ) InputHandler( );
    refInputHandler->Initialize( );

    while( ! g_Done )
    {
        ::utilATGLibService( );

        // Render the next frame...
        
        p3d::context->SetClearMask( PDDI_BUFFER_ALL );
        p3d::context->BeginFrame( );

        // Render the movie

        g_refMovieRender->DoRender( );

        // Maybe we would like to draw some subtitles...

        p3d::pddi->DrawString( "RED", 120, 427, pddiColour( 255, 0, 0 ) );
        p3d::pddi->DrawString( "GREEN", 180, 427, pddiColour( 0, 255, 0 ) );
        p3d::pddi->DrawString( "BLUE", 260, 427, pddiColour( 0, 0, 255 ) );

        // The frame is done.

        p3d::context->EndFrame( true );
    }

    refInputHandler->Terminate( );
    refInputHandler = NULL;
    g_refMovieRender->Terminate( );
    g_refMovieRender = NULL;
    ::utilPure3DTerminate( );

    ::radMovieTerminate2( );

    ::utilATGLibTerminate( );
       
    rReleaseString("===========================\n" );
    rReleaseString("Movie Player Test End\n" );
    rReleaseString("===========================\n" );

#ifdef RAD_WIN32
    return 0;
#endif //RAD_WIN32
}

void MemoryHackCallback() { }
void LeakDetectionStart( void ) {}
void LeakDetectionStop( void ) {}
void LeakDetectionAddRecord( const void* pMemory, const unsigned int size, const radMemoryAllocator heap ) {}
void LeakDetectionRemoveRecord( void* pMemory ) {}
