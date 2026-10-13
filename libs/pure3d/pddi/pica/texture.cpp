#include <pddi/pica/pica.hpp>
#include <pddi/pica/texture.hpp>
#include <pddi/pica/context.hpp>
#include <pddi/pica/swizzle.hpp>

#include <pddi/base/debug.hpp>
#include <pddi/gl/decompress.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>

static const int PICA_STAGING_BPP = 4;

static inline int NextPowerOfTwo(int v)
{
    int p = 1;
    while(p < v)
        p <<= 1;
    return p;
}

static inline int FastLog2(int x)
{
    int r = 0;
    int tmp = x;
    while(tmp > 1)
    {
        r++;
        tmp >>= 1;
        if((tmp << r) != x)
            return -1;
    }
    return r;
}

static inline pddiPixelFormat PickPixelFormat(pddiTextureType type, int bitDepth, int alphaDepth)
{
    switch(type)
    {
    case PDDI_TEXTYPE_RGB:
        switch(alphaDepth)
        {
        case 0:  return (bitDepth <= 16) ? PDDI_PIXEL_RGB565   : PDDI_PIXEL_RGB888;
        case 1:  return (bitDepth <= 16) ? PDDI_PIXEL_ARGB1555 : PDDI_PIXEL_ARGB8888;
        default: return (bitDepth <= 16) ? PDDI_PIXEL_ARGB4444 : PDDI_PIXEL_ARGB8888;
        }

    case PDDI_TEXTYPE_PALETTIZED: return PDDI_PIXEL_PAL8;
    case PDDI_TEXTYPE_LUMINANCE:  return PDDI_PIXEL_LUM8;
    case PDDI_TEXTYPE_BUMPMAP:    return PDDI_PIXEL_DUDV88;
    case PDDI_TEXTYPE_DXT1:       return PDDI_PIXEL_DXT1;
    case PDDI_TEXTYPE_DXT2:       return PDDI_PIXEL_DXT2;
    case PDDI_TEXTYPE_DXT3:       return PDDI_PIXEL_DXT3;
    case PDDI_TEXTYPE_DXT4:       return PDDI_PIXEL_DXT4;
    case PDDI_TEXTYPE_DXT5:       return PDDI_PIXEL_DXT5;
    case PDDI_TEXTYPE_YUV:        return PDDI_PIXEL_YUV;
    }

    return PDDI_PIXEL_UNKNOWN;
}

static inline bool IsCompressed(pddiTextureType type)
{
    return (type == PDDI_TEXTYPE_DXT1) ||
           (type == PDDI_TEXTYPE_DXT2) ||
           (type == PDDI_TEXTYPE_DXT3) ||
           (type == PDDI_TEXTYPE_DXT4) ||
           (type == PDDI_TEXTYPE_DXT5);
}

static inline GPU_TEXCOLOR PickHardwareFormat(int alphaDepth, int* bytesPerPixel)
{
    *bytesPerPixel = 2;

    if(alphaDepth <= 0)
        return GPU_RGB565;
    if(alphaDepth == 1)
        return GPU_RGBA5551;

    return GPU_RGBA4;
}

picaTexture::picaTexture(picaContext* c)
{
    context      = c;
    xSize        = 0;
    ySize        = 0;
    log2X        = 0;
    log2Y        = 0;
    type         = PDDI_TEXTYPE_RGB;
    nMipMap      = 0;
    alphaDepth   = 0;
    priority     = 15;
    created      = false;
    valid        = false;
    hwFormat     = GPU_RGBA8;
    staging      = NULL;
    stagingBytes = 0;
    stagingLevel = -1;
    memset(&tex, 0, sizeof(tex));
}

picaTexture::~picaTexture()
{
    FreeStaging();

    if(created)
    {
        C3D_TexDelete(&tex);
        created = false;

        context->ADD_STAT(PDDI_STAT_TEXTURE_COUNT_32BIT, -1);
    }
}

void picaTexture::FreeStaging(void)
{
    if(staging)
    {
        free(staging);
        staging = NULL;
    }
    stagingBytes = 0;
    stagingLevel = -1;
}

bool picaTexture::Create(int x, int y, int bpp, int alpha, int nMip,
                         pddiTextureType textureType, pddiTextureUsageHint usageHint)
{
    xSize      = x;
    ySize      = y;
    nMipMap    = nMip;
    type       = textureType;
    alphaDepth = alpha;

    log2X = FastLog2(xSize);
    log2Y = FastLog2(ySize);

    if((xSize > PICA_MAX_TEXTURE_DIMENSION) || (ySize > PICA_MAX_TEXTURE_DIMENSION))
    {
        lastError = PDDI_TEX_TOO_BIG;
        return false;
    }

    if(textureType == PDDI_TEXTYPE_YUV)
    {
        lastError = PDDI_TEX_BADFORMAT;
        return false;
    }

    if(textureType == PDDI_TEXTYPE_PALETTIZED)
    {
        textureType = PDDI_TEXTYPE_RGB;
        type        = PDDI_TEXTYPE_RGB;
        bpp         = 32;
    }

    if(IsCompressed(textureType))
    {
        alpha = (textureType == PDDI_TEXTYPE_DXT1) ? 1 : 8;
    }

    int texW = NextPowerOfTwo(xSize);
    int texH = NextPowerOfTwo(ySize);
    if(texW < PICA_MIN_TEXTURE_DIMENSION) texW = PICA_MIN_TEXTURE_DIMENSION;
    if(texH < PICA_MIN_TEXTURE_DIMENSION) texH = PICA_MIN_TEXTURE_DIMENSION;

    if(!g_picaMipmapsAllowed && !g_picaKeepFullSize)
    {
        while((texW > 256 || texH > 256) &&
              texW > PICA_MIN_TEXTURE_DIMENSION && texH > PICA_MIN_TEXTURE_DIMENSION)
        {
            texW >>= 1;
            texH >>= 1;
        }
    }

    if((texW > PICA_MAX_TEXTURE_DIMENSION) || (texH > PICA_MAX_TEXTURE_DIMENSION))
    {
        lastError = PDDI_TEX_TOO_BIG;
        return false;
    }

    int bytesPerPixel = 4;
    hwFormat = PickHardwareFormat(alpha, &bytesPerPixel);

    const unsigned mipExtra = (unsigned)(texW * texH * bytesPerPixel) / 3;
    const bool roomForMips = linearSpaceFree() > (PICA_MIPMAP_LINEAR_RESERVE + mipExtra);
    const bool wantMips = g_picaMipmapsAllowed && roomForMips &&
                          (texW >= 2 * PICA_MIN_TEXTURE_DIMENSION) &&
                          (texH >= 2 * PICA_MIN_TEXTURE_DIMENSION);

    if(wantMips ? !C3D_TexInitMipmap(&tex, (u16)texW, (u16)texH, hwFormat)
                : !C3D_TexInit(&tex, (u16)texW, (u16)texH, hwFormat))
    {
        lastError = PDDI_ERR;
        return false;
    }
    created = true;

    C3D_TexSetFilter(&tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&tex, GPU_REPEAT, GPU_REPEAT);
    C3D_TexSetFilterMipmap(&tex, GPU_NEAREST);

    lock.depth  = bpp;
    lock.format = PickPixelFormat(textureType, bpp, alphaDepth);

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

    context->ADD_STAT(PDDI_STAT_TEXTURE_COUNT_32BIT, 1);
    context->ADD_STAT(PDDI_STAT_TEXTURE_ALLOC_32BIT,
                      (float)((texW * texH * bytesPerPixel * 8) / 8192));

    return true;
}

pddiPixelFormat picaTexture::GetPixelFormat() { return lock.format; }
int picaTexture::GetWidth()                   { return xSize; }
int picaTexture::GetHeight()                  { return ySize; }
int picaTexture::GetDepth()                   { return 32; }
int picaTexture::GetNumMipMaps()              { return nMipMap; }
int picaTexture::GetAlphaDepth()              { return alphaDepth; }

pddiLockInfo* picaTexture::Lock(int mipMap, pddiRect* rect)
{
    PDDIASSERT(mipMap <= nMipMap);

    if(!created)
        return NULL;

    const int w = (xSize >> mipMap) > 0 ? (xSize >> mipMap) : 1;
    const int h = (ySize >> mipMap) > 0 ? (ySize >> mipMap) : 1;

    unsigned needed;
    if(IsCompressed(type))
    {
        const unsigned blockSize = (type == PDDI_TEXTYPE_DXT1) ? 8u : 16u;
        const unsigned blocksW = (unsigned)((w + 3) / 4);
        const unsigned blocksH = (unsigned)((h + 3) / 4);
        lock.pitch = (int)(blocksW * blockSize);
        needed = blocksW * blocksH * blockSize;
    }
    else
    {
        lock.pitch = w * PICA_STAGING_BPP;
        needed = (unsigned)(w * h * PICA_STAGING_BPP);
    }

    if(staging && (stagingBytes < needed))
        FreeStaging();

    if(staging == NULL)
    {
        staging = (unsigned char*)calloc(1, needed);
        if(staging == NULL)
            return NULL;
        stagingBytes = needed;
    }

    stagingLevel = mipMap;

    lock.width  = w;
    lock.height = h;
    lock.bits   = staging;

    return &lock;
}

void picaTexture::Unlock(int mipLevel)
{
    if((staging == NULL) || !created)
        return;

    if(mipLevel == 0)
        UploadLevel(0);

    FreeStaging();
}

extern "C" int g_picaMipmapsAllowed = 0;
extern "C" int g_picaKeepFullSize = 0;

void picaTexture::UploadLevel(int mipLevel)
{
    const int srcW = (xSize >> mipLevel) > 0 ? (xSize >> mipLevel) : 1;
    const int srcH = (ySize >> mipLevel) > 0 ? (ySize >> mipLevel) : 1;

    const int dstW = tex.width;
    const int dstH = tex.height;

    unsigned char* decompressed = NULL;
    const unsigned char* src = staging;

    if(IsCompressed(type))
    {
        decompressed = (unsigned char*)malloc((size_t)srcW * srcH * 4);
        if(decompressed == NULL)
            return;

        switch(type)
        {
            case PDDI_TEXTYPE_DXT1:
                BlockDecompressImageBC1(srcW, srcH, staging, decompressed);
                break;
            case PDDI_TEXTYPE_DXT2:
            case PDDI_TEXTYPE_DXT3:
                BlockDecompressImageBC2(srcW, srcH, staging, decompressed);
                break;
            default:
                BlockDecompressImageBC3(srcW, srcH, staging, decompressed);
                break;
        }
        src = decompressed;
    }

    unsigned int* level = NULL;
    bool levelOwned = false;
    if(decompressed != NULL && dstW == srcW && dstH == srcH)
    {
        level = (unsigned int*)decompressed;
        for(int y = 0; y < srcH / 2; y++)
        {
            unsigned int* a = level + (size_t)y * srcW;
            unsigned int* b = level + (size_t)(srcH - 1 - y) * srcW;
            for(int x = 0; x < srcW; x++)
            {
                const unsigned int t = a[x]; a[x] = b[x]; b[x] = t;
            }
        }
    }
    else if(decompressed == NULL && dstW == srcW && dstH == srcH)
    {
        level = (unsigned int*)staging;
    }
    else
    {
        level = (unsigned int*)malloc((size_t)dstW * dstH * 4);
        if(level == NULL)
        {
            if(decompressed)
                free(decompressed);
            return;
        }
        levelOwned = true;

        const bool boxShrink = (srcW >= dstW) && (srcH >= dstH) &&
                               (srcW > dstW || srcH > dstH) &&
                               (srcW % dstW == 0) && (srcH % dstH == 0);
        if(boxShrink)
        {
            const int rx = srcW / dstW;
            const int ry = srcH / dstH;
            const unsigned n = (unsigned)(rx * ry);
            for(int y = 0; y < dstH; y++)
            {
                unsigned int* row = level + (size_t)y * dstW;
                for(int x = 0; x < dstW; x++)
                {
                    unsigned r = 0, g = 0, b = 0, a = 0;
                    for(int j = 0; j < ry; j++)
                    {
                        const int sy0 = y * ry + j;
                        const int sy  = IsCompressed(type) ? (srcH - 1 - sy0) : sy0;
                        const unsigned int* srcRow =
                            (const unsigned int*)(src + ((size_t)sy * srcW * 4)) + x * rx;
                        for(int i = 0; i < rx; i++)
                        {
                            const unsigned p = srcRow[i];
                            r += p & 0xff;
                            g += (p >> 8) & 0xff;
                            b += (p >> 16) & 0xff;
                            a += p >> 24;
                        }
                    }
                    row[x] = (r / n) | ((g / n) << 8) | ((b / n) << 16) | ((a / n) << 24);
                }
            }
        }
        else
        for(int y = 0; y < dstH; y++)
        {
            const int sy0 = (dstH == srcH) ? y : ((y * srcH) / dstH);
            const int sy  = IsCompressed(type) ? (srcH - 1 - sy0) : sy0;
            const unsigned int* srcRow = (const unsigned int*)(src + ((size_t)sy * srcW * 4));
            unsigned int* row = level + (size_t)y * dstW;
            for(int x = 0; x < dstW; x++)
                row[x] = srcRow[(dstW == srcW) ? x : ((x * srcW) / dstW)];
        }
    }

    ConvertAndTile(level, dstW, dstH, tex.data);

    int lw = dstW, lh = dstH;
    for(int mip = 1; mip <= tex.maxLevel; mip++)
    {
        const int nw = lw / 2, nh = lh / 2;
        for(int y = 0; y < nh; y++)
        {
            if(g_picaYieldHook)
                g_picaYieldHook();

            const unsigned int* r0 = level + (size_t)(2 * y) * lw;
            const unsigned int* r1 = r0 + lw;
            unsigned int* out = level + (size_t)y * nw;
            for(int x = 0; x < nw; x++)
            {
                const unsigned int p0 = r0[2 * x], p1 = r0[2 * x + 1];
                const unsigned int p2 = r1[2 * x], p3 = r1[2 * x + 1];
                const unsigned int lo = (((p0 & 0x00ff00ffu) + (p1 & 0x00ff00ffu) +
                                          (p2 & 0x00ff00ffu) + (p3 & 0x00ff00ffu) +
                                          0x00020002u) >> 2) & 0x00ff00ffu;
                const unsigned int hi = ((((p0 >> 8) & 0x00ff00ffu) + ((p1 >> 8) & 0x00ff00ffu) +
                                          ((p2 >> 8) & 0x00ff00ffu) + ((p3 >> 8) & 0x00ff00ffu) +
                                          0x00020002u) >> 2) & 0x00ff00ffu;
                out[x] = lo | (hi << 8);
            }
        }
        lw = nw; lh = nh;
        ConvertAndTile(level, lw, lh, C3D_TexGetImagePtr(&tex, tex.data, mip, NULL));
    }

    if(levelOwned)
        free(level);
    if(decompressed)
        free(decompressed);

    C3D_TexFlush(&tex);
    valid = true;
}

void picaTexture::ConvertAndTile(const unsigned int* rgba, int w, int h, void* dstData)
{
    unsigned char* dst = (unsigned char*)dstData;

    for(int y = 0; y < h; y++)
    {
        if(g_picaYieldHook)

        {
            g_picaYieldHook();
        }

        const unsigned int* srcRow = rgba + (size_t)y * w;

        for(int x = 0; x < w; x++)
        {
            const unsigned int p = srcRow[x];

            const unsigned r = (p      ) & 0xff;
            const unsigned g = (p >>  8) & 0xff;
            const unsigned b = (p >> 16) & 0xff;
            const unsigned a = (p >> 24) & 0xff;

            const unsigned offset = picaSwizzleOffset(x, y, w);

            switch(hwFormat)
            {
                case GPU_RGBA8:
                    ((unsigned int*)dst)[offset] =
                        (r << 24) | (g << 16) | (b << 8) | a;
                    break;

                case GPU_RGBA5551:
                    ((unsigned short*)dst)[offset] = (unsigned short)
                        (((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | (a >> 7));
                    break;

                case GPU_RGBA4:
                    ((unsigned short*)dst)[offset] = (unsigned short)
                        (((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | (a >> 4));
                    break;

                default:
                    ((unsigned short*)dst)[offset] = (unsigned short)
                        (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
                    break;
            }
        }
    }
}

void picaTexture::SetPriority(int p) { priority = p; }
int  picaTexture::GetPriority(void)  { return priority; }

void picaTexture::Prefetch(void) { }
void picaTexture::Discard(void)  { }

int  picaTexture::GetNumPaletteEntries(void)                   { return 0; }
void picaTexture::SetPalette(int nEntries, pddiColour* palette) { }
int  picaTexture::GetPalette(pddiColour* palette)              { return 0; }
