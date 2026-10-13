//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include "initradcore.hpp"
#include "initradsound.hpp"

#include <radcontroller.hpp>
#include <radtextdisplay.hpp>
#include <radsound.hpp>
#include <raddebugwatch.hpp>
#include "effects.hpp"

#ifdef RAD_WIN32
#include <windows.h>
#include <radsound_win32.hpp>
#endif

#ifdef RAD_XBOX
#include <radsound_xbox.hpp>
#endif

#ifdef RAD_PS2
#include <radsound_ps2.hpp>
#endif 

#ifdef RAD_GAMECUBE
#include <radsound_gcn.hpp>
#endif

extern bool g_Done;

#ifndef RAD_WIN32

    #define SOURCE_POS_X_AXIS "LeftStickX"
    #define SOURCE_POS_Y_AXIS "LeftStickY"

#else

	#define SOURCE_POS_X_AXIS "XAxis"
	#define SOURCE_POS_Y_AXIS "YAxis"

#endif // RAD_WIN32

enum InputPointIDs
{
    SOURCE_POS_X_POINT,
    SOURCE_POS_Y_POINT
};

static ref< IRadSoundHalPositionalGroup > g_refIRadSoundHalPositionalGroup = NULL;

//=============================================================================
// Struct ControllerCallback
//=============================================================================

struct ControllerCallBack : public IRadControllerInputPointCallback, radRefCount
{
    IMPLEMENT_REFCOUNTED( "ControllerCallBack" )

    ControllerCallBack( void ) : radRefCount( 0 ) { }

	void OnControllerInputPointChange( unsigned int userData, float newValue)
    {
		switch(userData)
		{
            case SOURCE_POS_X_POINT:
            {
                radSoundVector rsdVector;
                g_refIRadSoundHalPositionalGroup->GetPosition( & rsdVector );
                rsdVector.m_x = ( newValue - 0.5f ) * 5.0f;
                g_refIRadSoundHalPositionalGroup->SetPosition( & rsdVector );
                break;
            }
            case SOURCE_POS_Y_POINT:
            {
                radSoundVector rsdVector;
                g_refIRadSoundHalPositionalGroup->GetPosition( & rsdVector );
                rsdVector.m_z = ( newValue - 0.5f ) * 5.0f;
                g_refIRadSoundHalPositionalGroup->SetPosition( & rsdVector );
                break;
            }
        }
	}
};

static void positionVectorUpdate( void* userData )
{
	float * pVector = ( float * ) userData;

	if( g_refIRadSoundHalPositionalGroup != NULL )
	{
		radSoundVector soundVector( pVector[ 0 ], pVector[ 1 ], pVector[ 2 ] );
		g_refIRadSoundHalPositionalGroup->SetPosition( & soundVector );
	}
}

static int g_Room, g_RoomHF, g_Reflections, g_Reverb;
static float g_RoomRolloffFactor, g_DecayTime, g_DecayHFRatio,
             g_ReflectionsDelay, g_ReverbDelay, g_Diffusion,
             g_Density, g_HFReference;


static unsigned int g_Ps2Mode;
static float        g_Ps2Gain;
static bool         g_Ps2Enabled;


static bool  g_GcnEnabled   ;
static float g_GcnGain      ; 
static float g_GcnPreDelay  ;
static float g_GcnReverbTime;
static float g_GcnColoration;
static float g_GcnDamping   ;

static unsigned int g_AuxSend = 1;

static bool g_EnableAutoChangeEffectValues, g_EnableDryMusic;

ref< IRadSoundStreamPlayer > g_refIRadSoundStreamPlayer = NULL;

#ifdef RAD_WIN32

ref< IRadSoundHalEffectEAX2Reverb > g_refIRadSoundHalEffect0 = NULL;
ref< IRadSoundHalEffectEAX2Reverb > g_refIRadSoundHalEffect1 = NULL;
ref< IRadSoundHalEffectEAX2Reverb > g_refICurrentEffect = NULL;

#endif // RAD_WIN32

#ifdef RAD_XBOX

ref< IRadSoundHalEffectI3DL2ReverbXBox > g_refIRadSoundHalEffect0 = NULL;
ref< IRadSoundHalEffectI3DL2ReverbXBox > g_refIRadSoundHalEffect1 = NULL;
ref< IRadSoundHalEffectI3DL2ReverbXBox > g_refICurrentEffect = NULL;

#endif

#ifdef RAD_PS2

ref< IRadSoundEffectPs2 > g_refIRadSoundHalEffect0 = NULL;
ref< IRadSoundEffectPs2 > g_refIRadSoundHalEffect1 = NULL;
ref< IRadSoundEffectPs2 > g_refICurrentEffect = NULL;

#endif 

#ifdef RAD_GAMECUBE

ref< IRadSoundEffectStdReverbGcn > g_refIRadSoundHalEffect0 = NULL;
ref< IRadSoundEffectStdReverbGcn > g_refIRadSoundHalEffect1 = NULL;
ref< IRadSoundEffectStdReverbGcn > g_refICurrentEffect = NULL;

#endif 

static void UpdateEffectsSettings( void )
{
    if( g_refICurrentEffect != NULL )
    {
        #if defined RAD_WIN32 || defined RAD_XBOX
        g_refICurrentEffect->SetRoom(              g_Room              );
        g_refICurrentEffect->SetRoomHF(            g_RoomHF            );
        g_refICurrentEffect->SetRoomRolloffFactor( g_RoomRolloffFactor );
        g_refICurrentEffect->SetDecayTime(         g_DecayTime         );
        g_refICurrentEffect->SetDecayHFRatio(      g_DecayHFRatio      );
        g_refICurrentEffect->SetReflections(       g_Reflections       );
        g_refICurrentEffect->SetReflectionsDelay(  g_ReflectionsDelay  );
        g_refICurrentEffect->SetReverb(            g_Reverb            );
        g_refICurrentEffect->SetReverbDelay(       g_ReverbDelay       );
        #elif defined RAD_PS2
        g_refICurrentEffect->SetEnabled( g_Ps2Enabled );
        g_refICurrentEffect->SetMode( ( IRadSoundEffectPs2::Mode ) g_Ps2Mode );
        g_refICurrentEffect->SetGain( g_Ps2Gain );
        #else 
        g_refICurrentEffect->SetEnabled(    g_GcnEnabled    );
        g_refICurrentEffect->SetGain(       g_GcnGain       );
        g_refICurrentEffect->SetPreDelay(   g_GcnPreDelay   ); // 0 to 100
        g_refICurrentEffect->SetReverbTime( g_GcnReverbTime ); // 100 to 10,000
        g_refICurrentEffect->SetColoration( g_GcnColoration ); // 0.0f to 1.0f 
        g_refICurrentEffect->SetDamping(    g_GcnDamping    ); // 0.0f to 1.0f
        #endif
    }
}

static void ToggleDryMusic( void )
{
    g_EnableDryMusic = ! g_EnableDryMusic;

    if( g_EnableDryMusic == true )
    {
        g_refIRadSoundStreamPlayer->Play( );
    }
    else
    {
        g_refIRadSoundStreamPlayer->Stop( );
    }
}

static void SwitchEffect( void )
{
    ref< IRadSoundHalEffect > refIEffect = ::radSoundHalSystemGet( )->GetAuxEffect( g_AuxSend );

    if( refIEffect == g_refIRadSoundHalEffect0 )
    {
        g_refICurrentEffect = g_refIRadSoundHalEffect1;
    }
    else
    {
        g_refICurrentEffect = g_refIRadSoundHalEffect0;
    }

    if( g_refICurrentEffect != NULL )
    {
        #if defined RAD_WIN32 || defined RAD_XBOX
        g_Room               = g_refICurrentEffect->GetRoom( );
        g_RoomHF             = g_refICurrentEffect->GetRoomHF( );
        g_RoomRolloffFactor  = g_refICurrentEffect->GetRoomRolloffFactor( );
        g_DecayTime          = g_refICurrentEffect->GetDecayTime( );
        g_DecayHFRatio       = g_refICurrentEffect->GetDecayHFRatio( );
        g_Reflections        = g_refICurrentEffect->GetReflections( );
        g_ReflectionsDelay   = g_refICurrentEffect->GetReflectionsDelay( );
        g_Reverb             = g_refICurrentEffect->GetReverb( );
        g_ReverbDelay        = g_refICurrentEffect->GetReverbDelay( );
        #elif defined RAD_PS2
        g_Ps2Enabled         = g_refICurrentEffect->GetEnabled( );
        g_Ps2Mode            = g_refICurrentEffect->GetMode( );
        g_Ps2Gain            = g_refICurrentEffect->GetGain( );
        #else
        g_GcnEnabled         = g_refICurrentEffect->GetEnabled( );
        g_GcnGain            = g_refICurrentEffect->GetGain( );
        g_GcnPreDelay        = g_refICurrentEffect->GetPreDelay( );
        g_GcnReverbTime      = g_refICurrentEffect->GetReverbTime( );
        g_GcnColoration      = g_refICurrentEffect->GetColoration( );
        g_GcnDamping         = g_refICurrentEffect->GetDamping( );
        #endif
    }
    ::radSoundHalSystemGet( )->SetAuxEffect( g_AuxSend, g_refICurrentEffect );
}

int effectsTest( int argc, char * argv[] )
{

    ::rstRadCoreInitialize( );
    ::rstRadSoundInitialize( );

    ref< IRadTextDisplay > refIRadTextDisplay;
    ::radTextDisplayGet( & refIRadTextDisplay, RADMEMORY_ALLOC_DEFAULT );
    refIRadTextDisplay->SetBackgroundColor( 0x000000FF );
    refIRadTextDisplay->TextOutAt( "Sound Effects...", 5, 5 );

    // Do the effect thing

    #ifdef RAD_WIN32
    
    g_refIRadSoundHalEffect0 = ::radSoundHalEffectEAX2ReverbCreate( RADMEMORY_ALLOC_DEFAULT );
    g_refIRadSoundHalEffect1 = ::radSoundHalEffectEAX2ReverbCreate( RADMEMORY_ALLOC_DEFAULT );
    g_refICurrentEffect = g_refIRadSoundHalEffect0;

    #endif // RAD_WIN32

    #ifdef RAD_XBOX
    
    g_refIRadSoundHalEffect0 = ::radSoundHalEffectI3DL2ReverbXBoxCreate( RADMEMORY_ALLOC_DEFAULT );
    g_refIRadSoundHalEffect1 = ::radSoundHalEffectI3DL2ReverbXBoxCreate( RADMEMORY_ALLOC_DEFAULT );
    g_refICurrentEffect = g_refIRadSoundHalEffect0;

    #endif

    #ifdef RAD_PS2
    
    g_refIRadSoundHalEffect0 = ::radSoundCreateEffectPs2( RADMEMORY_ALLOC_DEFAULT );
    g_refIRadSoundHalEffect1 = ::radSoundCreateEffectPs2( RADMEMORY_ALLOC_DEFAULT );
    g_refICurrentEffect = g_refIRadSoundHalEffect0;

    #endif // RAD_PS2

    #ifdef RAD_GAMECUBE

    g_refIRadSoundHalEffect0 = ::radSoundEffectStdReverbGcnCreate( RADMEMORY_ALLOC_DEFAULT );
    g_refIRadSoundHalEffect1 = NULL;//::radSoundEffectStdReverbGcnCreate( RADMEMORY_ALLOC_DEFAULT );
    g_refICurrentEffect = g_refIRadSoundHalEffect0;

    #endif // RAD_GAMECUBE

    ::radSoundHalListenerGet( )->SetEnvEffectsEnabled( true );
    ::radSoundHalListenerGet( )->SetEnvironmentAuxSend( g_AuxSend );
    ::radSoundHalSystemGet( )->SetAuxEffect( g_AuxSend, g_refICurrentEffect );
    ::radSoundHalSystemGet( )->SetAuxGain( g_AuxSend, 1.0f );

//    ::radSoundHalSystemGet( )->SetOutputMode( radSoundOutputMode_Surround );

    g_refIRadSoundHalPositionalGroup = ::radSoundHalPositionalGroupCreate( RADMEMORY_ALLOC_DEFAULT );
   
    //

    /*
    ref< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource_Clip0 = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundRsdFileDataSource_Clip0->InitializeFromFileName( "MaxPosF009a.rsd", 0, IRadSoundHalAudioFormat::Samples, NULL );

    ref< IRadSoundClip > refIRadSoundClip0 = ::radSoundClipCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundClip0->Initialize( refIRadSoundRsdFileDataSource_Clip0, radSoundHalSystemGet( )->GetRootMemoryRegion( ), true, "Test Clip" );
    ref< IRadSoundClipPlayer > refIRadSoundClipPlayer0 = ::radSoundClipPlayerCreate( RADMEMORY_ALLOC_DEFAULT );

    refIRadSoundClipPlayer0->SetClip( refIRadSoundClip0 );

    refIRadSoundClipPlayer0->SetPositionalGroup( g_refIRadSoundHalPositionalGroup );
	g_refIRadSoundHalPositionalGroup->SetMinMaxDistance( 1.0f, 400.0f );

    refIRadSoundClipPlayer0->Play( );

    refIRadSoundClipPlayer0->SetAuxGain( 0, 1.0f );
    refIRadSoundClipPlayer0->SetAuxGain( 1, 1.0f );
    refIRadSoundClipPlayer0->SetAuxMode( 0, radSoundAuxMode_PostFader );
    refIRadSoundClipPlayer0->SetAuxMode( 1, radSoundAuxMode_PostFader );

    //

    ref< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource_Clip = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundRsdFileDataSource_Clip->InitializeFromFileName( "heli.rsd", 0, IRadSoundHalAudioFormat::Samples, NULL );

    ref< IRadSoundClip > refIRadSoundClip = ::radSoundClipCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundClip->Initialize( refIRadSoundRsdFileDataSource_Clip, radSoundHalSystemGet( )->GetRootMemoryRegion( ), true, mode, "Test Clip" );
    ref< IRadSoundClipPlayer > refIRadSoundClipPlayer = ::radSoundClipPlayerCreate( RADMEMORY_ALLOC_DEFAULT );

    refIRadSoundClipPlayer->SetClip( refIRadSoundClip );

    refIRadSoundClipPlayer->SetPositionalGroup( g_refIRadSoundHalPositionalGroup );
	g_refIRadSoundHalPositionalGroup->SetMinMaxDistance( 1.0f, 400.0f );

    refIRadSoundClipPlayer->Play( );

    refIRadSoundClipPlayer->SetAuxGain( 0, 1.0f );
    refIRadSoundClipPlayer->SetAuxGain( 1, 1.0f );
    refIRadSoundClipPlayer->SetAuxMode( 0, radSoundAuxMode_PostFader );
    refIRadSoundClipPlayer->SetAuxMode( 1, radSoundAuxMode_PostFader );

    */

    //

    g_refIRadSoundStreamPlayer = ::radSoundStreamPlayerCreate( RADMEMORY_ALLOC_DEFAULT );
    g_refIRadSoundStreamPlayer->InitializeAsync( 1000, IRadSoundHalAudioFormat::Milliseconds, ::radSoundHalSystemGet( )->GetRootMemoryRegion( ), "robert" );

    ref< IRadSoundBufferedDataSource > refIRadSoundBufferedDataSource = ::radSoundBufferedDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundBufferedDataSource->Initialize( radMemorySpace_Local, ::radMemoryGetAllocator( RADMEMORY_ALLOC_DEFAULT ), 3000, IRadSoundHalAudioFormat::Milliseconds, NULL, "robert" );

    ref< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundRsdFileDataSource->InitializeFromFileName( "elli.rsd", 0, IRadSoundHalAudioFormat::Milliseconds, NULL );
    
    refIRadSoundBufferedDataSource->SetInputDataSource( refIRadSoundRsdFileDataSource );
    g_refIRadSoundStreamPlayer->SetDataSource( refIRadSoundBufferedDataSource );
//    refIRadSoundStreamPlayer->SetPositionalGroup( g_refIRadSoundHalPositionalGroup );
    g_refIRadSoundStreamPlayer->SetVolume( 1.0f );
//    g_refIRadSoundStreamPlayer->Play( );

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

	///////////////////////
	// ADD SOME WATCHER  //
	///////////////////////

    g_EnableAutoChangeEffectValues = true;
    g_EnableDryMusic = false;

    ::radDbgWatchAddFunction( "UpdateEffectParams", (RADDEBUGWATCH_CALLBACK ) UpdateEffectsSettings );
    ::radDbgWatchAddFunction( "ToggleDryMusic", (RADDEBUGWATCH_CALLBACK ) ToggleDryMusic );
    ::radDbgWatchAddFunction( "SwitchEffect", (RADDEBUGWATCH_CALLBACK ) SwitchEffect );
    ::radDbgWatchAddBoolean( & g_EnableAutoChangeEffectValues, "AutoChangeSettings" );


    #if defined RAD_WIN32 || defined RAD_XBOX
        if( g_refICurrentEffect != NULL )
        {
            g_Room               = g_refICurrentEffect->GetRoom( );
            g_RoomHF             = g_refICurrentEffect->GetRoomHF( );
            g_RoomRolloffFactor  = g_refICurrentEffect->GetRoomRolloffFactor( );
            g_DecayTime          = g_refICurrentEffect->GetDecayTime( );
            g_DecayHFRatio       = g_refICurrentEffect->GetDecayHFRatio( );
            g_Reflections        = g_refICurrentEffect->GetReflections( );
            g_ReflectionsDelay   = g_refICurrentEffect->GetReflectionsDelay( );
            g_Reverb             = g_refICurrentEffect->GetReverb( );
            g_ReverbDelay        = g_refICurrentEffect->GetReverbDelay( );
        }

        ::radDbgWatchAddInt( & g_Room, "Room" );
        ::radDbgWatchAddInt( & g_RoomHF, "RoomHF" );   
        ::radDbgWatchAddFloat( & g_RoomRolloffFactor, "RoomRolloffFactor", NULL, NULL, NULL, -30000.0f, 30000.0f );
        ::radDbgWatchAddFloat( & g_DecayTime, "DecayTime", NULL, NULL, NULL, -30000.0f, 30000.0f );
        ::radDbgWatchAddFloat( & g_DecayHFRatio, "DecayHFRatio", NULL, NULL, NULL, -30000.0f, 30000.0f );
        ::radDbgWatchAddInt( & g_Reflections, "Reflections" );
        ::radDbgWatchAddFloat( & g_ReflectionsDelay, "ReflectionsDelay", NULL, NULL, NULL, -30000.0f, 30000.0f );
        ::radDbgWatchAddInt( & g_Reverb, "Reverb" );
        ::radDbgWatchAddFloat( & g_ReverbDelay, "ReverbDelay", NULL, NULL, NULL, -30000.0f, 30000.0f );
        ::radDbgWatchAddFloat( & g_Diffusion, "Diffusion", NULL, NULL, NULL, -30000.0f, 30000.0f );
        ::radDbgWatchAddFloat( & g_Density, "Density", NULL, NULL, NULL, -30000.0f, 30000.0f );
        ::radDbgWatchAddFloat( & g_HFReference, "HFReference", NULL, NULL, NULL, -30000.0f, 30000.0f );

    #endif // RAD_WIN32 || RAD_XBOX

    #ifdef RAD_PS2

        if( g_refICurrentEffect != NULL )
        {
            g_Ps2Enabled = g_refICurrentEffect->GetEnabled( );
            g_Ps2Mode =    g_refICurrentEffect->GetMode( );
            g_Ps2Gain =    g_refICurrentEffect->GetGain( );
        }

        ::radDbgWatchAddBoolean( & g_Ps2Enabled, "Enabled" );
        ::radDbgWatchAddUnsignedInt( & g_Ps2Mode, "Mode" );
        ::radDbgWatchAddFloat( & g_Ps2Gain, "Gain", NULL, NULL, NULL, 0.0f, 1.0f );

    #endif // RAD_PS2

    #ifdef RAD_GAMECUBE
        if( g_refICurrentEffect != NULL )
        {
            g_GcnEnabled         = g_refICurrentEffect->GetEnabled( );
            g_GcnGain            = g_refICurrentEffect->GetGain( );
            g_GcnPreDelay        = g_refICurrentEffect->GetPreDelay( );
            g_GcnReverbTime      = g_refICurrentEffect->GetReverbTime( );
            g_GcnColoration      = g_refICurrentEffect->GetColoration( );
            g_GcnDamping         = g_refICurrentEffect->GetDamping( );
        }
        ::radDbgWatchAddBoolean( & g_GcnEnabled, "Enabled" );
        ::radDbgWatchAddFloat( & g_GcnGain, "Gain", NULL, NULL, NULL, 0.0f, 1.0f );
        ::radDbgWatchAddFloat( & g_GcnPreDelay, "PreDelay", NULL, NULL, NULL, 0.0f, 100.0f );
        ::radDbgWatchAddFloat( & g_GcnReverbTime, "ReverbTime", NULL, NULL, NULL, 100.0f, 10000.0f );
        ::radDbgWatchAddFloat( & g_GcnColoration, "Colouration", NULL, NULL, NULL, 0.0f, 1.0f );
        ::radDbgWatchAddFloat( & g_GcnDamping, "Damping", NULL, NULL, NULL, 0.0f, 1.0f );

    #endif // RAD_GAMECUBE

	float z = 0.0f;
    float y = 0.0f;
    float x = 0.0f;
	unsigned int before = ::radTimeGetMilliseconds( );


    for( unsigned int robert = 0; robert < 1; robert++ )
    {
        ref< IRadSoundRsdFileDataSource > refIRadSoundRsdFileDataSource_Clip0 = ::radSoundRsdFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
        refIRadSoundRsdFileDataSource_Clip0->InitializeFromFileName( "MaxPosF009a.rsd", 0, IRadSoundHalAudioFormat::Samples, NULL );

        ref< IRadSoundClip > refIRadSoundClip0 = ::radSoundClipCreate( RADMEMORY_ALLOC_DEFAULT );
        refIRadSoundClip0->Initialize( refIRadSoundRsdFileDataSource_Clip0, radSoundHalSystemGet( )->GetRootMemoryRegion( ), true, "Test Clip" );
        ref< IRadSoundClipPlayer > refIRadSoundClipPlayer0 = ::radSoundClipPlayerCreate( RADMEMORY_ALLOC_DEFAULT );

        refIRadSoundClipPlayer0->SetClip( refIRadSoundClip0 );

        refIRadSoundClipPlayer0->SetPositionalGroup( g_refIRadSoundHalPositionalGroup );
        g_refIRadSoundHalPositionalGroup->SetMinMaxDistance( 1.0f, 400.0f );

        refIRadSoundClipPlayer0->Play( );

//        refIRadSoundClipPlayer0->SetAuxGain( 0, 1.0f );
//        refIRadSoundClipPlayer0->SetAuxGain( 1, 1.0f );
//        refIRadSoundClipPlayer0->SetAuxMode( 0, radSoundAuxMode_PostFader );
//        refIRadSoundClipPlayer0->SetAuxMode( 1, radSoundAuxMode_PostFader );

        while( refIRadSoundClipPlayer0->GetState( ) != IRadSoundClipPlayer::Playing )
        {
            ::rstRadCoreService( );
            ::rstRadSoundService( );     
        }


        refIRadSoundClipPlayer0->AddRef( );
    }

    while( g_Done == false )
    {

        ::radThreadSleep( 33 );

		unsigned int now = ::radTimeGetMilliseconds( );

//		radSoundVector vector( x, y, z );
//		g_refIRadSoundHalPositionalGroup->SetPosition( & vector  );
//		refIRadSoundClipPlayer->SetVolume( z );
        ::rstRadCoreService( );
        ::rstRadSoundService( );     

        if( true == g_EnableAutoChangeEffectValues )
        {
            if( ( now - before ) > 30 )
            {
                before = now;
                
                /*
                #if defined RAD_XBOX || RAD_WIN32
                 float value = 5000.0f * ( sinf( ( float ) now / 1500.0f ) - 1.0f ); // "room"
                //float value = 10.05f + ( 9.95f * ( sinf( ( float ) now / 1500.0f ) ) ); // "room rolloff factor 
                #else
                float value = 0.5f * ( sinf( ( float ) now / 1500.0f ) + 1.0f );
                #endif 
                */


                float value = ( sinf( ( float ) now / 1500.0f ) + 1.0f ) / 2.0f;

//                rReleasePrintf( "VALUE [%f]\n", value );


                 if( g_refICurrentEffect != NULL )
                 {
                     ::radSoundHalSystemGet( )->SetAuxGain( g_AuxSend, value );

                    /*

                    #if defined RAD_XBOX || RAD_WIN32
                    g_refICurrentEffect->SetRoom( ( int ) value );
                    #else
                    g_refICurrentEffect->SetGain( ( float ) value );
                    #endif
                    */
                 }

            }
        }
    }
	
	return 1;
}