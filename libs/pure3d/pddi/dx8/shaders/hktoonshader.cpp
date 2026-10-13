#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "hktoonshader.hpp"
#include "direct3d.hpp"

#include "../psh/hktoonshader.h"

//#define  TOON_USE_PS


const float toonIntensity = 0.7f;

const D3DCOLOR toonIntensityColor = D3DCOLOR_ARGB( 0xff, ((int)(toonIntensity*0xff)), ((int)(toonIntensity*0xff)), ((int)(toonIntensity*0xff)) );

//-------------------------------------------------------------------
pddiShadeColourTable d3dHKToonShader::colourTable[] = 
{
    PDDID3D_STDSHADERPARAM_COLOUR,
    {PDDI_SP_NULL , NULL}
};

pddiShadeTextureTable d3dHKToonShader::textureTable[] = 
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&SetTexture)},
    {PDDI_SP_HCTOON , SHADE_TEXTURE(&SetTuneMap)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeIntTable d3dHKToonShader::intTable[] = 
{
    PDDID3D_STDSHADERPARAM_INT,
    {PDDI_SP_NULL , NULL}
};

pddiShadeFloatTable d3dHKToonShader::floatTable[] = 
{
    PDDID3D_STDSHADERPARAM_FLOAT,
    {PDDI_SP_NULL , NULL}
};


d3dHKToonShader::d3dHKToonShader(d3dContext* c) : 
    d3dShader(c),
    m_pTexture(NULL),
    m_pTuneMap(NULL)
{
#ifdef TOON_USE_PS
	LPDIRECT3DDEVICE8 d3d = context->GetDisplay()->GetD3DDevice();
    d3d->CreatePixelShader( ((const DWORD*)hktoonshaderPixelShaderTokens),  &m_PixelShaderHandle);
#endif

}

d3dHKToonShader::~d3dHKToonShader()
{
    if(m_pTexture)
        m_pTexture->Release();
    if(m_pTuneMap)
        m_pTuneMap->Release();
#ifdef TOON_USE_PS
	//Set the pixel shader
    LPDIRECT3DDEVICE8 d3d = context->GetDisplay()->GetD3DDevice();
    d3d->DeletePixelShader(m_PixelShaderHandle);
#endif
}

const char* d3dHKToonShader::GetType(void)
{
   static char type [] = "toon";
   return type;
}

int  d3dHKToonShader::GetPasses()
{
   return 1;
}

void d3dHKToonShader::SetPass(int pass)
{
    // do all SetTexture stuff here.
    d3d->SetTextures(2, m_pTexture, m_pTuneMap);

#ifndef TOON_USE_PS
	d3d->SetTextureFactor( toonIntensityColor );
    d3d->SetTextureCombine( 0, D3DTOP_MODULATE, D3DTA_TFACTOR, D3DTA_TEXTURE );
	d3d->SetTextureCombineResult( 0, D3DTA_TEMP );

    d3d->SetTextureCombine( 1, D3DTOP_ADD, D3DTA_DIFFUSE, D3DTA_TEXTURE );
	d3d->SetTextureCombine( 2, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEMP );
#endif

    //Normal setup for sampling for the first texture
    SetupTextureSampling(0);

    //Hardcoded sampling for the next state
    d3d->SetTextureAddress(1, PDDI_UV_CLAMP);
    d3d->SetTextureFilter(1, PDDI_FILTER_NONE);

    SetupShading( );
    SetupAlphaBlend( );
}

static D3DCULL oldCull;

void d3dHKToonShader::PreRender(void)
{
   LPDIRECT3DDEVICE8 d3d = context->GetDisplay()->GetD3DDevice();
#ifdef TOON_USE_PS
   d3d->SetPixelShader(m_PixelShaderHandle);

   float constant1[ 4 ] = { 0.6f, 0.6f, 0.6f, 0.6f };
   float constant0[ 4 ] = { 0.7f, 0.7f, 0.7f, 0.7f };

   d3d->SetPixelShaderConstant( 0, constant0, 1 );
   d3d->SetPixelShaderConstant( 1, constant1, 1 );
#endif
   if(twoSided)
   {
      d3d->GetRenderState(D3DRS_CULLMODE, (DWORD*)&oldCull);
      d3d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
   }
}

void d3dHKToonShader::PostRender(void)
{
   LPDIRECT3DDEVICE8 d3dDevice = context->GetDisplay()->GetD3DDevice();
#ifdef TOON_USE_PS
   d3dDevice->SetPixelShader(NULL);
#endif
   if(twoSided)
   {
      d3dDevice->SetRenderState(D3DRS_CULLMODE, oldCull);

   }

	d3d->SetTextureCombineResult(0, D3DTA_DIFFUSE);
}

//-------------------------------------------------------------------
void d3dHKToonShader::SetTexture(pddiTexture* t)
{
   if(t)
   {
      t->AddRef();
   }

   if(m_pTexture)
   {
      m_pTexture->Release();
   }

   m_pTexture = (d3dTexture*)t;
}

void d3dHKToonShader::SetTuneMap(pddiTexture* t)
{
   if(t)
   {
      t->AddRef();
   }

   if(m_pTuneMap)
   {
      m_pTuneMap->Release();
   }

   m_pTuneMap = (d3dTexture*)t;
}
