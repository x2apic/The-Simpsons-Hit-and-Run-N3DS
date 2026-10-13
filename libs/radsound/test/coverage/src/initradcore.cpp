//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radthread.hpp>
#include <radplatform.hpp>
#include <radtime.hpp>
#include <radfile.hpp>
#include <radmemorymonitor.hpp>
#include <raddebugcommunication.hpp>
#include <radcontroller.hpp>
#include <raddebugwatch.hpp>

bool g_Done = false;

#ifdef PS2MW    
    extern "C" void mwInit( void );
#endif

#if defined RAD_PS2
    const char g_DriveName[] = "CDROM:";
#elif defined RAD_XBOX
    const char g_DriveName[] = "D:";
#elif defined RAD_GAMECUBE
    const char g_DriveName[] = "DVD:";
#elif defined RAD_WIN32
	const char g_DriveName[] = "D:";

	extern HINSTANCE g_hInstance;
	HWND g_hWnd;
#endif

ref< IRadDrive > g_refIRadDrive;

#ifdef RAD_WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
            break;
    }

    return 0;
}
#endif // RAD_WIN

void rstRadCoreInitialize( void )
{
	g_Done = false;

    #ifdef PS2MW
        mwInit( );
    #endif

    ::radThreadInitialize( );
    
    #ifndef RAD_GAMECUBE
    ::radMemoryInitialize( );
    #else
    ::radMemoryInitialize( 0, 0 );
    #endif // RAD_GCN

    //
    // Windows nonesense
    //

    #ifdef RAD_WIN32

    // Create a window
    const char appName[] = "FTech RadMovie Simple Movie Player";

    // Create an application window
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = g_hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
    wndclass.lpszMenuName= NULL;
    wndclass.lpszClassName = appName;

    RegisterClass(&wndclass);
    
    RECT clientRect;
    clientRect.left = 0;
    clientRect.top = 0;
    clientRect.right = 640;
    clientRect.bottom = 480;

    AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd =
        CreateWindow
        (
            appName,
            appName,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            clientRect.right-clientRect.left, clientRect.bottom-clientRect.top,
            NULL,
            NULL,
            g_hInstance,
            NULL
        );

    rAssert(g_hWnd);
    ShowWindow(g_hWnd, SW_SHOW);

    #endif // RAD_WIN32
    
    #if defined RAD_PS2
	    ::radPlatformInitialize( "irx/", IOPMediaHost, GameMediaCD, RADPLATFORM_IOP_IMG_FILENAME, RADMEMORY_ALLOC_DEFAULT );
    #elif defined RAD_WIN32
        ::radPlatformInitialize( g_hWnd, g_hInstance );
    #elif defined RAD_XBOX || defined RAD_GAMECUBE
        ::radPlatformInitialize( );
    #endif

    ::radTimeInitialize( );
    
    ::radMemoryMonitorInitialize( 50 * 1024 );

    #if defined RAD_PS2
        radDbgComTargetInitialize( Deci );
    #elif defined RAD_WIN32 || defined RAD_XBOX
        radDbgComTargetInitialize( WinSocket );
    #elif defined RAD_GAMECUBE
        radDbgComTargetInitialize( HostIO );
    #endif

	::radDbgWatchInitialize( "\\" );

    ::radFileInitialize( );

    ::radDriveOpen( & g_refIRadDrive,
        g_DriveName,
        NormalPriority,
        RADMEMORY_ALLOC_DEFAULT );

    ::radControllerInitialize( );
   // g_refIRadDrive->WaitForCompletion( );
}

void rstRadCoreService( void )
{
    #ifdef RAD_WIN32
        MSG msg;

        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                g_Done = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    #endif // RAD_WIN32

    ::radMemoryMonitorService( );
    ::radDbgComService( );
    ::radFileService( );
    ::radControllerSystemService( );
}

void rstRadCoreTerminate( void )
{    
    g_refIRadDrive = NULL;

    radMemoryMonitorSuspend( );

    ::radControllerTerminate( );
    ::radFileTerminate( );
	::radDebugWatchTerminate( );
    ::radDbgComTargetTerminate( );
    ::radMemoryMonitorTerminate( );
    ::radTimeTerminate( );
    ::radPlatformTerminate( );
    ::radMemoryTerminate( );
    ::radThreadTerminate( );
}


