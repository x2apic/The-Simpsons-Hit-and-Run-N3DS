/*===========================================================================
    dddisplay.hpp
    25-Aug-97 Created by Neall

    Copyright (c)1997, 1998, 1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _DISPLAY_HPP
#define _DISPLAY_HPP

#include "../pddi.hpp"
#include "../pddiext.hpp"
#include "direct3d.hpp"
#include "util.hpp"

class d3dSurface;

class d3dDisplay : public pddiDisplay
{
public:
    d3dDisplay(pddiDisplayInfo* info, LPDIRECT3D8);
    virtual ~d3dDisplay();

    // public interface stuff
    virtual long ProcessWindowMessage(SDL_Window* wnd, const SDL_WindowEvent* event);
    virtual void SetWindow(SDL_Window* wnd);
    
    virtual bool InitDisplay(int x, int y, int bpp);
    virtual bool InitDisplay(const pddiDisplayInit*);

    pddiDisplayInfo* GetDisplayInfo(void) { return displayInfo;}

    virtual int GetHeight() { return displayHeight; }
    virtual int GetWidth()  { return displayWidth; }
    virtual int GetDepth()  { return displayDepth; }
    virtual pddiDisplayMode GetDisplayMode() { return displayMode; }
    virtual int GetNumColourBuffer()         { return nColourBuffer; }
    virtual unsigned GetBufferMask()         { return bufferMask; }

    virtual unsigned GetFreeVideoMem();
    virtual unsigned GetFreeTextureMem();

    virtual void SwapBuffers(void);

    unsigned Screenshot(pddiColour* buffer, int nBytes);
    unsigned Screenshot(pddiColour* buffer, int nBytes, const pddiRect& rectangle, pddiBuffer source = PDDI_BUFFER_FRONT);

	void Snapshot(void);
	LPDIRECT3DTEXTURE8  GetSnapshotTexture(void) { return snapshot;}

    // class specific stuff
    bool HasReset(void);

    bool              IsHardwareTransform(void) { return hwTrans;}
    bool              IsHardwareVertexShader ( void ){ return hwVertexShader; }
    LPDIRECT3D8       GetD3D(void)       { return d3d;}
    LPDIRECT3DDEVICE8 GetD3DDevice(void) { return d3dDevice;}
    D3DCAPS8*         GetCaps(void)      { return &d3dCaps;}
    D3DFORMAT         GetColourBufferFormat(void) { return colourBufferFormat;}

    void SetGamma(float r, float g, float b);
    void GetGamma(float* r, float* g, float* b);
    HRESULT Reset() { return d3dDevice->Reset(&d3dpp); };

protected:
    LPDIRECT3D8       d3d;
    LPDIRECT3DDEVICE8 d3dDevice;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DCAPS8 d3dCaps;
    D3DGAMMARAMP initialGammaRamp;
    D3DFORMAT colourBufferFormat;
    D3DFORMAT depthBufferFormat;
    D3DDISPLAYMODE origMode;
	LPDIRECT3DTEXTURE8 snapshot;
    bool hwTrans;
    bool hwVertexShader;

//   DDCAPS driverCaps;
//   DDCAPS helCaps;

    pddiDisplayInit  displayInit;
    pddiDisplayInfo* displayInfo;

    HWND hWnd;

    bool reset;
    bool isActive;
    bool ignoreResize;
    
    d3dSurface* backBuffer;  
	LPDIRECT3DSURFACE8 frontTmpBuffer;

    int      adapterID;
    int      nColourBuffer;
    unsigned bufferMask;

    pddiDisplayMode displayMode;   
    int  displayHeight;
    int  displayWidth;
    int  displayDepth;

    bool forceInit;
    bool ignoreDisplayChange;

    unsigned windowStyle;

    float gammaR, gammaG, gammaB;

    bool lockToVsync;
    bool asyncSwap;

    void Activate(bool);
};

#endif //_DISPLAY_HPP

