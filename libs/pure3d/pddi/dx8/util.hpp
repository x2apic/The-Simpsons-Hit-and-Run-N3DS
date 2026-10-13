/*===========================================================================
    ddutil.hpp
    05-Dec-99 Created by Neall

    Copyright (c)1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _DDUTIL_HPP
#define _DDUTIL_HPP

#include "direct3d.hpp"
#include "..\pddienum.hpp"
#include "..\pdditype.hpp"

struct pddiLockInfo;

void DDVERIFY(HRESULT res);
void FillLockInfo(D3DSURFACE_DESC* sd, D3DLOCKED_RECT* rect, pddiLockInfo* lockInfo, bool isTexture);
void FillLockInfo(D3DVOLUME_DESC* vd, D3DLOCKED_BOX* box, pddiLockInfo* lockInfo, bool isTexture);

static inline int DisplayBitDepth(D3DFORMAT format)
{
    switch(format)
    {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            return 32;
        case D3DFMT_R5G6B5  :
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_V8U8:
            return 16;
        case D3DFMT_P8:
            return 8;
        default:
            return 0;
    }
}

static inline int VertsToPrims(pddiPrimType type, int verts)
{
    switch(type)
    {
        case PDDI_PRIM_TRIANGLES:
            return verts/3;
        case PDDI_PRIM_TRISTRIP:
            return verts - 2;
        case PDDI_PRIM_LINES:
            return verts / 2;
        case PDDI_PRIM_LINESTRIP:
            return verts - 1;
        case PDDI_PRIM_POINTS:
            return verts;
    }
    return 0;
}

//-------------------------------------------------------------------
static inline bool IsPowerOf2(int x)
{
    return (!((x) & ((x) - 1)));
}

//-------------------------------------------------------------------
static inline int FirstSetBit(unsigned x)
{
    for(int i=0; i < sizeof(unsigned)*8; i++)
    {
        if(x & (1<<i))
        {
            return i;
        }
    }
    return 0;
}

//-------------------------------------------------------------------
static inline int BitCount(unsigned bits)
{
    int count = 0;
    for(int i=0; i < 32; i++)
        if(bits & (1<<i)) count++;
    return count;
}

//-------------------------------------------------------------------
// converts pddi's byte based colour to d3d's float based colour
static inline void d3dColourValue(pddiColour colour, D3DCOLORVALUE* value)
{
    value->r = (float)colour.Red() * (1.0f / 255.0f);
    value->g = (float)colour.Green() * (1.0f / 255.0f);
    value->b = (float)colour.Blue() * (1.0f / 255.0f);
    value->a = (float)colour.Alpha() * (1.0f / 255.0f);
}

#endif /* _DDUTIL_HPP */
