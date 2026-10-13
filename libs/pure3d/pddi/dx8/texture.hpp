/*===========================================================================
    $Workfile:: texture.hpp          $
    $Revision:: 30                   $
    $Author:: Neall                  $
    20-May-96 Created by Neall
    20-Oct-99 Updated for release 12

    Copyright (c)1995, 1996, 1997, 1998, 1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _TEXTURE_HPP
#define _TEXTURE_HPP

#include "../pddi.hpp"
#include "direct3d.hpp"

class d3dContext;

class d3dTexture : public pddiTexture
{
public:
    d3dTexture(d3dContext* context);
    d3dTexture(d3dContext* context, LPDIRECT3DBASETEXTURE8);
    virtual ~d3dTexture();
    
    bool Create(int xSize, int ySize, int bpp, int alphaDepth, int nMip, pddiTextureType type = PDDI_TEXTYPE_RGB, pddiTextureUsageHint usageHint = PDDI_USAGE_STATIC);
    bool CreateVolume(int xSize, int ySize, int zSize, int bpp, int alphaDepth, int nMip, pddiTextureType type = PDDI_TEXTYPE_RGB, pddiTextureUsageHint usageHint = PDDI_USAGE_STATIC);
    pddiPixelFormat GetPixelFormat() { return pixelFormat; }
    int GetWidth()                   { return width; }
    int GetHeight()                  { return height; }
    int GetDepth()                   { return bpp; }
    int GetAlphaDepth()              { return alphaDepth; }
    int GetNumMipMaps()              { return numMipMaps; }

    // palette managment
    int GetNumPaletteEntries(void);
    void SetPalette(int nEntries, pddiColour* palette);
    int GetPalette(pddiColour* palette);

    // texture upload
    pddiLockInfo* Lock(int mipLevel, pddiRect* rect = 0);
    void Unlock(int mipLevel);

    // paging control
    void Prefetch();
    void Discard();
    void SetPriority(int priority);
    int GetPriority()  { return priority; }

    bool IsRenderTarget()   { return rendertarget; }

    void ChangeForgedTexture(LPDIRECT3DTEXTURE8);

    LPDIRECT3DTEXTURE8 GetTexture(void) { return texture2d;}
    LPDIRECT3DVOLUMETEXTURE8 GetVolumeTexture(void) { return texture3d;}
    void SetTexture(int stage);

private:
    d3dContext* context;

    LPDIRECT3DBASETEXTURE8 forged;

    LPDIRECT3DTEXTURE8 texture2d;
    LPDIRECT3DVOLUMETEXTURE8 texture3d;
    D3DLOCKED_RECT lock2d;
    D3DLOCKED_BOX lock3d;
    D3DSURFACE_DESC sd;
    D3DVOLUME_DESC vd;
    D3DFORMAT format;

    pddiLockInfo lockInfo;

    pddiTextureType type;
    pddiTextureUsageHint usageHint;
    pddiPixelFormat pixelFormat;
    
    int width;
    int height;
    int bpp;
    int alphaDepth;
    int numMipMaps;

    unsigned texMemUse;

    bool dxtn;
    bool linear;
    bool rendertarget;
    bool first;

    int priority;

    unsigned char* bits;
};

#endif // _TEXTURE_HPP

