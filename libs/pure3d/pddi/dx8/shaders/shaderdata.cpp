//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "direct3d.hpp"

namespace ShaderData
{
    
    // pddiShadeMode
    D3DSHADEMODE shadeTable[] = 
    {
            D3DSHADE_FLAT,
            D3DSHADE_GOURAUD
    };
    
    // pddiBlendFactor
    D3DBLEND blendTable[] = 
    {
            D3DBLEND_ZERO,
            D3DBLEND_ONE,
            D3DBLEND_SRCCOLOR,
            D3DBLEND_INVSRCCOLOR,
            D3DBLEND_DESTCOLOR,   
            D3DBLEND_INVDESTCOLOR,
            D3DBLEND_SRCALPHA,
            D3DBLEND_INVSRCALPHA,
            D3DBLEND_DESTALPHA,   
            D3DBLEND_INVDESTALPHA,
            D3DBLEND_SRCALPHASAT
    };
    
    // pddiCompareMode
    D3DCMPFUNC compareTable[] =
    {
            D3DCMP_NEVER,
            D3DCMP_ALWAYS,
            D3DCMP_LESS,
            D3DCMP_LESSEQUAL,
            D3DCMP_GREATER,
            D3DCMP_GREATEREQUAL,
            D3DCMP_EQUAL,
            D3DCMP_NOTEQUAL
    };
    
}


