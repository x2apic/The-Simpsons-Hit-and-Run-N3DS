/*===========================================================================
    $Workfile:: ddsurf.cpp           $
    $Revision:: 77                   $
    $Author:: Neall                  $
    25-Aug-97 Created by Neall

    PDDI DirectDraw surface, and display classes

    Copyright (c)1997, 1998, 1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include "display.hpp"
#include "util.hpp"
#include "../base/debug.hpp"

#include <math.h>
#include <stdio.h>
#include <SDL.h>
#include <SDL_syswm.h>

D3DFORMAT colourFormatTable[2][4] = {
    {  D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8  },  // 16 bit
    {  D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8 , D3DFMT_R5G6B5, D3DFMT_X1R5G5B5 }    // 32 bit
};

D3DFORMAT depthFormatTable[4][7] = {
    { D3DFMT_D16, D3DFMT_D16_LOCKABLE, D3DFMT_D15S1, D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4}, // 16 bit
    { D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D16_LOCKABLE, D3DFMT_D15S1},  // 32 bit
    { D3DFMT_D15S1, D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D16, D3DFMT_D16_LOCKABLE, D3DFMT_D32, D3DFMT_D24X8}, // 16 bit, stencil
    { D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D15S1, D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D16, D3DFMT_D16_LOCKABLE}  // 32 bit, stencil
};

bool g_videoCardLayerdShaderBusted = false;

//------------------------------------------------------------------------
d3dDisplay::d3dDisplay(pddiDisplayInfo* info, LPDIRECT3D8 d)
{
    displayInfo = info;
    d3d = d;
    adapterID = displayInfo->id;;
    d3dDevice = NULL;
	snapshot = NULL;

    hWnd = NULL;
    backBuffer = NULL;
	frontTmpBuffer = NULL;

    nColourBuffer = 2;
    bufferMask = PDDI_BUFFER_COLOUR;
    displayMode = PDDI_DISPLAY_WINDOW;
    displayHeight = 0;
    displayWidth = 0;
    displayDepth = 0;
    reset = false;
    isActive = true;
    ignoreResize = false;
    ignoreDisplayChange = false;
    forceInit = false;

    windowStyle = 0;

    gammaR = 1.0f;
    gammaG = 1.0f;
    gammaB = 1.0f;
    
    d3d->GetAdapterDisplayMode(adapterID, &origMode);
    d3d->GetDeviceCaps(adapterID, D3DDEVTYPE_HAL, &d3dCaps);

    hwTrans = false;

    lockToVsync = false;
    asyncSwap = true;

	m_ForceVSync = false;

    //[cbc]Collect display specific disable hacks here
    D3DADAPTER_IDENTIFIER8 adaptID;
    if(d && D3D_OK == d->GetAdapterIdentifier(adapterID, 0, &adaptID))
    {
        if(adaptID.VendorId == 4098)
        {
            switch(adaptID.DeviceId)
            {
            case 20825://RADEON 7000 SERIES, ok 
            case 20804://RADEON 7200 SERIES, ok
            case 20823://ALL-IN-WONDER RADEON 7500, ok
                //These ones work
                g_videoCardLayerdShaderBusted = false;
                break;
            case 20812://RADEON 8500 SERIES, known bad
            case 19558://ATI MOBILITY RADEON 9000, known bad
            case 18790://RADEON 9000, known bad
                //These do not work
                g_videoCardLayerdShaderBusted = true;
                break;
            default:
                //Default to ok
                g_videoCardLayerdShaderBusted = false;
            }
        }
    }        
}


d3dDisplay::~d3dDisplay()
{
    if(d3dDevice)
    {
        // jpatry: Hide the window before destroying the context.
        // This fixes a problem with desktop background being left in a 
        // "shrunken" state, and some windows being moved to fit in the 
		// smaller resolution of the game's window.
        if (hWnd)
        {
            ShowWindow(hWnd, SW_HIDE);
        }
        d3dDevice->Release();
    }

	if( snapshot )
		snapshot->Release( );

	if( frontTmpBuffer )
		frontTmpBuffer->Release( );

}

//------------------------------------------------------------------------
long d3dDisplay::ProcessWindowMessage(SDL_Window* wnd, const SDL_WindowEvent* event)
{
    switch(event->event)
    {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
        case SDL_WINDOWEVENT_FOCUS_LOST:
            if(displayMode == PDDI_DISPLAY_FULLSCREEN) Activate(event->event == SDL_WINDOWEVENT_FOCUS_GAINED);
            break;

        case SDL_WINDOWEVENT_SIZE_CHANGED:
            if(!ignoreDisplayChange)
            {
                forceInit = true;
                if(displayMode == PDDI_DISPLAY_FULLSCREEN) 
                {
                    displayDepth  = 32; 
                    displayWidth  = event->data1;
                    displayHeight = event->data2;
                };
            }

            if(!ignoreResize)
            {
                RECT clientRect;
                GetClientRect((HWND)hWnd, &clientRect);
                //InitDisplay(clientRect.right, clientRect.bottom, displayDepth);
            }
            break;
    }

    return true;
}

void d3dDisplay::Activate(bool active)
{
/*
    if(isActive == active)
        return;

    if(!active)
    {
        SendMessage((HWND)hWnd, WM_PDDI_DRAW_ENABLE, 0, 0);
    }
    else
    {
        HRESULT res = d3dDevice->TestCooperativeLevel();
        switch(res)
        {
            case D3D_OK:
                SendMessage((HWND)hWnd, WM_PDDI_DRAW_ENABLE, 1, 0);
                break;
            case D3DERR_DEVICELOST:
                return;
                break;
            case D3DERR_DEVICENOTRESET:
                if(d3dDevice->Reset(&d3dpp) == D3D_OK)
                {
                    SendMessage((HWND)hWnd, WM_PDDI_DRAW_ENABLE, 1, 0);
                    reset = true;
                }
                else
                {
                    return;
                }
                break;
            default:
                break;
        }
    }
    isActive = active;
*/
}

//------------------------------------------------------------------------
void d3dDisplay::SetWindow(SDL_Window* wnd)
{
    SDL_SysWMinfo wmInfo;
    SDL_VERSION( &wmInfo.version );
    SDL_GetWindowWMInfo( wnd, &wmInfo );
    hWnd = wmInfo.info.win.window;
}

//------------------------------------------------------------------------
bool d3dDisplay::InitDisplay(int x, int y, int bpp)
{
    // check we are not trying to init to the same resolution
    if((x == displayWidth) &&  (y == displayHeight) && (bpp == displayDepth))
    {
        return true;
    }

    // fill in the relevent portions of the casced display init structure
    displayInit.xsize = x;
    displayInit.ysize = y;
    displayInit.bpp = bpp;

    // do the full init
    return InitDisplay(&displayInit);
}

bool d3dDisplay::InitDisplay(const pddiDisplayInit* init)
{
    displayInit = *init;

    int x = init->xsize;
    int y = init->ysize;
    int bpp = init->bpp;
    pddiDisplayMode mode = init->displayMode;
    int colourBufferCount = 1;
    unsigned bufMask = init->bufferMask;
    unsigned nSamples = 0;
    lockToVsync = init->lockToVsync;
    asyncSwap = init->asyncSwap;

    bool needResize = false;
    ignoreResize = true;
    PDDIASSERT(hWnd);

    nColourBuffer = colourBufferCount;
    bufferMask = bufMask;
    displayDepth = bpp;

    if(!(d3dCaps.Caps2 & D3DCAPS2_CANRENDERWINDOWED) && (mode == PDDI_DISPLAY_WINDOW))
    {
        mode = PDDI_DISPLAY_FULLSCREEN;
        OutputDebugString("PDDI: Cannot use a selected adapter in a window, switching to fullscreen.\n");
    }

    switch(mode)
    {
        case PDDI_DISPLAY_WINDOW :
            {
                // can't lock to vsync in windowed mode
                lockToVsync = false;

                // restore borders if we've been to fullscreen
                if(displayMode == PDDI_DISPLAY_FULLSCREEN)
                {
                    SetWindowLong(hWnd, GWL_STYLE, windowStyle);
                    needResize = true;

                    displayWidth = x;
                    displayHeight = y;

                }
                else
                {
                    RECT clientRect;
                    GetClientRect(hWnd, &clientRect);
                    displayWidth = clientRect.right;
                    displayHeight = clientRect.bottom;
                }
            }
            break;
        case PDDI_DISPLAY_FULLSCREEN:
            {
                displayWidth = x;
                displayHeight = y;

                // remove window borders and stuff
                if(displayMode == PDDI_DISPLAY_WINDOW)
                {
                    windowStyle = GetWindowLong(hWnd, GWL_STYLE);
                }
                unsigned style = windowStyle & ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME );
                SetWindowLong(hWnd, GWL_STYLE, style);
            }
    }

    displayMode = mode;

    if(displayMode == PDDI_DISPLAY_WINDOW)
    {
        D3DDISPLAYMODE mode;
        d3d->GetAdapterDisplayMode(adapterID, &mode);
        colourBufferFormat = mode.Format;
        displayDepth = DisplayBitDepth(mode.Format);
    }
    else
    {
        D3DFORMAT* testColourFormats = colourFormatTable[(displayDepth == 32) ? 1 : 0];

        colourBufferFormat = D3DFMT_UNKNOWN;
        for(int i = 0; i < 4; i++)
        {
            if(D3D_OK == d3d->CheckDeviceFormat(adapterID, D3DDEVTYPE_HAL, testColourFormats[i], 
                                                            D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE , testColourFormats[i]))
            {
                colourBufferFormat = testColourFormats[i];
                break;
            }
        }
        PDDIASSERT(colourBufferFormat != D3DFMT_UNKNOWN);

        if(displayDepth != DisplayBitDepth(colourBufferFormat))
        {
            // TODO : warn
        }
    }

    {
        D3DFORMAT* testDepthFormats = depthFormatTable[((displayDepth == 32) ? 1 : 0) | ((bufferMask & PDDI_BUFFER_STENCIL) ? 2 : 0)];
        depthBufferFormat = D3DFMT_UNKNOWN;

        for(int i = 0; i < 7; i++)
        {
            if(D3D_OK == d3d->CheckDeviceFormat(adapterID, D3DDEVTYPE_HAL, colourBufferFormat, 
                                                            D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE , testDepthFormats[i]))
            {
                depthBufferFormat = testDepthFormats[i];
                break;
            }
        }
        PDDIASSERT(depthBufferFormat != D3DFMT_UNKNOWN);
    }

    //if(nSamples == 1)
        nSamples = 0;

    /*
     D3DFMT_D16_LOCKABLE         =  70,
     D3DFMT_D32                  =  71,
     D3DFMT_D15S1                =  73,
     D3DFMT_D24S8                =  75,
     D3DFMT_D16                  =  80,
     D3DFMT_D24X8                =  77,
     D3DFMT_D24X4S4              =  79,
    */


    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed               = mode == PDDI_DISPLAY_WINDOW;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_FLIP;
    d3dpp.BackBufferCount        = nColourBuffer ;
    d3dpp.MultiSampleType        = D3DMULTISAMPLE_TYPE(nSamples);
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = depthBufferFormat;
    d3dpp.hDeviceWindow          = hWnd;
    d3dpp.BackBufferWidth        = displayWidth;
    d3dpp.BackBufferHeight       = displayHeight;
    d3dpp.BackBufferFormat       = colourBufferFormat;
    d3dpp.FullScreen_RefreshRateInHz = (mode == PDDI_DISPLAY_WINDOW) ? 0 : D3DPRESENT_RATE_DEFAULT;
    d3dpp.FullScreen_PresentationInterval = (mode == PDDI_DISPLAY_WINDOW) ? D3DPRESENT_INTERVAL_DEFAULT : (lockToVsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE);

    if(d3dDevice)
    {
        ignoreDisplayChange = true;
        int rc = d3dDevice->Reset(&d3dpp);
        ignoreDisplayChange = false;

/*      if(rc != D3DERR_DEVICELOST)
        {
            DDVERIFY(rc);
        }*/

        reset = true;
        forceInit = true;
    }
    else
    {
        unsigned int behavior = D3DCREATE_MULTITHREADED;

        //test if hardware vertex shader is supported
        unsigned int mainVersion, subVersion;
        mainVersion = ( d3dCaps.VertexShaderVersion >> 8 ) & 0xff;
        subVersion = d3dCaps.VertexShaderVersion  & 0xff;

        if( mainVersion >= 1 && subVersion >= 1 )
            hwVertexShader = true;        
        else
            hwVertexShader = false;

        if(d3dCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        {            
            hwTrans = true;
        }
        else
        {         
            hwTrans = false;
        }

        if( hwVertexShader && hwTrans ){
            behavior |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
        }
        else{
            behavior |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        DDVERIFY(d3d->CreateDevice( adapterID, D3DDEVTYPE_HAL, hWnd, behavior, &d3dpp, &d3dDevice ));

        // Get the initial gamma ramp.
        if( d3dDevice != NULL )
        {
            d3dDevice->GetGammaRamp(&initialGammaRamp);
            for(int i = 0; i < 256; i++)
            {
                initialGammaRamp.red[i] = (int(initialGammaRamp.red[i]) << 8) | 0xff;
                initialGammaRamp.green[i] = (int(initialGammaRamp.green[i]) << 8) | 0xff;
                initialGammaRamp.blue[i] = (int(initialGammaRamp.blue[i]) << 8) | 0xff;
            }
        }
    }

    if(needResize)
    {
        RECT clientRect;
        clientRect.left = 0;
        clientRect.top = 0;
        clientRect.right = displayWidth;
        clientRect.bottom = displayHeight;

        unsigned windowStyle = GetWindowLong(hWnd, GWL_STYLE);
        AdjustWindowRect(&clientRect, windowStyle, FALSE);
        SetWindowPos(hWnd,HWND_NOTOPMOST,0,0, 
            clientRect.right-clientRect.left,
            clientRect.bottom-clientRect.top, SWP_NOMOVE);
        ShowWindow(hWnd, TRUE);
    }

    ignoreResize = false;

	//create a texture for capturing snapshot
	if(init->enableSnapshot)
    {

		LPDIRECT3DSURFACE8 pRenderTarget;
		d3dDevice->GetRenderTarget(&pRenderTarget);
		D3DSURFACE_DESC descRenderTarget;
		pRenderTarget->GetDesc(&descRenderTarget);
    
        DDVERIFY(d3dDevice->CreateTexture(descRenderTarget.Width, descRenderTarget.Height, 1, D3DUSAGE_RENDERTARGET, /*descRenderTarget.Format*/D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &snapshot));
        pRenderTarget->Release();

        D3DDEVICE_CREATION_PARAMETERS dcp;
        dcp.AdapterOrdinal = D3DADAPTER_DEFAULT;
        d3dDevice->GetCreationParameters(&dcp);
        D3DDISPLAYMODE dm;
        dm.Width = dm.Height = 0;
        d3d->GetAdapterDisplayMode(dcp.AdapterOrdinal, &dm);

		//create temporary front buffer
        DDVERIFY(d3dDevice->CreateImageSurface(dm.Width, dm.Height, D3DFMT_A8R8G8B8, &frontTmpBuffer));
    }

    return d3dDevice != NULL;

    /*
#ifdef P3DDEBUG
    char buf[255];
    wsprintf(buf, "PDDI DirectDraw: Primary: %d, 3D: %d, '%s', '%s'\n",
        displayInfo->pddiInfo.primary, displayInfo->pddiInfo.hardware,
        displayInfo->driverName, displayInfo->driverDesc);
    OutputDebugString(buf);
#endif
    */
}

bool d3dDisplay::HasReset(void)
{
    if(forceInit)
        InitDisplay(&displayInit);

    forceInit = false;
    return reset;
}

//------------------------------------------------------------------------
void d3dDisplay::SwapBuffers(void)
{
    reset = false;
    HRESULT hr = d3dDevice->Present(0,0,0,0);

    if(hr != D3D_OK)
    {
        hr = d3dDevice->TestCooperativeLevel();
        while (hr != D3D_OK)
        {
            if (hr == D3DERR_DEVICENOTRESET)
            {
                hr = d3dDevice->Reset(&d3dpp);
            }
            ::Sleep(500);
            // Make sure we're still processing messages
            MSG msg;
            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );

                if( msg.message == WM_SYSCOMMAND && msg.wParam == SC_CLOSE )
                {
                    return;
                }
            }
            hr = d3dDevice->TestCooperativeLevel();
        }

        forceInit = true;
        reset = true;
    }
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
unsigned d3dDisplay::Screenshot(pddiColour* buffer, int nBytes)
{
    pddiRect rect; 
    rect.top = 0;
    rect.left = 0;
    rect.bottom = displayHeight;
    rect.right = displayWidth;
    return Screenshot(buffer, nBytes, rect);
}

unsigned d3dDisplay::Screenshot(pddiColour* buffer, int nBytes, const pddiRect& rectangle, pddiBuffer source)
{

    // define the areas to copy
    POINT upperLeft;
    upperLeft.x = rectangle.left;
    upperLeft.y = rectangle.top;

    RECT rect; 
    rect.top = rectangle.top;
    rect.left = rectangle.left;
    rect.bottom = rectangle.bottom;
    rect.right = rectangle.right;

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    PDDIASSERT(nBytes >= width * height * 4);

    if(source == PDDI_BUFFER_BACK)
    {
        // create a temp destination buffer the size of the whole screen
        // it'll be in a fixed format we can copy easily
        LPDIRECT3DSURFACE8 dest;
        DDVERIFY( d3dDevice->CreateImageSurface( width, height, colourBufferFormat, &dest ) ); // TODO : breaks in 16bpp

        // grab the back buffer
        LPDIRECT3DSURFACE8 src;
        DDVERIFY( d3dDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &src) );

        // suck the bits out of the back buffer
        POINT origin = {0,0};
        HRESULT res = d3dDevice->CopyRects( src, &rect, 1, dest, &origin );

        // copy the bits across to the supplied colour buffer
        D3DLOCKED_RECT lock;
        DDVERIFY( dest->LockRect( &lock, NULL, D3DLOCK_READONLY ) );

        unsigned char* scanline = (unsigned char*)lock.pBits;
        pddiColour* walk = buffer;

        for( int i = 0; i < height; i++ )
        {
            memcpy( walk, scanline, width * 4 );
            walk += width;
            scanline += lock.Pitch;
        }
    
        // clean up
        dest->UnlockRect();
        dest->Release();
        src->Release();
    }
    else // PDDI_BUFFER_FRONT
    {
        // figure out how big the screen is in the current display mode 
        // In windowed mode, the front buffer is the WHOLE screen, not the rendering area
        // so we can't just use displayWidth/displayHeight
        D3DDEVICE_CREATION_PARAMETERS dcp;
        dcp.AdapterOrdinal = D3DADAPTER_DEFAULT;
        d3dDevice->GetCreationParameters(&dcp);
        D3DDISPLAYMODE dm;
        dm.Width = dm.Height = 0;
        d3d->GetAdapterDisplayMode(dcp.AdapterOrdinal, &dm);

        // create a surface
        LPDIRECT3DSURFACE8 src;
        DDVERIFY(d3dDevice->CreateImageSurface(dm.Width, dm.Height, D3DFMT_A8R8G8B8, &src));

        // this copies the bits from the front buffer
//        DDVERIFY(d3dDevice->GetFrontBuffer(src));
		d3dDevice->GetFrontBuffer(src);

        // get real location in windowed mode
        if(displayMode == PDDI_DISPLAY_WINDOW)
        {
            ClientToScreen(hWnd, &upperLeft);
            OffsetRect(&rect, upperLeft.x, upperLeft.y);
        }

        // copy the bits from the temp surface to supplied colour buffer
        D3DLOCKED_RECT lock;
        DDVERIFY(src->LockRect(&lock, &rect, D3DLOCK_READONLY));

        unsigned char* scanline = ((unsigned char*)lock.pBits);
        pddiColour* walk = buffer;

        for( int i = 0; i < height; i++ )
        {
            memcpy( walk, scanline, width * 4 );
            walk += width;
            scanline += lock.Pitch;
        }
    
        // cleanup 
        src->UnlockRect();
        src->Release();
    }
    return width*height*4;
}

//------------------------------------------------------------------------
unsigned d3dDisplay::GetFreeVideoMem()
{
    return 0; // TODO : remove
}

//------------------------------------------------------------------------
unsigned d3dDisplay::GetFreeTextureMem()
{
    return d3dDevice->GetAvailableTextureMem();
}

void d3dDisplay::GetGamma(float* r, float* g, float* b)
{
    *r = gammaR;
    *g = gammaG;
    *b = gammaB;
}

void d3dDisplay::SetGamma(float r, float g, float b)
{
//   if(displayMode != PDDI_DISPLAY_FULLSCREEN)
//      return;

    gammaR = r;
    gammaG = g;
    gammaB = b;

    D3DGAMMARAMP gamma;

    double igr = 1.0 / (double)r;
    double igg = 1.0 / (double)g;
    double igb = 1.0 / (double)b;

    const double n = 1.0 / 65535.0;

    for(int i=0; i < 256; i++)
    {
        double gcr = pow((double)initialGammaRamp.red[i]   * n, igr);
        double gcg = pow((double)initialGammaRamp.green[i] * n, igg);
        double gcb = pow((double)initialGammaRamp.blue[i]  * n, igb);

        gamma.red[i] =   (WORD)(65535.0 * __min(1.0, gcr));
        gamma.green[i] = (WORD)(65535.0 * __min(1.0, gcg));
        gamma.blue[i] =  (WORD)(65535.0 * __min(1.0, gcb));
    }

    d3dDevice->SetGammaRamp(D3DSGR_CALIBRATE, &gamma);
}


void d3dDisplay::Snapshot(void)
{
    if(snapshot)
    {      

		POINT upperLeft;

		upperLeft.x = upperLeft.y = 0;
        
        // this copies the bits from the front buffer
//        DDVERIFY(d3dDevice->GetFrontBuffer(src));
		d3dDevice->GetFrontBuffer( frontTmpBuffer );

		RECT rect; 
		rect.top = 0;
		rect.left = 0;
		rect.bottom = 480;
		rect.right = 640;


		POINT corner;
		corner.x = corner.y = 0;

        // get real location in windowed mode
        if(displayMode == PDDI_DISPLAY_WINDOW)
        {
            ClientToScreen(hWnd, &upperLeft);
            OffsetRect(&rect, upperLeft.x, upperLeft.y);
        }

        LPDIRECT3DSURFACE8 snapshotSurface;     
        snapshot->GetSurfaceLevel(0, &snapshotSurface);

		d3dDevice->CopyRects( frontTmpBuffer, &rect, 1, snapshotSurface, &corner );
	     snapshotSurface->Release();		

/*		
        LPDIRECT3DSURFACE8 renderTarget;
        LPDIRECT3DSURFACE8 snapshotSurface;
        d3dDevice->GetRenderTarget(&renderTarget);
        snapshot->GetSurfaceLevel(0, &snapshotSurface);

        d3dDevice->CopyRects(renderTarget, NULL, 0, snapshotSurface, NULL); 

        renderTarget->Release();
        snapshotSurface->Release();
*/
    }

}