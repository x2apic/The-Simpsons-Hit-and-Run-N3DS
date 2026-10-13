//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "shader.hpp"
#include "reflectshader.hpp"
#include "direct3d.hpp"


pddiShadeTextureTable d3dReflShader::textureTable[] = 
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&SetTexture)},
    {PDDI_SP_REFLMAP , SHADE_TEXTURE(&SetReflectionMap)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeColourTable d3dReflShader::colourTable[] = 
{
    PDDID3D_STDSHADERPARAM_COLOUR,
    {PDDI_SP_ENVBLEND , SHADE_COLOUR(&SetEnvBlend)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeIntTable d3dReflShader::intTable[] = 
{
    PDDID3D_STDSHADERPARAM_INT, 
    {PDDI_SP_NULL , NULL}
};

pddiShadeFloatTable d3dReflShader::floatTable[] = 
{
    PDDID3D_STDSHADERPARAM_FLOAT, 
    {PDDI_SP_NULL , NULL}
};

//-----------------------------------------------------------------------------
//
// d3dReflShader::d3dReflShader
//
//-----------------------------------------------------------------------------
d3dReflShader::d3dReflShader(d3dContext* c) :
    d3dShader(c),
    texture(NULL),
    reflectionMap(NULL),
    envBlend(128,128,128,128)
{
    //
}

d3dReflShader::~d3dReflShader()
{
    if(texture) texture->Release();
    if(reflectionMap) reflectionMap->Release();
}

//-----------------------------------------------------------------------------
//
// d3dReflShader::GetType
//
//-----------------------------------------------------------------------------
const char* d3dReflShader::GetType(void)
{
    static char type [] = "reflection";
    return type;
}

//-----------------------------------------------------------------------------
//
// d3dReflShader::GetPasses
//
//-----------------------------------------------------------------------------
int  d3dReflShader::GetPasses()
{
    return 1;
}

//-----------------------------------------------------------------------------
//
// 
//
//-----------------------------------------------------------------------------
void d3dReflShader::SetPass(int pass)
{
    d3d->SetTextures(2, texture, reflectionMap);

    d3d->SetTextureFactor(envBlend);

    d3d->SetTextureCombine(0, D3DTOP_MODULATE, D3DTA_DIFFUSE, D3DTA_TEXTURE);
    d3d->SetTextureCombineResult(0, D3DTA_TEMP);
    SetupTextureSampling(0);

    d3d->SetTextureCombine(1, D3DTOP_MODULATE, D3DTA_TFACTOR, D3DTA_TEXTURE);
    d3d->SetTextureCoord(1, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
    d3d->SetTextureTransform(1, D3DTTFF_COUNT2);
    d3d->SetTextureAddress(1, uvMode);
    d3d->SetTextureFilter(1, filterMode);

    d3d->SetTextureCombine(2, D3DTOP_ADD, D3DTA_CURRENT, D3DTA_TEMP);

    SetupShading();
    SetupAlphaBlend();
}

void d3dReflShader::PreRender(void)
{
    d3d->SetTextureCombineResult(0, D3DTA_DIFFUSE);
    context->PushMatrix(PDDI_MATRIX_TEXTURE1);

    pddiMatrix mat;
    mat.Row4(0).Set(0.5f, 0.0f, 0.0f, 0.0f); 
    mat.Row4(1).Set(0.0f,-0.5f, 0.0f, 0.0f); 
    mat.Row4(2).Set(0.0f, 0.0f, 1.0f, 0.0f); 
    mat.Row4(3).Set(0.5f, 0.5f, 0.0f, 1.0f); 
    context->LoadMatrix(PDDI_MATRIX_TEXTURE1, &mat);
}

void d3dReflShader::PostRender(void)
{
    d3d->SetTextureCombineResult(0, D3DTA_DIFFUSE);
    context->PopMatrix(PDDI_MATRIX_TEXTURE1);
}

//-----------------------------------------------------------------------------
//
// d3dReflShader::SetTexture
//
//-----------------------------------------------------------------------------
void d3dReflShader::SetTexture(pddiTexture* t)
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
// d3dReflShader::SetReflectionMap
//
//-----------------------------------------------------------------------------
void d3dReflShader::SetReflectionMap(pddiTexture* t)
{
    if(t)
    {
        t->AddRef();
    }

    if(reflectionMap)
    {
        reflectionMap->Release();
    }

    reflectionMap = (d3dTexture*)t;
}


//-------------------------------------------------------
void d3dReflShader::SetEnvBlend(pddiColour c) 
{
    envBlend = c;
}
