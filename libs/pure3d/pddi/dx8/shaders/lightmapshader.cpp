//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "lightmapshader.hpp"
#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "shader.hpp"
#include "direct3d.hpp"

pddiShadeTextureTable d3dLightMapShader::textureTable[] = 
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&SetTexture)},
    {PDDI_SP_LIGHTMAP , SHADE_TEXTURE(&SetLightMap)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeIntTable d3dLightMapShader::intTable[] = 
{
    PDDID3D_STDSHADERPARAM_INT, 
    {PDDI_SP_NULL , NULL}
};
//-----------------------------------------------------------------------------
//
// d3dLightMapShader::d3dLightMapShader
//
//-----------------------------------------------------------------------------
d3dLightMapShader::d3dLightMapShader(d3dContext* c) :
    d3dShader(c),
    texture(NULL),
    lightMap(NULL)
{
}

d3dLightMapShader::~d3dLightMapShader( void )
{
     if(texture)
     {
         texture->Release();
     }
 
     if(lightMap)
     {
         lightMap->Release();
     }
}


//-----------------------------------------------------------------------------
//
// d3dLightMapShader::GetType
//
//-----------------------------------------------------------------------------
const char* d3dLightMapShader::GetType(void)
{
    static char type [] = "lightmap";
    return type;
}

//-----------------------------------------------------------------------------
//
// d3dLightMapShader::GetPasses
//
//-----------------------------------------------------------------------------
int  d3dLightMapShader::GetPasses()
{
    return 1;
}

//-----------------------------------------------------------------------------
//
// 
//
//-----------------------------------------------------------------------------
void d3dLightMapShader::SetPass(int pass)
{
    //
    // do all SetTexture stuff here
    //

    // set base texture
    d3d->SetTextures(2, texture, lightMap);

    
    d3d->SetTextureCombine(0, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEXTURE);
    SetupTextureSampling(0);

    d3d->SetTextureCombine(1, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEXTURE);
    SetupTextureSampling(1);

    d3d->SetMaterial(false, twoSided, NULL);

    SetupAlphaBlend();
}

//-----------------------------------------------------------------------------
//
// d3dLightMapShader::SetTexture
//
//-----------------------------------------------------------------------------
void d3dLightMapShader::SetTexture(pddiTexture* t)
{
    if(t)
    {
        t->AddRef();
    }

    if(texture)
    {
        texture->Release();
    }

    texture = (d3dTexture*)t;
}

//-----------------------------------------------------------------------------
//
// d3dLightMapShader::SetLightMap
//
//-----------------------------------------------------------------------------
void d3dLightMapShader::SetLightMap(pddiTexture* t)
{
    if(t)
    {
        t->AddRef();
    }

    if(lightMap)
    {
        lightMap->Release();
    }

    lightMap = (d3dTexture*)t;
}


