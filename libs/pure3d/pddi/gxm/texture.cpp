//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <pddi/gxm/gxm.hpp>
#include <pddi/gxm/device.hpp>
#include <pddi/gxm/display.hpp>
#include <pddi/gxm/texture.hpp>
#include <pddi/gxm/context.hpp>
#include <pddi/gxm/texture_swizzler.hpp>

#include <math.h>
#include <pddi/base/debug.hpp>
#include <radmemory.hpp>

#include <microprofile.h>

static inline SceGxmTextureFormat PickPixelFormat(pddiPixelFormat format)
{
    switch (format)
    {
    case PDDI_PIXEL_RGB888: return SCE_GXM_TEXTURE_FORMAT_X8U8U8U8_1RGB;
    case PDDI_PIXEL_ARGB8888: return SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ARGB;
    case PDDI_PIXEL_DXT1: return SCE_GXM_TEXTURE_FORMAT_UBC1_ABGR;
    case PDDI_PIXEL_DXT3: return SCE_GXM_TEXTURE_FORMAT_UBC2_ABGR;
    case PDDI_PIXEL_DXT5: return SCE_GXM_TEXTURE_FORMAT_UBC3_ABGR;
    case PDDI_TEXTYPE_YUV: return SCE_GXM_TEXTURE_FORMAT_YUV420P3_CSC0;
    }
    PDDIASSERT(false);
    return (SceGxmTextureFormat)0;
};

static inline pddiPixelFormat PickPixelFormat(pddiTextureType type, int bitDepth, int alphaDepth)
{
    switch (type)
    {
    case PDDI_TEXTYPE_RGB:
        switch (alphaDepth)
        {
        case 0:
            return (bitDepth <= 16) ? PDDI_PIXEL_RGB565 : PDDI_PIXEL_RGB888;
        case 1:
            return (bitDepth <= 16) ? PDDI_PIXEL_ARGB1555 : PDDI_PIXEL_ARGB8888;
        default:
            return (bitDepth <= 16) ? PDDI_PIXEL_ARGB4444 : PDDI_PIXEL_ARGB8888;
        }
        break;

    case PDDI_TEXTYPE_PALETTIZED:
        return PDDI_PIXEL_PAL8;

    case PDDI_TEXTYPE_LUMINANCE:
        return PDDI_PIXEL_LUM8;

    case PDDI_TEXTYPE_BUMPMAP:
        return PDDI_PIXEL_DUDV88;

    case PDDI_TEXTYPE_DXT1:
        return PDDI_PIXEL_DXT1;

    case PDDI_TEXTYPE_DXT2:
        return PDDI_PIXEL_DXT2;

    case PDDI_TEXTYPE_DXT3:
        return PDDI_PIXEL_DXT3;

    case PDDI_TEXTYPE_DXT4:
        return PDDI_PIXEL_DXT4;

    case PDDI_TEXTYPE_DXT5:
        return PDDI_PIXEL_DXT5;

    case PDDI_TEXTYPE_YUV:
        return PDDI_PIXEL_YUV;
    }
    PDDIASSERT(false);
    return PDDI_PIXEL_UNKNOWN;
};

int fastlog2(int x)
{
    int r = 0;
    int tmp = x;
    while(tmp > 1)
    {
        r++;
        tmp = tmp >> 1;

        if((tmp << r) != x)
            // not power of 2
            return -1;
    }
    return r;
}

bool gxmTexture::Create(int x, int y, int bpp, int alphaDepth, int nMip, pddiTextureType textureType, pddiTextureUsageHint usageHint)
{
    xSize = x;
    ySize = y;
    nMipMap = nMip;
    type = textureType;

    log2X = fastlog2(xSize);
    log2Y = fastlog2(ySize);

    if((log2X == -1) || (log2Y == -1))
    {
        if(type == PDDI_TEXTYPE_DXT1 || type == PDDI_TEXTYPE_DXT3 || type == PDDI_TEXTYPE_DXT5)
        {
            lastError = PDDI_TEX_NOT_POW_2;
            return false;
        }
    }

    if ((xSize > context->GetMaxTextureDimension()) ||
        (ySize > context->GetMaxTextureDimension()))
    {
        lastError = PDDI_TEX_TOO_BIG;
        return false;
    }

    // TODO palletized
    if (textureType == PDDI_TEXTYPE_PALETTIZED)
    {
        textureType = PDDI_TEXTYPE_RGB;
        bpp = 32;
    }

    lock.depth = bpp;
    lock.format = PickPixelFormat(textureType, bpp, alphaDepth);

    radMemorySetAllocationName("gxmTexture");
    bits = new uint8_t* [nMipMap + 1];
    if (type == PDDI_TEXTYPE_DXT1 || type == PDDI_TEXTYPE_DXT3 || type == PDDI_TEXTYPE_DXT5)
    {
        unsigned int blocksize = type == PDDI_TEXTYPE_DXT1 ? 8 : 16;
        for(int i = 0; i < nMipMap+1; i++)
            bits[i] = (uint8_t*)radMemorySpaceAllocAligned(radMemorySpace_Cdram, radMemoryGetCurrentAllocator(), size_t(ceil(double(xSize>>i)/4)*ceil(double(ySize>>i)/4)*blocksize), SCE_GXM_TEXTURE_ALIGNMENT);
        CHK_GXM(sceGxmTextureInitSwizzled(&texture, bits[0], PickPixelFormat(lock.format), xSize, ySize, nMipMap));
    }
    else
    {
        for(int i = 0; i < nMipMap+1; i++)
            bits[i] = (uint8_t*)radMemorySpaceAllocAligned(radMemorySpace_Cdram, radMemoryGetCurrentAllocator(), ((xSize>>i)*(ySize>>i)*bpp)/8, SCE_GXM_TEXTURE_ALIGNMENT);
        CHK_GXM(sceGxmTextureInitLinear(&texture, bits[0], PickPixelFormat(lock.format), xSize, ySize, nMipMap));
    }

    if(context->GetDisplay()->ExtBGRA())
    {
        lock.native = true;
        lock.rgbaLShift[0] = lock.rgbaRShift[0] =
        lock.rgbaLShift[1] = lock.rgbaRShift[1] =
        lock.rgbaLShift[2] = lock.rgbaRShift[2] =
        lock.rgbaLShift[3] = lock.rgbaRShift[3] = 0;

        lock.rgbaMask[0] = 0x00ff0000;
        lock.rgbaMask[1] = 0x0000ff00;
        lock.rgbaMask[2] = 0x000000ff;
        lock.rgbaMask[3] = 0xff000000;
    }
    else
    {
        lock.native = false;
        lock.rgbaRShift[0] = 16;
        lock.rgbaLShift[2] = 16;

        lock.rgbaLShift[0] = 
        lock.rgbaLShift[1] = lock.rgbaRShift[1] =
        lock.rgbaRShift[2] =
        lock.rgbaLShift[3] = lock.rgbaRShift[3] = 0;

        lock.rgbaMask[0] = 0x000000ff;
        lock.rgbaMask[1] = 0x0000ff00;
        lock.rgbaMask[2] = 0x00ff0000;
        lock.rgbaMask[3] = 0xff000000;
    }

    context->ADD_STAT(PDDI_STAT_TEXTURE_ALLOC_32BIT, (float)((xSize * ySize * lock.depth) / 8192));
    context->ADD_STAT(PDDI_STAT_TEXTURE_COUNT_32BIT, 1);

    return true;
}

gxmTexture::gxmTexture(gxmContext* c)
{
    context = c;
    contextID = c->contextID;
    bits = NULL;
    priority = 15;
}

gxmTexture::~gxmTexture()
{
    if(bits)
    {
        for(int i = 0; i < nMipMap+1; i++)
            radMemorySpaceFreeAligned(radMemorySpace_Cdram, radMemoryGetCurrentAllocator(), bits[i]);
        delete [] bits;
    }

    context->ADD_STAT(PDDI_STAT_TEXTURE_ALLOC_32BIT, -(float)((xSize * ySize * lock.depth) / 8192));
    context->ADD_STAT(PDDI_STAT_TEXTURE_COUNT_32BIT, -1);
}

pddiPixelFormat gxmTexture::GetPixelFormat()
{
    return PDDI_PIXEL_ARGB8888;
}

int   gxmTexture::GetWidth()
{
    return xSize;
}

int   gxmTexture::GetHeight()
{
    return ySize;
}

int   gxmTexture::GetDepth()
{
    return 32;
}

int   gxmTexture::GetNumMipMaps()
{
    return nMipMap;
}

int gxmTexture::GetAlphaDepth()
{
    return 8;
}

pddiLockInfo* gxmTexture::Lock(int mipMap, pddiRect* rect)
{
    PDDIASSERT(mipMap <= nMipMap);

    lock.width = 1 << (log2X-mipMap);
    lock.height = 1 << (log2Y-mipMap);
    if (lock.format == PDDI_PIXEL_DXT1 || lock.format == PDDI_PIXEL_DXT3 || lock.format == PDDI_PIXEL_DXT5)
    {
        unsigned int blocksize = lock.format == PDDI_PIXEL_DXT1 ? 8 : 16;
        lock.pitch = ceil( double( xSize >> mipMap ) / 4 ) * blocksize;
        lock.bits = radMemoryAllocAligned(radMemoryGetCurrentAllocator(), size_t(ceil(double(ySize>>mipMap)/4)*lock.pitch), 16);
    }
    else if (lock.format == PDDI_PIXEL_YUV)
    {
        lock.pitch = (lock.width * lock.depth) / 8;
        lock.bits = bits[mipMap];
    }
    else
    {
        lock.pitch = -(lock.width * 4);
        lock.bits = bits[mipMap] + (lock.width * (lock.height - 1) * 4);
    }

    return &lock;
}

void gxmTexture::Unlock(int mipLevel)
{
    if(lock.format == PDDI_PIXEL_DXT1 || lock.format == PDDI_PIXEL_DXT3 || lock.format == PDDI_PIXEL_DXT5)
    {
        uint32_t width = ceil(double(xSize >> mipLevel) / 4);
        uint32_t height = ceil(double(ySize >> mipLevel) / 4);
        if(type == PDDI_TEXTYPE_DXT5)
            SwizzleTexData128Bpp(bits[mipLevel], (uint8_t*)lock.bits, 0, 0, width, height, width, rmt::Min(width, height));
        else
			SwizzleTexData64Bpp(bits[mipLevel], (uint8_t*)lock.bits, 0, 0, width, height, width, rmt::Min(width, height));
        radMemoryFreeAligned(lock.bits);
    }
}

void gxmTexture::SetPriority(int p)
{
    priority = p;
}

int gxmTexture::GetPriority(void)
{
    return priority;
}

// paging control
void gxmTexture::Prefetch(void)
{
}

void gxmTexture::Discard(void)
{
}

// palette managment
int gxmTexture::GetNumPaletteEntries(void)
{
    return 0;
}

void gxmTexture::SetPalette(int nEntries, pddiColour* palette)
{
}

int gxmTexture::GetPalette(pddiColour* palette)
{
    return 0;
}


