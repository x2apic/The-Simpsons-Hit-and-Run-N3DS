//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#ifdef RAD_WIN32
#include "window.hpp"
#endif

#include <radplatform.hpp>
#include <radfile.hpp>
#include <radtime.hpp>
#include <radscript.hpp>
#include <radtypeinfo.hpp>
#include <radtypeinfoutil.hpp>
#include <raddebugcommunication.hpp>
#include <radremotecommand.hpp>

#ifdef RAD_GAMECUBE
#include <dolphin.h>
#endif

#include "radtstobj.hpp"

//============================================================================
// Globals
//============================================================================
bool g_Done;
char g_szBuffer[256];
// global namespace for scripting
ref< IRadNameSpace > g_xIRadNameSpace = NULL;

//
// Declare the typeinfo file name for different platform and build
//
#if defined RAD_XBOX
    #ifdef RADSCRIPT_DEBUG
        #define TYPEINFO_FILE "tix"
        #define BUILD_CONF "XBOX DEBUG"
    #else
        #define TYPEINFO_FILE "tix"
        #define BUILD_CONF "XBOX RELEASE"
    #endif
#elif defined RAD_WIN32	// RAD_WIN32 only
    #ifdef RADSCRIPT_DEBUG
        #define TYPEINFO_FILE "tiw"
        #define BUILD_CONF "PC DEBUG"
    #else
        #define TYPEINFO_FILE "tiw"
        #define BUILD_CONF "PC RELEASE"
    #endif
#elif defined RAD_GAMECUBE
    #ifdef RADSCRIPT_DEBUG
        #define TYPEINFO_FILE "tig"
        #define BUILD_CONF "GCN DEBUG"
    #else
        #define TYPEINFO_FILE "tig"
        #define BUILD_CONF "GCN RELEASE"
    #endif
#elif defined RAD_PS2
    #ifdef RADSCRIPT_DEBUG
        #define TYPEINFO_FILE "tip"
        #define BUILD_CONF "PS2EE DEBUG"
    #else
        #define TYPEINFO_FILE "tip"
        #define BUILD_CONF "PS2EE RELEASE"
    #endif
#endif

#ifdef PS2MW
extern "C"
{
extern void mwInit( );
}
#endif PS2MW

struct ITestRef
{
	virtual void AddRef( void ) = 0;
	virtual void Release( void ) = 0;
};

struct ITest : public ITestRef
{
	virtual void Function1( void ) = 0;
	virtual void Function2( void ) = 0;
};

struct realObject : ITest
{
	virtual void Function1( void );
	virtual void Function2( void );
	virtual void AddRef( void );	
	virtual void Release( void );	
};

void realObject::Function1( void ) { }
void realObject::Function2( void ) { }
void realObject::AddRef( void ) { }
void realObject::Release( void ) { }

#ifdef RAD_WIN32
int WINAPI WinMain
(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow
)
#endif

#ifdef RAD_XBOX
int _cdecl main (void)
#endif

#if defined RAD_PS2 || defined RAD_GAMECUBE
int main( void )
#endif
{
    #ifdef PS2MW
        mwInit( );
    #endif

    //========================================================================
    // Initialization
    //========================================================================
    rReleaseString( "=============================\n" );
    rReleaseString( "RadScriptTest [" );
    rReleaseString( BUILD_CONF );
    rReleaseString( "] - Begin\n" );
    rReleaseString( "=============================\n" );

    ::radMemoryInitialize( );
    ::radThreadInitialize( );

    #if defined RAD_PS2
        ::radPlatformInitialize( "IRX\\", IOPMediaHost );
    #elif defined RAD_WIN32
        ::radPlatformInitialize( ::CreateMainWindow( hInstance, nCmdShow ), hInstance );
    #elif defined RAD_XBOX || defined RAD_GAMECUBE
        ::radPlatformInitialize( );
    #endif

    ::radTimeInitialize( );

    #if defined RAD_PS2
        ::radDbgComTargetInitialize( Deci );
    #elif defined RAD_XBOX || defined RAD_WIN32
        ::radDbgComTargetInitialize( WinSocket );
    #elif defined  RAD_GAMECUBE
        ::radDbgComTargetInitialize( HostIO );
    #endif

    rDebugChannelInitialize( RADMEMORY_ALLOC_DEFAULT );
    rDebugChannelEnable( "radScriptBuffer" );

    ::radFileInitialize( );
    ::radDriveMount( );
    ::radScriptInitialize( RADMEMORY_ALLOC_DEFAULT );
    ::radRemoteScriptInitialize( RADMEMORY_ALLOC_DEFAULT );
    ::scrTstClassRegisterFactories( );

    //========================================================================
    // loading typeinfo file from disk
    //========================================================================
    // Note: typeinfo file is generated from radtstobj.hpp by using
    //       hpp2typ.exe, see custom build steps.
    //========================================================================
    ::radTypeInfoLoadTypeInfoSync( TYPEINFO_FILE".typ" );

    //========================================================================
    // Create a namespace
    //========================================================================
    g_xIRadNameSpace = ::radNameSpaceCreate( );

    //
    // Basic functionality testing
    //
    rReleaseString( "==========================\n" );
    rReleaseString( "Basic functionality test\n" );
    rReleaseString( "==========================\n" );

    // 1) Adding instance via code
    const char szName[] = "TestObject1";
    sprintf(
             g_szBuffer,
             "1) Adding instanace to namespace via code.\n"
             "\tobject [%s], class [%s].\n", szName, "IScrTstClass1"
           );

    rReleaseString( "==========================\n" );
    rReleaseString( g_szBuffer );
    rReleaseString( "==========================\n" );
    g_xIRadNameSpace->AddInstance( szName, ::scrTstClass1Create( ) );

    rReleaseString( "Testing script function call via \"with\".\n" );
    ::radScriptLoadAndRunScriptSync( "file1.txt", g_xIRadNameSpace, RADMEMORY_ALLOC_TEMP );

    // 2) Adding instance via script
    sprintf(
             g_szBuffer,
             "2) Adding instanace to namespace via script.\n"
             "\tobject [%s], class [%s].\n", "TestObject2", "IScrTstClass2"
           );

    rReleaseString( "==========================\n" );
    rReleaseString( g_szBuffer );
    rReleaseString( "==========================\n" );
    rReleaseString( " Testing script function call via \"create\" as well as inheritance.\n" );
    ::radScriptLoadAndRunScriptSync( "file2.txt", g_xIRadNameSpace, RADMEMORY_ALLOC_TEMP );

    // 3) Testing platform specific classes
    rReleaseString( "==========================\n" );
    rReleaseString( "3) Testing platform specific code.\n" );
    rReleaseString( "==========================\n" );
    ::radScriptLoadAndRunScriptSync( "file3.txt", g_xIRadNameSpace, RADMEMORY_ALLOC_TEMP );


    // 4) Testing event handling classes
    rReleaseString( "==========================\n" );
    rReleaseString( "3) Testing event handling event classes.\n" );
    rReleaseString( "==========================\n" );
    ::radScriptLoadAndRunScriptSync( "eventcfg.txt", g_xIRadNameSpace, RADMEMORY_ALLOC_TEMP );
    rReleaseString( "Fire a event by loading eventsrc.txt...\n" );
    ::radScriptLoadAndRunScriptSync( "eventsrc.txt", g_xIRadNameSpace, RADMEMORY_ALLOC_TEMP );


    unsigned int maxTime = 0;
    unsigned int starttime = radTimeGetMilliseconds( );
    unsigned int lastTime = radTimeGetMilliseconds( );

    while ( ! g_Done )
    {       
        unsigned thisTime = ::radTimeGetMilliseconds( );

        if( thisTime - lastTime > maxTime )
        {
            maxTime = thisTime - lastTime;

            char buf[128];
            sprintf( buf, "GameLoop: [%d]\n", maxTime );
            rDebugString( buf );

        }

        lastTime = thisTime;

        #if defined RAD_WIN32	// RAD_WIN32 only
        {
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

            Sleep( 1 );
        }
        #endif

        ::radFileService( );
        ::radDbgComService( );

        #ifdef RAD_GAMECUBE
        VIWaitForRetrace( );
        #endif
    }


    //========================================================================
    // Close Namespace
    //========================================================================
    g_xIRadNameSpace = NULL;

    //
    // Terminate
    //

    rDebugChannelTerminate( );
    ::radRemoteScriptTerminate( );
    ::radScriptTerminate( );
    ::radDriveUnmount( );
    ::radFileTerminate( );
    ::radTimeTerminate( );
    ::radDbgComTargetTerminate( );
    ::radPlatformTerminate( );
    ::radThreadTerminate( );
    ::radMemoryTerminate( );

    //
    // Look for memory leaks
    //
    #ifdef RADSCRIPT_DEBUG
    radObject::DumpObjects( );
    #endif 

    rReleaseString( "=============================\n" );
    rReleaseString( "RadScriptTest [" );
    rReleaseString( BUILD_CONF );
    rReleaseString( "] - End\n" );
    rReleaseString( "=============================\n" );
    return 0;
}