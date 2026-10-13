//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        platform.cpp
//
// Subsystem:	Foundation Technologies - platform object
//
// Description:	This file contains the implementation of the plafrom services.
//
// Revisions:   Mar 13, 2001 Creation
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================


#include "pch.hpp"
#include <string.h>
#include <radobject.hpp>
#include <radmemory.hpp>
#include <radmemorymonitor.hpp>
#include <radplatform.hpp>
#include <radstring.hpp>
#include <radobjectlist.hpp>

#ifdef RAD_WIN32
#include <SDL.h>
#ifdef WIN32
#include <windows.h>
#if SDL_MAJOR_VERSION < 3
#include <SDL_syswm.h>
#endif
#endif
#endif 
#ifdef RAD_XBOX
#include <xtl.h>
#endif 

#ifdef RAD_PS2
#include <sifdev.h>
#include <sifrpc.h>
#include <libcdvd.h>
#endif

#ifdef RAD_GAMECUBE
#include <dolphin.h>
#include <dolphin/arq.h>
#endif

#undef NULL
#define NULL 0

//============================================================================
// Constants
//============================================================================

#define VersionString "Foundation Technologies - RadCore - Version 3.01\n"
   
//============================================================================
// Class radPlatform
//============================================================================
// The following class is constructed as a global singleton object. It is 
// not destroyed since the memory system is not in place when contructed.
// The reference counting is not employed other than to catch problems
// releasing it.
//============================================================================

class radPlatform : public IRadPlatform
{
    public:

    // Don't write a constructor for this class, Construct() will get
    // called by the memory system before global constructors are called.

    ~radPlatform( void )
    {
    }

    void* operator new( size_t size, void* p )
    {
        return( p );
    }    	

    void Construct( radMemoryAllocator allocator )
    {
        rDebugString( VersionString );

        #ifdef RAD_WIN32        
             m_pMainWindow = NULL;
        #endif

        m_RefCount = 0;
        m_Initialized = false;
        m_ThisAllocator = allocator;
    }
    
    //
    // Add reference and delete do nothing except update reference count.
    //

    void AddRef( void )
    {
        rWarningMsg( m_Initialized, "radPlatform has not been initialized");
        m_RefCount++;
    }
            
    void Release( void )
    {
        rAssert( m_RefCount != 0 );
        m_RefCount--;
    }

#ifdef RAD_WIN32

    //
    // Windows specific interfaces.
    //
#if SDL_MAJOR_VERSION < 3
    static int SDLCALL MainWindowProcedure
#else
    static bool SDLCALL MainWindowProcedure
#endif
    (
        void* userdata, SDL_Event* event
    );

    void Initialize( SDL_Window* pMainWindow, radMemoryAllocator allocator )
    {
        rAssertMsg( !m_Initialized, "radPlatform already initialized");

        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radPlatform" );

        m_ThisAllocator = allocator;
        m_Initialized = true;
        m_pMainWindow = pMainWindow;
		
        rWarningMsg( m_pMainWindow != NULL, "hMainWindow set to NULL in platform component" );

#if defined(WIN32) && SDL_MAJOR_VERSION < 3
        SDL_VERSION( &m_wmInfo.version );
        SDL_GetWindowWMInfo( pMainWindow, &m_wmInfo );
#endif
    }

    void Terminate( void  )
    {
        rAssertMsg( m_Initialized, "radPlatform not initialized");
        rAssertMsg( m_RefCount == 0, "radPlatorm still in use" );
        m_Initialized = false;
    }

#ifdef WIN32
	virtual HWND GetMainWindowHandle( void )
    {
#if SDL_MAJOR_VERSION < 3
        rWarningMsg( m_wmInfo.subsystem == SDL_SYSWM_WINDOWS, "WM info doesn't match the windows subsystem." );

        return( m_wmInfo.info.win.window );
#else
        return (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(m_pMainWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif
    }
    virtual HINSTANCE GetInstanceHandle( void )
    {
#if SDL_MAJOR_VERSION < 3
        return( m_wmInfo.info.win.hinstance );
#else
        return (HINSTANCE)SDL_GetPointerProperty(SDL_GetWindowProperties(m_pMainWindow), SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, NULL);
#endif
    }
#endif

    virtual void RegisterMainWindowCallback( IRadPlatformWin32MessageCallback* pICallback )
    {
        rAssert( pICallback != NULL );

        SDL_AddEventWatch( MainWindowProcedure, pICallback );

    }

    virtual void UnRegisterMainWindowCallback( IRadPlatformWin32MessageCallback* pICallback )
    {
        rAssert( pICallback != NULL );

#if SDL_MAJOR_VERSION < 3
        SDL_DelEventWatch( MainWindowProcedure, pICallback );
#else
        SDL_RemoveEventWatch( MainWindowProcedure, pICallback );
#endif
    }

    private:    
    
    SDL_Window* m_pMainWindow;
#if defined(WIN32) && SDL_MAJOR_VERSION < 3
    SDL_SysWMinfo m_wmInfo;
#endif

#endif
#ifdef RAD_XBOX
    //
    // XBOX implementation
    //
    void Initialize( void )
    {
        rAssertMsg( !m_Initialized, "radPlatform already initialized");
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radPlatform" );

        //
        // Initialize the XBox peripheral control software 
        // **This is the default call and will create 4 game controllers, 2 memory units
        // and one voice controller.
        //
        XInitDevices( 0, NULL );

        m_Initialized = true;
    }

    void Terminate( void )
    {
        rAssertMsg( m_Initialized, "radPlatform not initialized");
        rAssertMsg( m_RefCount == 0, "radPlatorm still in use" );
        m_Initialized = false;
    }

    private:

#endif

#ifdef RAD_PS2
    //
    // PS2 Implementaition
    //
    void Initialize( const char* pIrxPath, radPlatformIOPMedia IOPMedia, radPlatformGameMediaType MediaType, const char* pIopImage, radMemoryAllocator allocator )
    {
        rAssertMsg( !m_Initialized, "radPlatform already initialized");
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radPlatform" );
        m_Initialized = true;
        m_ThisAllocator = allocator;

        ::radObjectListCreate( & m_xIOl_LoadedIrxModules, m_ThisAllocator );

        //
        // Initialize low level stuff
        //
        ::sceSifInitRpc( 0 );
        ::sceSifInitIopHeap( );
        
        if( MediaType == GameMediaCD )
        {
            ::sceCdInit( SCECdINIT );
            ::sceCdMmode( SCECdCD );                
        }
        else if( MediaType == GameMediaDVD )
        {
            ::sceCdInit( SCECdINIT );
            ::sceCdMmode( SCECdDVD );                
        }            

        rAssert( pIrxPath != NULL );

        m_IOPMedia = IOPMedia; 
        m_MediaType = MediaType;

        radStringCreate( &m_xIString_IrxPath, m_ThisAllocator );

        m_xIString_IrxPath->Copy( pIrxPath );
        rDebugString( "bPlatform: .irx search path set to: [" );
        rDebugString( m_xIString_IrxPath->GetChars( ) );
        rDebugString( "]\n" );

        //
        // Check if caller wants us to load the IopImage for them.
        //
        if( pIopImage != NULL )
        {
            char loadString[ 200 ];
            if( m_IOPMedia == IOPMediaHost )
            {
                strcpy( loadString, "host0:" );
                strcat( loadString, m_xIString_IrxPath->GetChars( ) );
                strcat( loadString, pIopImage );
            }
            else
            {
                char tempString[ 200 ];
                strcpy( loadString, "cdrom:\\" );
                strcpy( tempString, m_xIString_IrxPath->GetChars( ) );
                strcat( tempString, pIopImage );
                strcat( tempString, ";1");
                strupr( tempString );
                strcat( loadString, tempString );
            }
            
            while( !sceSifRebootIop( loadString ) ) { }
            while( !sceSifSyncIop( ) ) { }

            //
            // Re-initialize low level stuff
            //
            ::sceSifInitRpc( 0 );
            ::sceSifInitIopHeap( );
            ::sceSifLoadFileReset( );
            ::sceFsReset( );
    
            if( MediaType == GameMediaCD )
            {
                ::sceCdInit( SCECdINIT );
                ::sceCdMmode( SCECdCD );                
            }
            else if( MediaType == GameMediaDVD )
            {
                ::sceCdInit( SCECdINIT );
                ::sceCdMmode( SCECdDVD );                
            }            
        }
    }

    void Terminate( void )
    {

        //
        // Must do the work of the destructor here because we initialized before
        // main, and thus before the memory leak check runs.
        //

        m_xIString_IrxPath = NULL;
        m_xIOl_LoadedIrxModules = NULL;

        rAssertMsg( m_Initialized, "radPlatform not initialized");
        rAssertMsg( m_RefCount == 0, "radPlatorm still in use" );
        m_Initialized = false;
    }
    
    bool IsIrxLoaded( const char * pIrxFilenameNoPath )
    {
        if ( m_xIOl_LoadedIrxModules != NULL )
        {
            IRadString  * pIRadString;
         
            m_xIOl_LoadedIrxModules->Reset( );

            while( NULL != ( pIRadString = reinterpret_cast< IRadString * >( m_xIOl_LoadedIrxModules->GetNext( ) ) ) )
            {
                if( stricmp( pIRadString->GetChars( ), pIrxFilenameNoPath ) == 0 )
				{
					return true;
				}
            }
        }
        
        return false;        
    }

    void LoadIrxModule( const char* pIrxFilenameNoPath, int argSize = 0, char* args = NULL )
    {
        rAssert( pIrxFilenameNoPath != NULL );

        if ( pIrxFilenameNoPath != NULL )
        {
            if ( IsIrxLoaded( pIrxFilenameNoPath ) == true )
            {
                rDebugString( "Irx module: [" );
                rDebugString( pIrxFilenameNoPath );
                rDebugString( "] already loaded, ignoring request.\n" );
            }
            else
            {
                int loadResult;
                //
                // Load the IOP module off CD or host drive based on initialization parameter.
                //
                if( m_IOPMedia != IOPMediaHost )
                {
                    ref< IRadString > xIString_CdPath;
                    radStringCreate( &xIString_CdPath, RADMEMORY_ALLOC_TEMP  );
                    ref< IRadString > xIString_CdFilespec;
                    radStringCreate( &xIString_CdFilespec, RADMEMORY_ALLOC_TEMP  );

                    xIString_CdPath->Append( "cdrom0:\\" );

                    xIString_CdFilespec->Append( m_xIString_IrxPath );
                    xIString_CdFilespec->Append( pIrxFilenameNoPath );
                    xIString_CdFilespec->Append( ";1" );

                    xIString_CdFilespec->ToUppercase( ); // All cd files MUST be uppercase

                    xIString_CdPath->Append( xIString_CdFilespec );

                    rDebugString( "Attempting to load: [" );
                    rDebugString( xIString_CdPath->GetChars( ) );
                    rDebugString( "] from CD.\n" );

				    loadResult = sceSifLoadModule( const_cast< char* >( xIString_CdPath->GetChars( ) ), argSize, args );
                }
                else
                {
                    //
                    // First try loading the module from the host.
                    //
                    ref< IRadString > xIString_HostPath;
                    radStringCreate( &xIString_HostPath, RADMEMORY_ALLOC_TEMP  );

                    xIString_HostPath->Append( "host0:" );
                    xIString_HostPath->Append( m_xIString_IrxPath );
                    xIString_HostPath->Append( pIrxFilenameNoPath );

                    rDebugString( "Attempting to load: [" );
                    rDebugString( xIString_HostPath->GetChars( ) );
                    rDebugString( "] from host\n" );

                    loadResult = sceSifLoadModule( const_cast< char* >( xIString_HostPath->GetChars( ) ), argSize, args );
            
                    if( loadResult < 0 )
                    {
                        //
                        // Failed. Lets try using SN systems firewire Proview file server.
                        //
                        ref< IRadString > xIString_SNHostPath;
                        radStringCreate( &xIString_SNHostPath, RADMEMORY_ALLOC_TEMP  );

                        xIString_SNHostPath->Append( "snfile:" );
                        xIString_SNHostPath->Append( m_xIString_IrxPath );
                        xIString_SNHostPath->Append( pIrxFilenameNoPath );

                        rDebugString( "Attempting to load: [" );
                        rDebugString( xIString_SNHostPath->GetChars( ) );
                        rDebugString( "] from SNs Proview\n" );
            
                        loadResult = sceSifLoadModule( const_cast< char* >( xIString_SNHostPath->GetChars( ) ), argSize, args );
                    }
                }

                if ( loadResult >= 0 )
                {
					ref< IRadString > xIString;
                    radStringCreate( &xIString, m_ThisAllocator );
					rAssert( xIString != NULL );

					if ( xIString != NULL )
					{
						xIString->Copy( pIrxFilenameNoPath );
						m_xIOl_LoadedIrxModules->AddObject( xIString );
					}

                    rDebugString( "Loaded: [" );
                    rDebugString( pIrxFilenameNoPath );
                    rDebugString( "]\n" );
                }
                else
                {
                    rDebugString( "Failed to load: [" );
                    rDebugString( pIrxFilenameNoPath );
                    rDebugString( "]\n" );

                    rAssert( 0 );
                }
            }
        }
    }

    void UnloadIrxModule( const char* IrxFilenameNoPath )
    {
        rAssertMsg( 0, "Sony doesn't support unloading irx modules yet." );
    }

    radPlatformGameMediaType GetMediaType( void )
    {
        return( m_MediaType );
    }

    private:

    ref< IRadString >           m_xIString_IrxPath;
    ref< IRadObjectList >       m_xIOl_LoadedIrxModules;
    radPlatformIOPMedia         m_IOPMedia;    
    radPlatformGameMediaType    m_MediaType;

#endif

#ifdef RAD_GAMECUBE

    //
    // GAMECUBE implementation
    //
    void Initialize( void )
    {
        rAssertMsg( !m_Initialized, "radPlatform already initialized");
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radPlatform" );
        m_Initialized = true;
   	    		
		//
		// Initializes Operating System
		//
		OSInit();

        // Set the DMA Chunk Size

        ARQSetChunkSize( 4096 );
        //
        // This function initializes ARQ (Aram DMA Queue).
        //

        ARQInit( );	

        //
        // Set the DMA chunk size (used when higher priority DMA interrups
        // current DMA.  4096 is the default set in ARQInit, set it explicitly
        // just to be clear.
        //
    }

    void Terminate( void )
    {
        rAssertMsg( m_Initialized, "radPlatform not initialized");
        rAssertMsg( m_RefCount == 0, "radPlatorm still in use" );
        m_Initialized = false;

    }

#endif // RAD_GAMECUBE
    
    //
    // Data members common to all plaforms.
    //
    unsigned int m_RefCount;
public:
    bool         m_Initialized;
    unsigned int m_ThisAllocator;

};

//
// The one and only platform object.
//
static radPlatform* pthePlatform;
static unsigned int thePlaftormSpace[(sizeof( radPlatform ) / sizeof( unsigned int))  + 1 ];

//=============================================================================
// Function: radPlatformInitialize
//=============================================================================
// This function should be invoked very early in the life of a game. Specific
// parameters required for each platform.
//
//=============================================================================
#ifdef RAD_WIN32
//
// Windows requires the game provide the main window handle and the module
// instance.
//
void radPlatformInitialize( SDL_Window* pMainWindow, radMemoryAllocator allocator )
{
    pthePlatform = new( thePlaftormSpace ) radPlatform( );
    pthePlatform->Construct( allocator );
    pthePlatform->Initialize( pMainWindow, allocator );
}

#if SDL_MAJOR_VERSION < 3
int SDLCALL radPlatform::MainWindowProcedure
#else
bool SDLCALL radPlatform::MainWindowProcedure
#endif
(
    void* userdata, SDL_Event* event
)
{
    radPlatform* pThis = pthePlatform;

    IRadPlatformWin32MessageCallback* callback = (IRadPlatformWin32MessageCallback*)userdata;

    callback->OnWindowMessage( pThis->m_pMainWindow, event );

    return false;
}

#endif

#ifdef RAD_XBOX
//
// XBOX initialization requires no parameters
//
void radPlatformInitialize( void )
{
    pthePlatform = new( thePlaftormSpace ) radPlatform( );
    pthePlatform->Initialize( );
}    
#endif

#ifdef RAD_PS2
//
// On the PS2, provide the root path for IRXs. Also provide the name of the 
// IOP image to load if you want this operaton performed. The IRX path should
// be of the form "IRX\\", with no leading slash. 
//
void radPlatformInitialize( const char* pIrxPath, radPlatformIOPMedia IOPMedia, radPlatformGameMediaType GameMediaType, const char* pIopImage, radMemoryAllocator allocator )
{
    pthePlatform = new( thePlaftormSpace ) radPlatform( );
    pthePlatform->Initialize( pIrxPath, IOPMedia, GameMediaType, pIopImage, allocator );
}
#endif // RAD_PS2

#ifdef RAD_GAMECUBE
//
// On the RAD_GAMECUBE, OSInit() & OSInitAlloc() initialized the OS memory allocator 
// and ensures that new and delete will work properly.
//
void radPlatformInitialize( void )
{
    pthePlatform = new( thePlaftormSpace ) radPlatform( );
    pthePlatform->Initialize();
}
#endif // RAD_GAMECUBE

//=============================================================================
// Function: radPlatformTerminate
//=============================================================================

void radPlatformTerminate( void )
{
    pthePlatform->Terminate( );
    pthePlatform->~radPlatform();
}

//=============================================================================
// Function: radPlatformTerminate
//=============================================================================

IRadPlatform* radPlatformGet( void )
{
    rAssertMsg( pthePlatform != NULL, "radPlatform not initialized" );

    return( pthePlatform );

}
