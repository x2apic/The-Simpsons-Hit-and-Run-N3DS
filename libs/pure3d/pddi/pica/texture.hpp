#ifndef PDDI_PICA_TEXTURE_HPP
#define PDDI_PICA_TEXTURE_HPP

#include <pddi/pica/pica.hpp>
#include <pddi/pdditype.hpp>

class picaTexture : public pddiTexture
{
public:
    picaTexture(picaContext*);
    ~picaTexture();

    bool Create(int xSize, int ySize, int bpp, int alphaDepth, int nMip,
                pddiTextureType type = PDDI_TEXTYPE_RGB,
                pddiTextureUsageHint usageHint = PDDI_USAGE_STATIC);

    pddiPixelFormat GetPixelFormat();
    int GetWidth();
    int GetHeight();
    int GetDepth();
    int GetNumMipMaps();
    int GetAlphaDepth();

    int GetNumPaletteEntries(void);
    void SetPalette(int nEntries, pddiColour* palette);
    int GetPalette(pddiColour* palette);

    pddiLockInfo* Lock(int mipLevel, pddiRect* rect = 0);
    void Unlock(int mipLevel);

    void Prefetch(void);
    void Discard(void);
    void SetPriority(int priority);
    int GetPriority();

    C3D_Tex* GetC3DTexture(void) { return valid ? &tex : NULL; }

protected:
    void FreeStaging(void);
    void UploadLevel(int mipLevel);
    void ConvertAndTile(const unsigned int* rgba, int w, int h, void* dst);

    picaContext* context;

    int log2X, log2Y;
    int xSize, ySize;
    pddiTextureType type;
    int nMipMap;
    int alphaDepth;
    int priority;

    C3D_Tex  tex;
    bool    created;
    bool    valid;
    GPU_TEXCOLOR hwFormat;

    pddiLockInfo lock;

    unsigned char* staging;
    unsigned       stagingBytes;
    int            stagingLevel;
};
#endif
