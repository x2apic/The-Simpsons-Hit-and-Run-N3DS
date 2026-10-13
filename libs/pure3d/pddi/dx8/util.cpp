/*===========================================================================
    ddutil.hpp
    05-Dec-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include "../pddi.hpp"
#include "util.hpp"
#include "direct3d.hpp"
#include "../base/debug.hpp"

void DDVERIFY(HRESULT res)
{
    if(res == D3D_OK)
    {
        return;
    }

    char errStr[256];
    D3DXGetErrorString(res, errStr, 256);

    char errBuf[512];
    wsprintf(errBuf, "%s\nCode: %d, facility: %d, severity: %d",
        errStr, HRESULT_CODE(res), HRESULT_FACILITY(res), HRESULT_SEVERITY(res));

    OutputDebugString(errBuf);
    OutputDebugString("\n");

     int result = MessageBox(NULL, errBuf, "DirectX Error", MB_RETRYCANCEL | MB_ICONERROR);
     if(result == IDCANCEL)
     {
         exit(-10);
     }
     else
     {
         __asm int 3
     }
}

void BaseFillLockInfo(D3DFORMAT format, pddiLockInfo* lockInfo)
{
    int rMask, gMask, bMask, aMask;
    rMask = gMask = bMask = aMask = 0;

    switch(format)
    {
        case D3DFMT_A8R8G8B8:             
            aMask = 0xff000000;
        case D3DFMT_X8R8G8B8:             
            rMask = 0x00ff0000; gMask = 0x0000ff00; bMask = 0x000000ff;
            lockInfo->format = PDDI_PIXEL_ARGB8888;
            break;

        case D3DFMT_R5G6B5:               
            rMask = 0x0000f800; gMask = 0x000007e0; bMask = 0x0000001f;
            lockInfo->format = PDDI_PIXEL_RGB565;
            break;

        case D3DFMT_X1R5G5B5:             
            rMask = 0x00007c00; gMask = 0x000003e0; bMask = 0x0000001f;
            lockInfo->format = PDDI_PIXEL_RGB555;
            break;

        case D3DFMT_A1R5G5B5:             
            aMask = 0x00008000; rMask = 0x00007c00; gMask = 0x000003e0; bMask = 0x0000001f;
            lockInfo->format = PDDI_PIXEL_ARGB1555;
            break;

        case D3DFMT_A4R4G4B4:             
            aMask = 0x0000f000; rMask = 0x00000f00; gMask = 0x000000f0; bMask = 0x0000000f;
            lockInfo->format = PDDI_PIXEL_ARGB4444;
            break;

        case D3DFMT_DXT1:
             lockInfo->format = PDDI_PIXEL_DXT1;
        break;

        case D3DFMT_DXT2:
             lockInfo->format = PDDI_PIXEL_DXT2;
        break;

        case D3DFMT_DXT3:
             lockInfo->format = PDDI_PIXEL_DXT3;
        break;

        case D3DFMT_DXT4:
             lockInfo->format = PDDI_PIXEL_DXT4;
        break;

        case D3DFMT_DXT5:
             lockInfo->format = PDDI_PIXEL_DXT5;
        break;

        case D3DFMT_P8:
             lockInfo->format = PDDI_PIXEL_PAL8;
        break;

        case D3DFMT_V8U8:
             lockInfo->format = PDDI_PIXEL_DUDV88;
        break;

        default:
            PDDIASSERT(0);
    }

    lockInfo->rgbaMask[0] = rMask;
    lockInfo->rgbaMask[1] = gMask;
    lockInfo->rgbaMask[2] = bMask;
    lockInfo->rgbaMask[3] = aMask;

    lockInfo->rgbaRShift[0] = (8 - BitCount(rMask)) + 16;
    lockInfo->rgbaRShift[1] = (8 - BitCount(gMask)) + 8;
    lockInfo->rgbaRShift[2] = (8 - BitCount(bMask));
    lockInfo->rgbaRShift[3] = (8 - BitCount(aMask)) + 24;
    lockInfo->rgbaLShift[0] = FirstSetBit(rMask);
    lockInfo->rgbaLShift[1] = FirstSetBit(gMask);
    lockInfo->rgbaLShift[2] = FirstSetBit(bMask);
    lockInfo->rgbaLShift[3] = FirstSetBit(aMask);

    int rCount = BitCount(rMask);
    int gCount = BitCount(gMask);
    int bCount = BitCount(bMask);
    int aCount = BitCount(aMask);

    lockInfo->native = (rMask==0x00ff0000 && gMask==0x0000ff00 && bMask==0x000000ff && aMask==0xff000000);
}

//------------------------------------------------------------------------
void FillLockInfo(D3DSURFACE_DESC* sd, D3DLOCKED_RECT* rect, pddiLockInfo* lockInfo, bool isTexture)
{
    lockInfo->width = sd->Width;
    lockInfo->height = sd->Height;
    lockInfo->volDepth = 1;
    lockInfo->depth = DisplayBitDepth(sd->Format);

    if(rect)
    {
         //I am not sure what's the offset set for lockInfo->bits used for when isTexture is true
         //I add the condition to force the lockInfo->bits to be the same as rect->pBits
         //for DXTn format. Need ask Nealle what is the problem
        if(isTexture && sd->Format != D3DFMT_DXT1 &&
            sd->Format != D3DFMT_DXT2 && sd->Format != D3DFMT_DXT3 &&
            sd->Format != D3DFMT_DXT4 && sd->Format != D3DFMT_DXT5 )
        {
            lockInfo->bits = (char*)rect->pBits + ((lockInfo->height-1) * rect->Pitch);
            lockInfo->pitch = -rect->Pitch;
        }
        else
        {
            lockInfo->bits = (char*)rect->pBits;
            lockInfo->pitch = rect->Pitch;
        }
    }

    BaseFillLockInfo(sd->Format, lockInfo);
}

//------------------------------------------------------------------------
void FillLockInfo(D3DVOLUME_DESC* vd, D3DLOCKED_BOX* box, pddiLockInfo* lockInfo, bool isTexture)
{
    lockInfo->width = vd->Width;
    lockInfo->height = vd->Height;
    lockInfo->volDepth = vd->Depth;
    lockInfo->depth = DisplayBitDepth(vd->Format);

    if(box)
    {
         //I am not sure what's the offset set for lockInfo->bits used for when isTexture is true
         //I add the condition to force the lockInfo->bits to be the same as rect->pBits
         //for DXTn format. Need ask Nealle what is the problem
        if(isTexture && vd->Format != D3DFMT_DXT1 &&
            vd->Format != D3DFMT_DXT2 && vd->Format != D3DFMT_DXT3 &&
            vd->Format != D3DFMT_DXT4 && vd->Format != D3DFMT_DXT5 )
        {
            lockInfo->bits = (char*)box->pBits + ((lockInfo->height-1) * box->RowPitch);           
            lockInfo->pitch = -box->RowPitch;
            lockInfo->slice = box->SlicePitch;
        }
        else
        {
            lockInfo->bits = (char*)box->pBits;
            lockInfo->pitch = box->RowPitch;
            lockInfo->slice = box->SlicePitch;
        }
    }

    BaseFillLockInfo(vd->Format, lockInfo);
}

