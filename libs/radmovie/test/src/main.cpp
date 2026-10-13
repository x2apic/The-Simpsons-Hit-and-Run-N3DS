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
// 
// Welcome to the new movie player interface!!!!!
// 
//=============================================================================

//=============================================================================
// Included Files
//=============================================================================

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


//=============================================================================
// Definitions
//=============================================================================


#define ROBERT_ALLOCATOR 5

#define MOVIE_MAX_WIDTH 640
#define MOVIE_MAX_HEIGHT 480
#define MOVIE_ENCODED_VIDEO_BUFFER_SIZE ( 768 * 1024 )
#define MOVIE_PRIMARY_AUDIO_BUFFER_SIZE 2000
#define MOVIE_SECONDARY_AUDIO_BUFFER_SIZE 1000
#define MOVIE_AUDIO_BUFFER_SIZE_TYPE IRadSoundHalAudioFormat::Milliseconds

#ifdef RAD_PS2

    #define PLAYBACK_RATE 0
    #define SOUND_MEMORY 0
    #define AUX_SENDS 0
    #define SOUND_MAX_ROOT_ALLOCATIONS 8

//    #define VIDEO_FILE "sample_4000.rmv"
    #define VIDEO_FILE "robert.rmv"
//    #define AUDIO_TRACK_0 radMovie_NoAudioTrack
    #define AUDIO_TRACK_0 0
    #define AUDIO_TRACK_1 1

    #define PLAY_BUTTON "X"
    #define PAUSE_BUTTON "Circle"
    #define RELOAD_BUTTON "Square"
    #define RELOAD_BUTTON_1 "Triangle"
    #define QUIT_BUTTON "Start"
    #define VOLUME_AXIS "LeftStickY"
    #define PAN_AXIS "RightStickX"

#endif // RAD_PS2

#ifdef RAD_XBOX

    #define PLAYBACK_RATE 0
    #define SOUND_MEMORY 2 * 1024 * 1024
    #define AUX_SENDS 0
    #define SOUND_MAX_ROOT_ALLOCATIONS 8

    #define VIDEO_FILE "d:\\intro_final.RMV"
    #define AUDIO_TRACK_0 radMovie_NoAudioTrack
//    #define AUDIO_TRACK_0 0
    #define AUDIO_TRACK_1 1

    #define PLAY_BUTTON "A"
    #define PAUSE_BUTTON "B"
    #define RELOAD_BUTTON "X"
    #define RELOAD_BUTTON_1 "Y"
    #define QUIT_BUTTON "Start"
    #define VOLUME_AXIS "LeftStickY"
    #define PAN_AXIS "RightStickX"

#endif // RAD_XBOX

#ifdef RAD_GAMECUBE

    #define PLAYBACK_RATE 48000
    #define SOUND_MEMORY 2 * 1024 * 1024
    #define AUX_SENDS 0
    #define SOUND_MAX_ROOT_ALLOCATIONS 8

    #define VIDEO_FILE "movie.rmv"
//    #define AUDIO_TRACK_0 radMovie_NoAudioTrack
    #define AUDIO_TRACK_0 0
    #define AUDIO_TRACK_1 1

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
    PLAY_POINT,
    STOP_POINT,
    PAUSE_POINT,
	RELOAD_POINT,
	RELOAD_POINT_1,
    QUIT_POINT,
    VOLUME_POINT,
    PAN_POINT
};

//=============================================================================
// Forward Declarations
//=============================================================================

struct ControllerCallBack;
struct SampleRender;

//=============================================================================
// Global Declarations
//=============================================================================

//
//
// A test variable that can be changed at run-time using a debugger
//
//

unsigned int g_TheTestVariable = 0x00000000;



bool g_Done = false;
ref< SampleRender > g_refSampleRender = NULL;

//=============================================================================
// Struct SampleRender
//=============================================================================

struct SampleRender 
    : 
    public IRadMovieRenderLoop,
    public radRefCount
{
    IMPLEMENT_REFCOUNTED( "SampleRender" )

    SampleRender( void );
    virtual ~SampleRender( void );

    void Initialize( void );
    void Terminate( void );
    void Go( void );

    void IterateLoop( IRadMoviePlayer2 * pIRadMoviePlayer );

    void Load( unsigned int audioTrackIndex ) { if( m_LoadCount == 0 ) { m_refIRadMoviePlayer->Load( VIDEO_FILE, audioTrackIndex ); m_LoadCount++; } }
    void Unload( void );
    void Play( void ) { m_refIRadMoviePlayer->Play( ); }
    void Pause( void ) { m_refIRadMoviePlayer->Pause( ); }
    void SetPan( float pan ) { m_refIRadMoviePlayer->SetPan( pan ); }
    void SetVolume( float volume ) { m_refIRadMoviePlayer->SetVolume( volume ); }
    void Reinitialize( void );

    IRadMoviePlayer2::State m_LastState;

    ref< IRadMoviePlayer2 > m_refIRadMoviePlayer;
    tPlatform * m_pPlatform;
    tContext * m_pContext;

    unsigned int m_LoadCount;
};

//=============================================================================
// SampleRender::SampleRender
//

SampleRender::SampleRender( void )
    :
    radRefCount( 0 ),
    m_LastState( IRadMoviePlayer2::NoData ),
    m_refIRadMoviePlayer( NULL ),
    m_pPlatform( NULL ),
    m_pContext( NULL ),
    m_LoadCount( 0 )
{

}

//=============================================================================
// SampleRender::~SampleRender
//

SampleRender::~SampleRender( void )
{

}

//=============================================================================
// SampleRender::Initialize
//

void SampleRender::Initialize( void )
{
    //=========================================================================
    //
    // CREATION AND INITIALIZATION OF MOVIE PLAYER OBJECTS
    // 
    // * When playing a movie, you choose which class will implement the
    //   render loop and which class will render the video component of
    //   the movie
    //
    //=========================================================================

    //
    // RENDER LOOP INTERFACE
    //
    // The render loop interface must be implemented by your game somewhere.
    // The movie player uses this interface to let the game control what will
    // happen during the render of each frame and how the rendering of the movie
    // itself will fit into everything else that the game wants to render.
    //
   
    ref< IRadMovieRenderLoop > refIRadMovieRenderLoop = this;

    //
    // MOVIE RENDER STRATEGY
    //
    // The render strategy provides a render algorithm that the movie player
    // will use to render its video to the screen.  In this example we'll
    // use the simple full screen render strategy, which will render the
    // movie as a full screen.  (A texture render strategy is an example of 
    // what could also be implemented.)
    //
    
    ref< IRadMovieRenderStrategy > refIRadMovieRenderStrategy = ::radMovieSimpleFullScreenRenderStrategyCreate( ROBERT_ALLOCATOR );

    //
    // MOVIE PLAYER
    //
    // The movie player requires the above two objects in its initialization.
    // It also requires some parameters that control how much memory will
    // be allocated for buffering of data, etc.
    //
    
    m_refIRadMoviePlayer = ::radMoviePlayerCreate2( ROBERT_ALLOCATOR );
    rAssert( m_refIRadMoviePlayer != NULL );


#ifdef RAD_PS2

    m_refIRadMoviePlayer->Initialize( 
        refIRadMovieRenderLoop,
        refIRadMovieRenderStrategy,
        MOVIE_MAX_WIDTH, MOVIE_MAX_HEIGHT, 
        MOVIE_ENCODED_VIDEO_BUFFER_SIZE, 
        MOVIE_PRIMARY_AUDIO_BUFFER_SIZE, 
        MOVIE_SECONDARY_AUDIO_BUFFER_SIZE, 
        MOVIE_AUDIO_BUFFER_SIZE_TYPE );

#endif // RAD_PS2

#ifdef RAD_XBOX

    m_refIRadMoviePlayer->Initialize(
        refIRadMovieRenderLoop,
        refIRadMovieRenderStrategy );

#endif // RAD_XBOX

#ifdef RAD_GAMECUBE

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

void SampleRender::Reinitialize( void )
{
    m_refIRadMoviePlayer = NULL;

    ref< IRadMovieRenderLoop > refIRadMovieRenderLoop = this;
    
    ref< IRadMovieRenderStrategy > refIRadMovieRenderStrategy = ::radMovieSimpleFullScreenRenderStrategyCreate( ROBERT_ALLOCATOR );

    m_refIRadMoviePlayer = ::radMoviePlayerCreate2( ROBERT_ALLOCATOR );
    rAssert( m_refIRadMoviePlayer != NULL );

#ifdef RAD_PS2

    m_refIRadMoviePlayer->Initialize( 
        refIRadMovieRenderLoop,
        refIRadMovieRenderStrategy,
        MOVIE_MAX_WIDTH, MOVIE_MAX_HEIGHT, 
        MOVIE_ENCODED_VIDEO_BUFFER_SIZE, 
        MOVIE_PRIMARY_AUDIO_BUFFER_SIZE, 
        MOVIE_SECONDARY_AUDIO_BUFFER_SIZE, 
        MOVIE_AUDIO_BUFFER_SIZE_TYPE );

#endif // RAD_PS2

#ifdef RAD_XBOX

    m_refIRadMoviePlayer->Initialize(
        refIRadMovieRenderLoop,
        refIRadMovieRenderStrategy );
#endif // RAD_XBOX

#ifdef RAD_GAMECUBE

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
// SampleRender::Terminate
//

void SampleRender::Terminate( void )
{
    if( m_refIRadMoviePlayer != NULL )
    {
        m_refIRadMoviePlayer->Unload( );
    }
    m_refIRadMoviePlayer = NULL;
}

//=============================================================================
// SampleRender::Service
//

void SampleRender::Go( void )
{
    Unload( );
    Load( AUDIO_TRACK_0 );
    Play( );

    IRadMoviePlayer2::State state = m_refIRadMoviePlayer->GetState( );

    g_TheTestVariable = 1;

    while( true )
    {
		::radMovieService2( );
        ::radControllerSystemService( );
        ::radSoundHalSystemGet( )->Service( );
        ::radSoundHalSystemGet( )->ServiceOncePerFrame( );      
        ::radFileService( );
        ::radMemoryMonitorService( );
        ::radDbgComService( );


        state = m_refIRadMoviePlayer->GetState( );

        if( g_TheTestVariable == 0 )
        {
            //
            // The default test is a random function call test
            //

            unsigned int time = ::radTimeGetMilliseconds( );

            if( ( time % 256 ) == 0 )
            {
                unsigned int functionNumber = rand( ) % 15;

                switch( functionNumber )
                {
                case 0:
                    {
//                        rReleasePrintf( "Load\n" );
                        this->Load( 0 );
                        break;
                    }
                case 1:
                    {
//                        rReleasePrintf( "Pause\n" );
                        this->Pause( );
                        break;
                    }
                case 2:
                    {
//                        rReleasePrintf( "Play\n" );
                        this->Play( );
                        break;
                    }
                case 3:
                    {
//                        rReleasePrintf( "Reinitialize\n" );
                        this->Reinitialize( );
                        break;
                    }
                case 4:
                    {
//                        rReleasePrintf( "SetPan\n" );
                        this->SetPan( ( ( float )( time % 202 ) - 101.0f ) / 101.0f );
                        break;
                    }
                case 5:
                    {
//                        rReleasePrintf( "SetVolume\n" );
                        this->SetVolume( ( time % 202 ) / ( float ) 202 );
                        break;
                    }
                case 6:
                    {
//                        rReleasePrintf( "Unload\n" );
                        this->Unload( );
                        break;
                    }
                case 7:
                    {
//                        rReleasePrintf( "Seeding random number generator\n" );
                        srand( ::radTimeGetMilliseconds( ) );
                    }
                default:
                    {
                        break;
                    }
                }

            }
        }
        else if( g_TheTestVariable == 1 )
        {
            if( state == IRadMoviePlayer2::NoData )
            {
                static unsigned int g_Test1_LoadCount = 0;
                this->Unload( );
                this->Load( radMovie_NoAudioTrack );
                this->Play( );
                g_Test1_LoadCount++;
            }
        }
    }
}

//=============================================================================
// SampleRender::IterateLoop
//

void SampleRender::IterateLoop( IRadMoviePlayer2 * pIRadMoviePlayer )
{
    rAssert( pIRadMoviePlayer != NULL );

    //
    // This function gives your game full control over 
    // what happens when the movie player needs to render a frame.
    //
    // There are a few requirements...
    // 
    // 1) You must call BeginFrame() and EndFrame()
    //
    // 2) You must call IRadMoviePlayer::Render() somewhere
    //    between the calls to BeginFrame() and EndFrame()
    //

    //
    // This next call is required...
    //
    p3d::context->SetClearMask( PDDI_BUFFER_ALL );

    p3d::context->BeginFrame( );

    //
    // (Do whatever you like here.)
    //
    // This next call is required...
    //
    pIRadMoviePlayer->Render( );

    //
    // (Do whatever you like here.)
    //

    p3d::pddi->DrawString( "RED", 120, 427, pddiColour( 255, 0, 0 ) );
    p3d::pddi->DrawString( "GREEN", 180, 427, pddiColour( 0, 255, 0 ) );
    p3d::pddi->DrawString( "BLUE", 260, 427, pddiColour( 0, 0, 255 ) );
    
    //
    // This next call is required...
    //
    p3d::context->EndFrame( true );
}

//=============================================================================
// SampleRender::Unload
//

void SampleRender::Unload( void )
{
    if( m_LoadCount > 0 )
    {
        m_LoadCount--;
    }

    //
    // Call unload on the movie player
    //

    m_refIRadMoviePlayer->Unload( );

	p3d::pddi->Clear(PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH);
}

//=============================================================================
// Struct ControllerCallback
//=============================================================================

struct ControllerCallBack : public IRadControllerInputPointCallback, radRefCount
{
    IMPLEMENT_REFCOUNTED( "ControllerCallBack" )

    ControllerCallBack( void ) : radRefCount( 0 ) { }

	void OnControllerInputPointChange( unsigned int userData, float newValue)
	{
		// care about when the button is pressed only
		if(newValue == 1.0f)
		{
		    switch(userData)
			{
            case PLAY_POINT:
                // rDebugPrintf( "DEBUG: (Play) input[%d] val[%f]\n", userData, newValue);
                g_refSampleRender->Play( );
                break;
            case PAUSE_POINT:
                // rDebugPrintf( "DEBUG: (Pause) input[%d] val[%f]\n", userData, newValue);
                g_refSampleRender->Pause( );
                break;
            case RELOAD_POINT:
                // rDebugPrintf( "DEBUG: (Unload-Load) input[%d] val[%f]\n", userData, newValue);
                g_refSampleRender->Unload( );
                g_refSampleRender->Load( AUDIO_TRACK_0 );
                break;
            case RELOAD_POINT_1:
                // rDebugPrintf( "DEBUG: (Unload-Load) input[%d] val[%f]\n", userData, newValue);
                g_refSampleRender->Unload( );
                g_refSampleRender->Load( AUDIO_TRACK_1 );
                break;
            case QUIT_POINT:
                // rDebugPrintf( "DEBUG: (Unload) input[%d] val[%f]\n", userData, newValue);
                g_refSampleRender->Unload( );
                g_Done = true;
                break;
			}
		}
        else
        {
		    switch(userData)
			{
            case VOLUME_POINT:
                // rDebugPrintf( "DEBUG: (Volume) input[%d] val[%f]\n", userData, newValue);
                g_refSampleRender->SetVolume( newValue );
                break;
            case PAN_POINT:
                // rDebugPrintf( "DEBUG: (Pan) input[%d] val[%f]\n", userData, newValue);
                g_refSampleRender->SetPan( newValue * 2 - 1 );
                break;
            }
        }

	}
};

//=============================================================================
// Main
//=============================================================================

int main( void )
{
    rReleaseString("===========================\n" );
    rReleaseString("Movie Player Test Start\n" );
    rReleaseString("===========================\n" );

    ::radThreadInitialize();
    ::radMemoryInitialize();
    ::radTimeInitialize( );

    #ifdef RAD_PS2
        // Initialize using DECI as the communication avenue.
        ::radDbgComTargetInitialize( Deci );

    #elif defined RAD_WIN32 || defined RAD_XBOX

        ::radDbgComTargetInitialize( WinSocket );

    #elif defined RAD_GAMECUBE
        
        ::radDbgComTargetInitialize( HostIO );

    #endif        

    radDbgWatchInitialize_Macro( ( "\\" ) );
    radMemoryMonitorInitialize( 50 * 1024 );

	::radPlatformInitialize
    ( 
        #ifdef RAD_PS2 
        "irx\\", IOPMediaHost, GameMediaCD, NULL, RADMEMORY_ALLOC_DEFAULT 
        #endif // RAD_PS2
    );


    ::radFileInitialize( 50, 8 );
    ::radControllerInitialize( NULL, RADMEMORY_ALLOC_DEFAULT );

#ifndef RAD_GAMECUBE

    ref< IRadMemoryHeap > refIRadMemoryHeap = ::radMemoryCreateDougLeaHeap( 5 * 1024 * 1024 / 2, RADMEMORY_ALLOC_DEFAULT, "TestMovieHeap" );
#else
    ref< IRadMemoryHeap > refIRadMemoryHeap = ::radMemoryCreateDougLeaHeap( 4 * 1024 * 1024, RADMEMORY_ALLOC_DEFAULT, "TestMovieHeap" );
#endif


    ::radMemoryRegisterAllocator( ROBERT_ALLOCATOR, RADMEMORY_ALLOC_DEFAULT, refIRadMemoryHeap );




    ::radSoundHalSystemInitialize( ROBERT_ALLOCATOR );
    ::radSoundHalSystemGet( )->InitializeMemory( SOUND_MEMORY, SOUND_MAX_ROOT_ALLOCATIONS, AUX_SENDS );
    ::radMovieInitialize2( ROBERT_ALLOCATOR );


    //=========================================================================
    //
    // PURE3D INITIALIZATION:
    // 
    // * Done by your game already
    //
    //=========================================================================

    // Initialise Pure3D platform object.

    tPlatform * m_pPlatform = tPlatform::Create();
    rAssert( m_pPlatform );

    // Initialise the Pure3D context object.

    #ifdef RAD_PS2

    tContextInitData init;
    init.xsize      = 640;
    init.bpp        = 32;

    #endif // RAD_PS2

    #ifdef RAD_XBOX

    tContextInitData init;
    init.xsize = 640;
    init.ysize = 480;
    init.lockToVsync = true;

    #endif // RAD_XBOX

    #ifdef RAD_GAMECUBE

    tContextInitData init;

    init.xsize = 640;
    init.ysize = 480;
    init.lockToVsync = true;

    #endif // RAD_GAMECUBE

    tContext * m_pContext = m_pPlatform->CreateContext( & init );
    rAssert( m_pContext );   

    // Assign this context to the platform.

    m_pPlatform->SetActiveContext( m_pContext );

    // Set up Pddi

    p3d::pddi->EnableStatsOverlay( false );   // Turn stats on or off here
    p3d::pddi->EnableZBuffer( false );
    p3d::InstallDefaultLoaders();
	p3d::pddi->SetProjectionMode(PDDI_PROJECTION_DEVICE);
	p3d::pddi->SetCullMode(PDDI_CULL_NONE);
	p3d::pddi->Clear(PDDI_BUFFER_COLOUR | PDDI_BUFFER_DEPTH);

    ///////////////////////////////////////////
	// INITIALIZION OF SAMPLE RENDER OBJECT  //
	///////////////////////////////////////////

    g_refSampleRender = new( RADMEMORY_ALLOC_DEFAULT ) SampleRender( );
    g_refSampleRender->Initialize( );

	///////////////////////
	// SET UP CONTROLLER //
	///////////////////////

    ref< IRadController > refIRadController ( ::radControllerSystemGet( )->GetControllerByIndex( 0 ) );

	rAssert( refIRadController != NULL );
	rAssert( refIRadController->IsConnected() );

    ref< IRadControllerInputPoint > refIRcip_Play  (  refIRadController->GetInputPointByName( PLAY_BUTTON ) );
    ref< IRadControllerInputPoint > refIRcip_Pause ( refIRadController->GetInputPointByName( PAUSE_BUTTON ) );
    ref< IRadControllerInputPoint > refIRcip_Reload ( refIRadController->GetInputPointByName( RELOAD_BUTTON ) );
    ref< IRadControllerInputPoint > refIRcip_Reload1  (  refIRadController->GetInputPointByName( RELOAD_BUTTON_1 ) );
    ref< IRadControllerInputPoint > refIRcip_Quit  ( refIRadController->GetInputPointByName( QUIT_BUTTON ) );
    ref< IRadControllerInputPoint > refIRcip_Volume  ( refIRadController->GetInputPointByName( VOLUME_AXIS ) );
    ref< IRadControllerInputPoint > refIRcip_Pan     ( refIRadController->GetInputPointByName( PAN_AXIS ) );

	ref< ControllerCallBack > refControllerCallback = new ( RADMEMORY_ALLOC_DEFAULT ) ControllerCallBack();

    refIRcip_Play->RegisterControllerInputPointCallback( refControllerCallback, PLAY_POINT );
    refIRcip_Pause->RegisterControllerInputPointCallback( refControllerCallback, PAUSE_POINT );
    refIRcip_Reload->RegisterControllerInputPointCallback( refControllerCallback, RELOAD_POINT );
    refIRcip_Reload1->RegisterControllerInputPointCallback( refControllerCallback, RELOAD_POINT_1 );
    refIRcip_Quit->RegisterControllerInputPointCallback( refControllerCallback, QUIT_POINT );
    refIRcip_Volume->RegisterControllerInputPointCallback( refControllerCallback, VOLUME_POINT );
    refIRcip_Pan->RegisterControllerInputPointCallback( refControllerCallback, PAN_POINT );

	//////////////////////////////////
	// PRINT OUT USAGE INSTRUCTIONS //
    //////////////////////////////////

    rReleasePrintf( 
      "\n///////////////////////////////////\n" \
        "//  MOVIEPLAYER SAMPLE CONTROLS  //\n" \
        "///////////////////////////////////\n" \
        "// PLAY:           [%11s] //\n"       \
        "// PAUSE:          [%11s] //\n"       \
        "// RELOAD AUDIO 0: [%11s] //\n"       \
        "// RELOAD AUDIO 1: [%11s] //\n"       \
        "// VOLUME:         [%11s] //\n"       \
        "// PAN:            [%11s] //\n"       \
        "// QUIT:           [%11s] //\n"       \
        "///////////////////////////////////\n",
        PLAY_BUTTON,
        PAUSE_BUTTON,
        RELOAD_BUTTON, 
        RELOAD_BUTTON_1, 
        VOLUME_AXIS,
        PAN_AXIS,
        QUIT_BUTTON );

	//////////////////////////////////
	// START OF THE MAIN WHILE LOOP //
    //////////////////////////////////

    g_refSampleRender->Go( );
    g_refSampleRender->Terminate( );

	//////////////////////////
	// CLEAN UP EVERYTHING  //
    //////////////////////////
   
    refIRcip_Play->UnRegisterControllerInputPointCallback( refControllerCallback );
    refIRcip_Pause->UnRegisterControllerInputPointCallback( refControllerCallback );
    refIRcip_Reload->UnRegisterControllerInputPointCallback( refControllerCallback );
    refIRcip_Reload1->UnRegisterControllerInputPointCallback( refControllerCallback );
    refIRcip_Quit->UnRegisterControllerInputPointCallback( refControllerCallback );
    refIRcip_Volume->UnRegisterControllerInputPointCallback( refControllerCallback );
    refIRcip_Pan->UnRegisterControllerInputPointCallback( refControllerCallback );

    refIRcip_Play = NULL;
    refIRcip_Pause = NULL;
    refIRcip_Reload = NULL;
    refIRcip_Reload1 = NULL;
    refIRcip_Quit = NULL;
    refIRcip_Volume = NULL;
    refIRcip_Pan = NULL;
    refControllerCallback = NULL;
    refIRadController = NULL;
    g_refSampleRender = NULL;

    m_pPlatform->DestroyContext( m_pContext );

    m_pContext = NULL;
    tPlatform::Destroy( m_pPlatform );
    m_pPlatform = NULL;


    ::radMemoryUnregisterAllocator( ROBERT_ALLOCATOR );
    refIRadMemoryHeap = NULL;

    for( unsigned int cleanup = 0; cleanup < 500; cleanup++ )
    {
        ::radControllerSystemService( );
		::radMovieService2( );
        ::radSoundHalSystemGet( )->Service( );
        ::radSoundHalSystemGet( )->ServiceOncePerFrame( );      
        ::radFileService( );
    }

    ::radMovieTerminate2( );
    ::radSoundHalSystemTerminate( );
    ::radControllerTerminate( );
    ::radFileTerminate( );
    radDbgWatchTerminate_Macro( ( ) );
    radDbgComTargetTerminate( );

    ::radMemoryMonitorTerminate( );
    ::radDbgComTargetTerminate( );

    ::radTimeTerminate( );
    ::radPlatformTerminate( );
    ::radMemoryTerminate();

    //
    // This will assert on the ps2 because the 
    // pure3d context doesn't actually get destroyed
    // on that platform
    //

    ::radThreadTerminate();    

    #ifdef RAD_DEBUG
        radObject::DumpObjects( );
    #endif
        
    rReleaseString("===========================\n" );
    rReleaseString("Movie Player Test End\n" );
    rReleaseString("===========================\n" );
}


