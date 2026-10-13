/* main.cpp */

#include <radload/radload.hpp>
#include <radthread.hpp>
#include <radtextdisplay.hpp>
#include <radmemorymonitor.hpp>
#include <radplatform.hpp>
#include <radtime.hpp>
#include <radfile.hpp>
#include <radsound_hal.hpp>
#include <radsound.hpp>
#include <radmusic/radmusic.hpp>
#include <radcontroller.hpp>
#include <raddebugcommunication.hpp>

#ifdef RAD_WIN32	// RAD_WIN32 only
#include "window.hpp"
#endif

#if defined RAD_PS2
	#if !defined PS2MW
    	#include <libsn.h>
    	#include <eetypes.h>
    #else
    	extern "C"
    	{
    		void mwInit(void);
    		void mwExit(void);
    	}
    #endif
#endif

#if defined RAD_GAMECUBE

    #define BUTTON_DOWN   "A"
    #define BUTTON_RIGHT   "X"
    #define BUTTON_UP "Y"
    #define BUTTON_LEFT  "B"

    #define BUTTON_QUIT "Menu"
   
#elif defined (RAD_WIN32) || defined (RAD_XBOX)

    #define BUTTON_DOWN   "A"
    #define BUTTON_RIGHT   "B"
    #define BUTTON_UP "Y"
    #define BUTTON_LEFT  "X"

    #define BUTTON_QUIT "Start"

#elif defined RAD_PS2

    #define BUTTON_DOWN   "X"
    #define BUTTON_RIGHT  "Circle"
    #define BUTTON_UP     "Triangle"
    #define BUTTON_LEFT   "Square"

    #define BUTTON_QUIT "Start"

#endif

#define X_MARGIN 3
#define Y_MARGIN 2

#define UPDATE_TIME_MS 0

struct ControllerPoint
{
    ControllerPoint( IRadControllerInputPoint * pIRcip ) : m_refIRcip( pIRcip ), m_Value( 0 ) { }
    
    bool Poll( void )
    {
        float lastValue = m_Value;
        
        m_Value = m_refIRcip->GetCurrentValue( );

        if ( lastValue < 0.5 && m_Value >= 0.5 )
        {
            return true;
        }

        return false;
    }

    ref< IRadControllerInputPoint > m_refIRcip;
    float m_Value;
};


#ifdef RAD_PS2
	#define RADSOUND_MEMORY 0
#else
	#define RADSOUND_MEMORY 8 * 1024 * 1024 
#endif

#if defined RAD_WIN32
    #define RADSOUND_SAMPLINGRATE 48000
#else
    #define RADSOUND_SAMPLINGRATE 0 // no choice on other platforms       
#endif

#ifdef RAD_GAMECUBE
    #define DRIVE_NAME "DVD:"
#elif defined RAD_PS2
    #ifdef CD_BUILD
        #define DRIVE_NAME "CDROM:"
    #else
        #define DRIVE_NAME "HOSTDRIVE:"
    #endif    
#elif defined RAD_XBOX
    #define DRIVE_NAME "D:"
#elif defined RAD_WIN32
    #define DRIVE_NAME ""
#endif

    
bool g_Done = false;

const char * const FILE_NAME = "art/sample.rms"; // "comp.txt";

void rmsSleep( void )
{
	static unsigned int nextUpdateTime = ::radTimeGetMilliseconds( );

	unsigned int now = ::radTimeGetMilliseconds( );

	if ( now >= nextUpdateTime )
	{
		//::radDbgComService( );
		//::radMemoryMonitorService( );
		::radFileService( );
		::radControllerSystemService( );     	
		::radSoundHalSystemGet( )->Service( );
		::radLoadService( );

		::radSoundHalSystemGet( )->ServiceOncePerFrame( );

		nextUpdateTime = now + UPDATE_TIME_MS;
	}

     //
     // Now perform platform specific processing.
     //

     #if defined RAD_WIN32	// RAD_WIN32 only
 
         MSG msg;

         if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) 
         {
             TranslateMessage(&msg); 

             //
             // We have a message. Normally, we would check for a quit message. We
             // use our own termination system instead.
             //
             DispatchMessage(&msg); 
         } 

     #endif
}

void rmsPaint( IRadTextDisplay * pIRtd, radmusic::performance * p_performance, unsigned int selectedEvent )
{

    #if ! defined RAD_WIN32
    for( unsigned int i = 0; i < radmusic::performance_num_events( p_performance ) ; i ++ )
    {
        char event_name[ 256 ];
        radmusic::performance_event_name( p_performance, i, event_name, 256 );
        char buf[ 256 ];

        sprintf( buf, "%s%s", i == selectedEvent ? "*" : "-",
            event_name );

        pIRtd->TextOutAt( buf, X_MARGIN, Y_MARGIN + i );        
    }
    #endif
}

void rmsMain( void )
{
    unsigned int selectedEvent = 0;

    ref< IRadTextDisplay > refIRadTextDisplay;

    ::radTextDisplayGet( & refIRadTextDisplay, RADMEMORY_ALLOC_DEFAULT );

    refIRadTextDisplay->TextOutAt( "Loading:", X_MARGIN, Y_MARGIN + 0 );
    refIRadTextDisplay->TextOutAt( FILE_NAME, X_MARGIN, Y_MARGIN + 1 );

    // Load the composition
    
    radLoadRequest * request_ptr;
    
    ::radLoadInstance( )->Load( FILE_NAME, & request_ptr );
    
    while ( request_ptr->IsComplete( ) == false )
    {
        rmsSleep( );
    }
        
    radmusic::radload_composition_adapter * p_adapter =
        radLoadFind< radmusic::radload_composition_adapter>( request_ptr->GetInventory( ), (radKey) 0 );
    
    radmusic::composition * p_composition = p_adapter->p_composition;
    p_adapter->p_composition = 0;    

    // create the performance
        
    radmusic::performance * p_performance = radmusic::performance_new(
        p_composition, "art/", radMemorySpace_Local );


	#if defined RAD_WIN32	// RAD_WIN32 only
		ref< IRadController > refIRc = ::radControllerSystemGet( )->GetControllerAtLocation( "Joystick0" );
	#else
		ref< IRadController > refIRc = ::radControllerSystemGet( )->GetControllerByIndex( 0 );
	#endif

	ControllerPoint cpQuit( refIRc->GetInputPointByName( BUTTON_QUIT ) );
    ControllerPoint cpUp( refIRc->GetInputPointByName( BUTTON_UP ) );
    ControllerPoint cpDown( refIRc->GetInputPointByName( BUTTON_DOWN ) );
	ControllerPoint cpRight( refIRc->GetInputPointByName( BUTTON_RIGHT ) );	

    refIRadTextDisplay->Clear( );
    refIRadTextDisplay->SetAutoSwap( false );

    rmsPaint( refIRadTextDisplay, p_performance, selectedEvent );

    float frameTime = 0.0f;
    
    unsigned int last_time = ::radTimeGetMilliseconds( );

	while ( !cpQuit.Poll( ) && ! g_Done )
	{
        unsigned int startTime = ::radTimeGetMicroseconds( );
        unsigned int now = ::radTimeGetMilliseconds( );
        
	    radmusic::performance_update( p_performance, now - last_time );
	    
	    last_time = now;
	            
        if ( cpDown.Poll( ) )
        {
            selectedEvent = ( selectedEvent == radmusic::performance_num_events( p_performance ) - 1 ) ? 0 : selectedEvent + 1;

            rmsPaint( refIRadTextDisplay, p_performance, selectedEvent );
        }
        else if ( cpUp.Poll( ) )
        {
            selectedEvent = ( selectedEvent == 0 ) ? radmusic::performance_num_events( p_performance ) - 1 : selectedEvent - 1;

            rmsPaint( refIRadTextDisplay, p_performance, selectedEvent );
        }
        else if ( cpRight.Poll( ) )
        {
            radmusic::performance_trigger_event( p_performance, selectedEvent );
        }
  
        // #define THRASH

        #ifdef THRASH

            static unsigned int nextThrashTime = ::radTimeGetMilliseconds( );

            unsigned numEvents = refIRmp->GetNumberOfEventControls( );

            if ( numEvents > 0 )
            {
                unsigned int now = ::radTimeGetMilliseconds( );

                if ( now > nextThrashTime )
                {
                    if ( (rand() % 5) == 0 )
                    {
                        switch( rand() % 2 )
                        {
                            case 0 :
                                refIRmp->Stop( );
                                rDebugPrintf( "****> Stopping Engine\n" );
                                break;
                            case 1 :
                                refIRmp->SetPaused( ! refIRmp->GetPaused( ) );
                                rDebugPrintf( "****> SetPaused(%d)\n", refIRmp->GetPaused( ) ? 1 : 0 );
                                break;
                        }
                    }
                    else
                    {
                        ref< IRadMusicControlEvent > refIRmce =
                        refIRmp->GetEventControlByIndex( rand() % numEvents );

                        rDebugPrintf( "****> TriggeringEvent: [%s]\n", refIRmce->GetName( ) );

                        refIRmce->Trigger( );

                        nextThrashTime = ::radTimeGetMilliseconds( ) + rand() % 1000;
                    }
                }
            }

        #endif
            				
		rmsSleep( );

        float time = (float) ::radTimeGetMicroseconds( ) - startTime;

        if ( time <= 0.0f )
        {
            time = 1.0f; // avoid div by zero
        }


        if ( time > frameTime )
        {
            frameTime = time;
        }
        else
        {
            frameTime = frameTime * 0.99f;

            if ( frameTime < time )
            {
                frameTime = time;
            }
        }
        
        #if !defined RAD_WIN32

            char frameBuf[ 256 ];

            float beat;
            
            (void) radmusic::debug_performance_current_beat( p_performance, & beat );

            sprintf( frameBuf, "Beat: [%.2f]",
                radSoundFloatToDouble( beat ) );

            refIRadTextDisplay->TextOutAt( frameBuf, X_MARGIN, Y_MARGIN + 13);
        
            sprintf( frameBuf, "Peak  : [%5.0f]fps [%5.2f]mspf",
                radSoundFloatToDouble( 1000000.0f / frameTime ),
                radSoundFloatToDouble( frameTime / 1000.0f ) );

            refIRadTextDisplay->TextOutAt( frameBuf, X_MARGIN, Y_MARGIN +14 );

            sprintf( frameBuf, "Single: [%5.0f]fps [%5.2f]mspf",
                radSoundFloatToDouble( 1000000.0f / time ),
                radSoundFloatToDouble( time / 1000.0f ) );


            refIRadTextDisplay->TextOutAt( frameBuf, X_MARGIN, Y_MARGIN + 15 );

            refIRadTextDisplay->SwapBuffers( );
        #endif
	}
	
	radmusic::performance_delete( & p_performance );
    radmusic::composition_delete( & p_composition );
}


void rmsOutputHandler( const char * pMsg )
{
}

#if defined RAD_WIN32	// RAD_WIN32 only
	int WINAPI WinMain(
	  HINSTANCE hInstance,  // handle to current instance
	  HINSTANCE hPrevInstance,  // handle to previous instance
	  LPSTR lpCmdLine,      // pointer to command line
	  int nCmdShow          // show state of window
	) 
#else
	int main( void )
#endif
{
	#ifdef PS2MW
		mwInit( );
	#endif

    ::radThreadInitialize( );
    
    #ifdef RAD_GAMECUBE
        ::radMemoryInitialize( 0, 0 );
    #else		
        ::radMemoryInitialize( );
    #endif
    #if defined RAD_WIN32	// RAD_WIN32 only
	    ::radPlatformInitialize( ::CreateMainWindow( hInstance, SW_SHOWNORMAL ), hInstance, RADMEMORY_ALLOC_DEFAULT );
    #elif defined RAD_GAMECUBE || defined RAD_XBOX
        ::radPlatformInitialize( );
    #else
	    ::radPlatformInitialize( "irx/", IOPMediaHost, GameMediaCD, RADPLATFORM_IOP_IMG_FILENAME, RADMEMORY_ALLOC_DEFAULT );
    #endif

    /* #if defined RAD_PS2 && !defined PS2MW

       rReleaseString( "Sn Profile Enabled\n" );

        snDebugInit();

        u_long128 * pSnProfileBuffer = ( u_long128 *) radMemoryAlloc( 
            RADMEMORY_ALLOC_DEFAULT, 2048 * sizeof( u_long128 ) );

        ::radPlatformGet( )->LoadIrxModule( "SNProfil.irx", 0, 0 );

        if( snProfInit( _4KHZ, pSnProfileBuffer, 2048 * sizeof( u_long128 ) ) != 0 )
        {             
            rReleaseString( "Profiler init failed\n" );
        }

    #endif */

	::radTimeInitialize( );
    //::radMemoryMonitorInitialize( 50 * 1024, true );

    /* #if defined RAD_XBOX || defined RAD_WIN32
        ::radDbgComTargetInitialize( WinSocket );
    #elif defined RAD_GAMECUBE
        radDbgComTargetInitialize( HostIO );
    #elif defined RAD_PS2
        radDbgComTargetInitialize( Deci );
    #endif */

	::radFileInitialize( 100, 100, RADMEMORY_ALLOC_DEFAULT );
	::radSetDefaultDrive( DRIVE_NAME );
	::radDriveMount( );

	::radSoundHalSystemInitialize( RADMEMORY_ALLOC_DEFAULT );
	IRadSoundHalSystem::SystemDescription s;

    #ifndef RAD_PS2	
	    s.m_ReservedSoundMemory = RADSOUND_MEMORY;
	#endif
	
	#ifdef RAD_WIN32
	s.m_SamplingRate        = RADSOUND_SAMPLINGRATE;
	#endif	
	
	s.m_MaxRootAllocations = 10;
	s.m_NumAuxSends         = 2;

	::radSoundHalSystemGet( )->Initialize( s );
	::radControllerInitialize( NULL, RADMEMORY_ALLOC_DEFAULT );	
    rDebugChannelInitialize( RADMEMORY_ALLOC_DEFAULT );
    //rDebugSetOutputHandler( rmsOutputHandler );
        
    //rDebugChannelEnable( radmusic::debug_channel );
    //rDebugChannelEnable( radSoundDebugChannel );
    //rDebugChannelEnable( radFileDebugChannel );
    radLoadInitialize( );
    radmusic::initialize( NULL, 0, 34, RADMEMORY_ALLOC_DEFAULT );
    radmusic::register_radload_loaders( );

	srand( 10 );

	rmsMain( );
	
	radmusic::terminate( );
	
	radLoadTerminate( );
    rDebugChannelTerminate( );
	::radControllerTerminate( );
	::radSoundHalSystemTerminate( );
    ::radDriveUnmount( );
	::radFileTerminate( );
    //::radMemoryMonitorTerminate( );
    //radDbgComTargetTerminate( );
	::radTimeTerminate( );
	::radPlatformTerminate( );
    ::radMemoryTerminate( );
    ::radThreadTerminate( );

#ifdef RAD_DEBUG
	::radObject::DumpObjects( );
#endif

	#ifdef PS2MW
		mwExit( );
	#endif
	return 0;
}