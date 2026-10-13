//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _TEXTURE_HPP
#define _TEXTURE_HPP

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>
#include <pddi/pddi.hpp>
#include <radmath/random.hpp> // Used by the set chunk loader to pick a chunk from the set.

class tTexture: public tEntity
{
public:  
    tTexture();

    bool Create(int xSize, int ySize, int bpp, int alphaDepth, int nMip,
                    pddiTextureType textureType = PDDI_TEXTYPE_RGB,
                    pddiTextureUsageHint usageHint = PDDI_USAGE_STATIC);

    bool CreateVolume(int xSize, int ySize, int zSize, int bpp, int alphaDepth, int nMip,
                            pddiTextureType textureType = PDDI_TEXTYPE_RGB, 
                            pddiTextureUsageHint usageHint = PDDI_USAGE_STATIC);

    pddiPixelFormat GetPixelFormat()                      { return texture ? texture->GetPixelFormat() : PDDI_PIXEL_UNKNOWN; }
    int GetWidth()                                        { return texture ? texture->GetWidth() : 0; }
    int GetHeight()                                       { return texture ? texture->GetHeight() : 0; }
    int GetDepth()                                        { return texture ? texture->GetDepth() : 0; }
    int GetAlphaDepth()                                   { return texture ? texture->GetAlphaDepth() : 0; }
    int GetNumMipMaps()                                   { return texture ? texture->GetNumMipMaps() : 0; }
    
    int GetNumPaletteEntries()                            { return texture ? texture->GetNumPaletteEntries() : 0; }
    void SetPalette(int nEntries, tColour* palette)       { if(texture) texture->SetPalette(nEntries, palette); }
    int GetPalette(tColour* palette)                      { return texture ? texture->GetPalette(palette) : 0; }
    
    pddiLockInfo* Lock(int mip, pddiRect* rect = 0)       { return texture ? texture->Lock(mip, rect) : NULL; }
    void Unlock(int mip)                                  { if(texture) texture->Unlock(mip); }
    
    void Prefetch()                                       { if(texture) texture->Prefetch(); }
    void Discard()                                        { if(texture) texture->Discard(); }
    void SetPriority(int priority)                        { if(texture) texture->SetPriority(priority); }
    int GetPriority()                                     { return texture ? texture->GetPriority() : 0; }

    pddiTexture* GetTexture()                             { return texture; }
    void SetTexture(pddiTexture *t);

protected:
    virtual ~tTexture();
    pddiTexture* texture;
};

class tImageFactory;
class tImageConverter;

class tTextureLoader : public tSimpleChunkHandler
{
public:
    tTextureLoader();

    tEntity* LoadObject(tChunkFile*, tEntityStore*);

    tTexture* LoadTexture(tChunkFile*);

protected:
    tTexture* LoadImage(tChunkFile*, tImageFactory* factory = NULL, tTexture* texture = NULL, int mipLevel = 0);
    tTexture* LoadVolumeImage(tChunkFile*, tImageFactory* factory = NULL, tTexture* texture = NULL, int mipLevel = 0, int numMipMaps = 0);

    tImageFactory* imageFactory;
    tImageConverter* imageConverter;

    ~tTextureLoader();
};

// Currently the SetChunk only has textures in it, but
//theortically, it could have tDrawables. However, until
//this happens, the setloader will be treated like a texture.
class tSetLoader : public tChunkHandler // Can't extend from tSimpleChunkHandler.
{
public:
    tSetLoader();
    tLoadStatus Load( tChunkFile* file, tEntityStore* store );
    bool CheckChunkID( unsigned ID );
	unsigned int GetChunkID();
    static void SetRandomSeed( unsigned NewSeed );

protected:
    ~tSetLoader();
    static rmt::Randomizer m_Random;
};

#endif //_TEXTURE_HPP

