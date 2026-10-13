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

pddiShadeTextureTable d3dLayeredShader::textureTable[] = 
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&SetTexture)},
    {PDDI_SP_TOPTEX , SHADE_TEXTURE(&SetTopTexture)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeIntTable d3dLayeredShader::intTable[] = 
{
    PDDID3D_STDSHADERPARAM_INT, 
    {PDDI_SP_TEXBLENDMODE , SHADE_INT(&SetTexBlendMode)},
    {PDDI_SP_TWOLAYERCBV, SHADE_INT(&EnableCBV2Layers)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeColourTable d3dLayeredShader::colourTable[] = 
{
    PDDID3D_STDSHADERPARAM_COLOUR, 
    {PDDI_SP_NULL , NULL}
};

pddiShadeFloatTable d3dLayeredShader::floatTable[] = 
{
    PDDID3D_STDSHADERPARAM_FLOAT, 
    {PDDI_SP_NULL , NULL}
};

extern bool g_videoCardLayerdShaderBusted;

//-----------------------------------------------------------------------------
//
// d3dLayeredShader::d3dLayeredShader
//
//-----------------------------------------------------------------------------
d3dLayeredShader::d3dLayeredShader(d3dContext* c) :
    d3dShader(c),
    texture(NULL),
    topText(NULL),
    texBlendMode(PDDI_BLEND_MODULATE),
    twoLayersCBV( false )
{
}

d3dLayeredShader::~d3dLayeredShader( void )
{
     if(texture)
     {
         texture->Release();
     }
 
     if(topText)
     {
         topText->Release();
     }
}
//-----------------------------------------------------------------------------
//
// d3dLayeredShader::GetType
//
//-----------------------------------------------------------------------------
const char* d3dLayeredShader::GetType(void)
{
    static char type [] = "layered";
    return type;
}

//-----------------------------------------------------------------------------
//
// d3dLayeredShader::GetPasses
//
//-----------------------------------------------------------------------------
int  d3dLayeredShader::GetPasses()
{    
    return 1;
}

//-----------------------------------------------------------------------------
void d3dLayeredShader::SetPass(int pass)
{
    
    // set base texture
    d3d->SetTextures(2, texture, topText);

    if( twoLayersCBV ){

        d3d->SetTextureCombine(0, D3DTOP_SELECTARG2, D3DTA_CURRENT, D3DTA_TEXTURE);
        SetupTextureSampling(0);

        d3d->SetTextureCombine(1, GetBlendOp( texBlendMode ), D3DTA_CURRENT, D3DTA_TEXTURE);
        SetupTextureSampling(1);

        d3d->SetTextureCombine(2, D3DTOP_MODULATE, D3DTA_DIFFUSE, D3DTA_CURRENT);
        SetupTextureSampling(2);

    }
    else{

        d3d->SetTextureCombine(0, D3DTOP_MODULATE, D3DTA_DIFFUSE, D3DTA_TEXTURE);
        SetupTextureSampling(0);

        //Some cards such as the 8500 and 9000 have problems with the layered shader
        //for some crazy reason
        if(g_videoCardLayerdShaderBusted)
        {
            d3d->SetTextureCombine(1, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_TEXTURE);
        }
        else
        {
            d3d->SetTextureCombine(1, GetBlendOp( texBlendMode ), D3DTA_CURRENT, D3DTA_TEXTURE);
        }
        SetupTextureSampling(1);
    }
    SetupShading();
    SetupAlphaBlend();

    if( texBlendMode == PDDI_BLEND_SUBMODULATEALPHA ){

       d3d->SetTextureColourCombine(1, D3DTOP_SUBTRACT, D3DTA_CURRENT, D3DTA_TEXTURE);
       d3d->SetTextureAlphaCombine(1, D3DTOP_SELECTARG2, D3DTA_CURRENT, D3DTA_TEXTURE);

       d3d->SetAlphaBlend( true, D3DBLENDOP_ADD, PDDI_BF_SRCALPHA, PDDI_BF_ZERO );
    }
    else{
        SetupAlphaBlend( );
    }

}

//-----------------------------------------------------------------------------
//
// d3dLayeredShader::SetTexture
//
//-----------------------------------------------------------------------------
void d3dLayeredShader::SetTexture(pddiTexture* t)
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
// d3dLayeredShader::SetTopTexture
//
//-----------------------------------------------------------------------------
void d3dLayeredShader::SetTopTexture(pddiTexture* t)
{
    if(t)
    {
        t->AddRef();
    }

    if(topText)
    {
        topText->Release();
    }

    topText = (d3dTexture*)t;
}

void d3dLayeredShader::SetTexBlendMode(int mode)
{
    texBlendMode = (pddiBlendMode)mode;
}

int d3dLayeredShader::GetBlendOp( pddiBlendMode mode )
{
    switch( mode ){
          case PDDI_BLEND_NONE:            
                return D3DTOP_SELECTARG1;            
          break;

          case PDDI_BLEND_ALPHA:
                return D3DTOP_BLENDTEXTUREALPHA ;
          break;

          case PDDI_BLEND_MODULATE:
                return D3DTOP_MODULATE;
          break;

          case PDDI_BLEND_ADD:
                return D3DTOP_ADD;
          break;

          case PDDI_BLEND_SUBTRACT:
                return D3DTOP_SUBTRACT;
          break;

          case PDDI_BLEND_MODULATE2:
                return D3DTOP_MODULATE2X;
          break;

          case PDDI_BLEND_ADDMODULATEALPHA:
                return D3DTOP_MODULATEALPHA_ADDCOLOR;
          break;

          default:
                return D3DTOP_MODULATE;
          break;
    }

}

