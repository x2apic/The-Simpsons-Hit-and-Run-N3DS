//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "layeredshader.hpp"
#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "shader.hpp"
#include "direct3d.hpp"
#include "layeredlmapshader.hpp"

pddiShadeTextureTable d3dLayeredLMapShader::textureTable[] = 
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&SetTexture)},
    {PDDI_SP_TOPTEX , SHADE_TEXTURE(&SetTopTexture)},
    {PDDI_SP_LIGHTMAP , SHADE_TEXTURE(&SetLightMap)},
    {PDDI_SP_NULL , NULL}
};

//-----------------------------------------------------------------------------
//
// d3dLayeredShader::d3dLayeredShader
//
//-----------------------------------------------------------------------------
d3dLayeredLMapShader::d3dLayeredLMapShader(d3dContext* c)
: d3dLayeredShader( c ),
  lightMap( NULL )
{
}

d3dLayeredLMapShader::~d3dLayeredLMapShader( void )
{
    if( lightMap )
        lightMap->Release( );
}

//-----------------------------------------------------------------------------
//
// d3dLayeredLMapShader::GetType
//
//-----------------------------------------------------------------------------
const char* d3dLayeredLMapShader::GetType(void)
{
    static char type [] = "layeredlmap";
    return type;
}

//-----------------------------------------------------------------------------
//
// d3dLayeredLMapShader::GetPasses
//
//-----------------------------------------------------------------------------
int  d3dLayeredLMapShader::GetPasses()
{
     return 1;
}

//-----------------------------------------------------------------------------
//
// 
//
//-----------------------------------------------------------------------------
void d3dLayeredLMapShader::SetPass(int pass)
{
    // set base texture
    d3d->SetTextures(3, texture, topText, lightMap);
    
    d3d->SetTextureCombine(0, D3DTOP_MODULATE, D3DTA_DIFFUSE, D3DTA_TEXTURE);
    SetupTextureSampling(0);

    d3d->SetTextureCombine(1, GetBlendOp( texBlendMode ), D3DTA_CURRENT, D3DTA_TEXTURE);
    SetupTextureSampling(1);

    d3d->SetTextureCombine(2, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEXTURE);
    SetupTextureSampling(2);

    SetupShading();
    SetupAlphaBlend();
}


//-----------------------------------------------------------------------------
//
// d3dSimpleShader::SetLightMap
//
//-----------------------------------------------------------------------------
void d3dLayeredLMapShader::SetLightMap(pddiTexture* t)
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
