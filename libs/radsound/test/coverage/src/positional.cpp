//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "initradcore.hpp"
#include "initradsound.hpp"

#include <radcontroller.hpp>
#include <radtextdisplay.hpp>
#include <radsound.hpp>
#include <raddebugwatch.hpp>
#include "positional.hpp"

extern bool g_Done;

#ifdef RAD_PS2

    #define PLAYBACK_RATE 0
    #define SOUND_MEMORY 0
    #define AUX_SENDS 0
    #define SOUND_MAX_ROOT_ALLOCATIONS 8

    #define VIDEO_FILE "sample.rmv"
    #define AUDIO_TRACK_0 0
    #define AUDIO_TRACK_1 1

    #define PLAY_BUTTON "X"
    #define PAUSE_BUTTON "Circle"
    #define RELOAD_BUTTON "Square"
    #define RELOAD_BUTTON_1 "Triangle"
    #define QUIT_BUTTON "Start"
    #define SOURCE_POS_X_AXIS "LeftStickX"
    #define SOURCE_POS_Y_AXIS "LeftStickY"

#endif // RAD_PS2

#ifdef RAD_XBOX

    #define PLAYBACK_RATE 0
    #define SOUND_MEMORY 2 * 1024 * 1024
    #define AUX_SENDS 0
    #define SOUND_MAX_ROOT_ALLOCATIONS 8

    #define VIDEO_FILE "d:\\sample.rmv"
    #define AUDIO_TRACK_0 radMovie_NoAudioTrack
//    #define AUDIO_TRACK_0 0
    #define AUDIO_TRACK_1 1

    #define PLAY_BUTTON "A"
    #define PAUSE_BUTTON "B"
    #define RELOAD_BUTTON "X"
    #define RELOAD_BUTTON_1 "Y"
    #define QUIT_BUTTON "Start"
    #define SOURCE_POS_X_AXIS "LeftStickX"
    #define SOURCE_POS_Y_AXIS "LeftStickY"

#endif // RAD_XBOX

#ifdef RAD_GAMECUBE

    #define PLAYBACK_RATE 48000
    #define SOUND_MEMORY 2 * 1024 * 1024
    #define AUX_SENDS 0
    #define SOUND_MAX_ROOT_ALLOCATIONS 8

    #define VIDEO_FILE "sample.rmv"
//    #define AUDIO_TRACK_0 radMovie_NoAudioTrack
    #define AUDIO_TRACK_0 0
    #define AUDIO_TRACK_1 1

    #define PLAY_BUTTON "A"
    #define PAUSE_BUTTON "X"
    #define RELOAD_BUTTON "B"
    #define RELOAD_BUTTON_1 "Y"
    #define QUIT_BUTTON "Menu"
    #define SOURCE_POS_X_AXIS "LeftStickX"
    #define SOURCE_POS_Y_AXIS "LeftStickY"

#endif // RAD_GAMECUBE

#ifdef RAD_WIN32

    #define SOUND_MEMORY 2 * 1024 * 1024
    #define AUX_SENDS 0
    #define SOUND_MAX_ROOT_ALLOCATIONS 8

	#define SOURCE_POS_X_AXIS "XAxis"
	#define SOURCE_POS_Y_AXIS "YAxis"

#endif // RAD_WIN32

enum InputPointIDs
{
    PLAY_POINT,
    STOP_POINT,
    PAUSE_POINT,
	RELOAD_POINT,
	RELOAD_POINT_1,
    QUIT_POINT,
    SOURCE_POS_X_POINT,
    SOURCE_POS_Y_POINT,
    PAN_POINT
};

ref< IRadSoundHalPositionalGroup > g_refIRadSoundHalPositionalGroup = NULL;

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
                break;
            case PAUSE_POINT:
                break;
            case RELOAD_POINT:
                break;
            case RELOAD_POINT_1:
                break;
            case QUIT_POINT:
                break;
			}
		}
        else
        {
		    switch(userData)
			{
                case SOURCE_POS_X_POINT:
                {
                    radSoundVector rsdVector;
                    g_refIRadSoundHalPositionalGroup->GetPosition( & rsdVector );
                    rReleasePrintf( "<- x [%f] z [%f]\n", rsdVector.m_x, rsdVector.m_z );
                    rsdVector.m_x = ( newValue - 0.5f ) * 100.0f;
                    g_refIRadSoundHalPositionalGroup->SetPosition( & rsdVector );
                    rReleasePrintf( "-> x [%f] z [%f]\n", rsdVector.m_x, rsdVector.m_z );
                    break;
                }
                case SOURCE_POS_Y_POINT:
                {
                    radSoundVector rsdVector;
                    g_refIRadSoundHalPositionalGroup->GetPosition( & rsdVector );
                    rReleasePrintf( "<- x [%f] z [%f]\n", rsdVector.m_x, rsdVector.m_z );
                    rsdVector.m_z = ( newValue - 0.5f ) * 100.0f;
                    g_refIRadSoundHalPositionalGroup->SetPosition( & rsdVector );
                    rReleasePrintf( "->x [%f] z [%f]\n", rsdVector.m_x, rsdVector.m_z );
                    break;
                }
            }
        }
	}
};

void positionVectorUpdate( void* userData )
{
	float * pVector = ( float * ) userData;

	if( g_refIRadSoundHalPositionalGroup != NULL )
	{
		radSoundVector soundVector( pVector[ 0 ], pVector[ 1 ], pVector[ 2 ] );
		g_refIRadSoundHalPositionalGroup->SetPosition( & soundVector );
	}
}

int positionalTest( int argc, char * argv[] )
{

    ::rstRadCoreInitialize( );
    ::rstRadSoundInitialize( );

	rReleasePrintf( 
		"\n\n***********************************************************\n"
		" USE THE WATCHER FOR THIS TEST \n"
		"***************************************************************\n\n\n" );

    ref< IRadTextDisplay > refIRadTextDisplay;
    ::radTextDisplayGet( & refIRadTextDisplay, RADMEMORY_ALLOC_DEFAULT );
    refIRadTextDisplay->SetBackgroundColor( 0x000000FF );
    refIRadTextDisplay->TextOutAt( "Positional Surround Sound", 5, 5 );

    ::radSoundHalSystemGet( )->SetOutputMode( radSoundOutputMode_Stereo );
  
    ref< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource_Clip = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundRsdFileDataSource_Clip->InitializeFromFileName( "heli.rsd", 0, IRadSoundHalAudioFormat::Samples, NULL );

    g_refIRadSoundHalPositionalGroup = ::radSoundHalPositionalGroupCreate( RADMEMORY_ALLOC_DEFAULT );

    ref< IRadSoundClip > refIRadSoundClip = ::radSoundClipCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundClip->Initialize( refIRadSoundRsdFileDataSource_Clip, radSoundHalSystemGet( )->GetRootMemoryRegion( ), true, "Test Clip" );
    ref< IRadSoundClipPlayer > refIRadSoundClipPlayer = ::radSoundClipPlayerCreate( RADMEMORY_ALLOC_DEFAULT );

    refIRadSoundClipPlayer->SetClip( refIRadSoundClip );

    refIRadSoundClipPlayer->SetPositionalGroup( g_refIRadSoundHalPositionalGroup );
	g_refIRadSoundHalPositionalGroup->SetMinMaxDistance( 1.0f, 400.0f );

    refIRadSoundClipPlayer->Play( );

    ref< IRadSoundStreamPlayer > refIRadSoundStreamPlayer = ::radSoundStreamPlayerCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundStreamPlayer->InitializeAsync( 1000, IRadSoundHalAudioFormat::Milliseconds, ::radSoundHalSystemGet( )->GetRootMemoryRegion( ), "robert" );

    ref< IRadSoundBufferedDataSource > refIRadSoundBufferedDataSource = ::radSoundBufferedDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundBufferedDataSource->Initialize( radMemorySpace_Local, ::radMemoryGetAllocator( RADMEMORY_ALLOC_DEFAULT ), 3000, IRadSoundHalAudioFormat::Milliseconds, NULL, "robert" );

    ref< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundRsdFileDataSource->InitializeFromFileName( "elli.rsd", 0, IRadSoundHalAudioFormat::Milliseconds, NULL );
    
    refIRadSoundBufferedDataSource->SetInputDataSource( refIRadSoundRsdFileDataSource );
    refIRadSoundStreamPlayer->SetDataSource( refIRadSoundBufferedDataSource );
refIRadSoundStreamPlayer->SetPositionalGroup( g_refIRadSoundHalPositionalGroup );
refIRadSoundStreamPlayer->SetVolume( 1.0f );
//    refIRadSoundStreamPlayer->Play( );

	float Vector[ 3 ];
	Vector[ 0 ] = 0.0f;
	Vector[ 1 ] = 0.0f;
	Vector[ 2 ] = 0.0f;
	::radDbgWatchAddVector( Vector, "positionVector", NULL, positionVectorUpdate, Vector, -500.0f, 500.0f, false );

 
    //
    // Input points
    //

	///////////////////////
	// SET UP CONTROLLER //
	///////////////////////

	
	#ifdef RAD_WIN32
	ref< IRadController > refIRadController ( ::radControllerSystemGet( )->GetControllerAtLocation( "Joystick0" ) );
	#else
    ref< IRadController > refIRadController ( ::radControllerSystemGet( )->GetControllerByIndex( 0 ) );
	#endif // RAD_WIN32

	rAssert( refIRadController != NULL );
	rAssert( refIRadController->IsConnected() );

	#ifdef RAD_WIN32
    ref< IRadControllerInputPoint > refIRcip_Source_X (  refIRadController->GetInputPointByTypeAndIndex( SOURCE_POS_X_AXIS, 0 ) );
    ref< IRadControllerInputPoint > refIRcip_Source_Y (  refIRadController->GetInputPointByTypeAndIndex( SOURCE_POS_Y_AXIS, 0 ) );
	#else
    ref< IRadControllerInputPoint > refIRcip_Source_X (  refIRadController->GetInputPointByName( SOURCE_POS_X_AXIS ) );
    ref< IRadControllerInputPoint > refIRcip_Source_Y (  refIRadController->GetInputPointByName( SOURCE_POS_Y_AXIS ) );
	#endif // RAD_WIN32

	rAssert( refIRcip_Source_X != NULL );
	rAssert( refIRcip_Source_Y != NULL );
//    refIRcip_Source_X->SetRange( -400.0f, 400.0f );
//    refIRcip_Source_Y->SetRange( -400.0f, 400.0f );

    ref< ControllerCallBack > refControllerCallback = new ( RADMEMORY_ALLOC_DEFAULT ) ControllerCallBack();

    refIRcip_Source_X->RegisterControllerInputPointCallback( refControllerCallback, SOURCE_POS_X_POINT );
    refIRcip_Source_Y->RegisterControllerInputPointCallback( refControllerCallback, SOURCE_POS_Y_POINT );


	float z = 0.0f;
    float y = 0.0f;
    float x = 0.0f;
	unsigned int before = ::radTimeGetMilliseconds( );
	unsigned int robert = 0;
    while( g_Done == false )
    {
		unsigned int now = ::radTimeGetMilliseconds( );

		radSoundVector vector( x, y, z );
//		g_refIRadSoundHalPositionalGroup->SetPosition( & vector  );
//		refIRadSoundClipPlayer->SetVolume( z );
        ::rstRadCoreService( );
        ::rstRadSoundService( );     


    }
	
	return 1;
}