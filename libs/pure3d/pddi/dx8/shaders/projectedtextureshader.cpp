//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "ProjectedTextureShader.hpp"
#include "direct3d.hpp"

pddiShadeColourTable d3dProjectedTextureShader::colourTable[] = 
{
    PDDID3D_STDSHADERPARAM_COLOUR,
    { PDDI_SP_NULL,     NULL }
};

pddiShadeTextureTable d3dProjectedTextureShader::textureTable[] = 
{
    { PDDI_SP_BASETEX, SHADE_TEXTURE(&SetBaseTexture) },
    { PDDI_SP_PROJTEX, SHADE_TEXTURE(&SetProjTexture) },
    { PDDI_SP_NULL,    NULL }
};

pddiShadeIntTable d3dProjectedTextureShader::intTable[] = 
{
    PDDID3D_STDSHADERPARAM_INT,
    { PDDI_SP_NULL,         NULL }
};

pddiShadeFloatTable d3dProjectedTextureShader::floatTable[] = 
{
    PDDID3D_STDSHADERPARAM_FLOAT,
    { PDDI_SP_NULL , NULL}
};

pddiShadeMatrixTable d3dProjectedTextureShader::matrixTable[] = 
{
    { PDDI_SP_PROJMATRIX,   SHADE_MATRIX(&SetProjMatrix) },
    { PDDI_SP_NULL, NULL }
};


d3dProjectedTextureShader::d3dProjectedTextureShader(d3dContext* c) :
    d3dShader(c)
{ 
    baseTex = NULL;
    projTex = NULL;
    projMat.Identity();
}

d3dProjectedTextureShader::~d3dProjectedTextureShader()
{
    if(baseTex)
        baseTex->Release();
    if(projTex)
        projTex->Release();
}

const char* d3dProjectedTextureShader::GetType(void)
{
    static char type [] = "texproj";
    return type;
}

int  d3dProjectedTextureShader::GetPasses()
{
    return 1;
}

void d3dProjectedTextureShader::SetPass(int pass)
{
    d3d->SetTextures(2, baseTex, projTex);

    d3d->SetTextureCombine(0, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEXTURE);
    d3d->SetTextureCoord(0, 0);
    d3d->SetTextureTransform(0, D3DTTFF_DISABLE);
    d3d->SetTextureAddress(0, PDDI_UV_TILE);
    d3d->SetTextureFilter(0, filterMode);
    
    d3d->SetTextureCombine(1, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEXTURE);
    d3d->SetTextureCoord(1, D3DTSS_TCI_CAMERASPACEPOSITION );
    d3d->SetTextureTransform(1, D3DTTFF_PROJECTED | D3DTTFF_COUNT3);
    d3d->SetTextureAddress(1, PDDI_UV_CLAMP);
    d3d->SetTextureFilter(1, filterMode);

    SetupShading();
    SetupAlphaBlend();
}

void d3dProjectedTextureShader::PreRender()
{
    d3dShader::PreRender();
    context->PushMatrix(PDDI_MATRIX_TEXTURE1);
    context->LoadMatrix(PDDI_MATRIX_TEXTURE1,&projMat);
}

void d3dProjectedTextureShader::PostRender()
{
    d3dShader::PostRender();
    context->PopMatrix(PDDI_MATRIX_TEXTURE1);
}

//-------------------------------------------------------------------
void d3dProjectedTextureShader::SetBaseTexture(pddiTexture* t)
{
    if(t)
        t->AddRef();

    if(baseTex)
        baseTex->Release();

    baseTex = (d3dTexture*)t;
}

//-------------------------------------------------------------------
void d3dProjectedTextureShader::SetProjTexture(pddiTexture* t)
{
    if(t)
        t->AddRef();

    if(projTex)
        projTex->Release();

    projTex = (d3dTexture*)t;
}

//-------------------------------------------------------------------
// projector's matrix (precombine camera to projector space matrix with perspective projection)
void d3dProjectedTextureShader::SetProjMatrix(const pddiMatrix& matrix)
{  
    projMat = matrix;
}

