//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "simpleshader.hpp"
#include "direct3d.hpp"
#include "state.hpp"

//-------------------------------------------------------------------

pddiShadeColourTable d3dSimpleShader::colourTable[] = 
{
    PDDID3D_STDSHADERPARAM_COLOUR,
    {PDDI_SP_CBV_BLEND_COLOUR, SHADE_COLOUR(&SetMultiCBVBlendColour)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeTextureTable d3dSimpleShader::textureTable[] = 
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&SetTexture)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeIntTable d3dSimpleShader::intTable[] = 
{
    PDDID3D_STDSHADERPARAM_INT, 
    {PDDI_SP_MULTI_CBV, SHADE_INT(&EnableMultiCBV)},
    {PDDI_SP_CBV_BLEND_MODE, SHADE_INT(&SetMultiCBVBlendMode)},
    {PDDI_SP_CBV_BLEND_SET_A, SHADE_INT(&SetMultiCBVBlendSetA)},
    {PDDI_SP_CBV_BLEND_SET_B, SHADE_INT(&SetMultiCBVBlendSetB)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeFloatTable d3dSimpleShader::floatTable[] = 
{
    PDDID3D_STDSHADERPARAM_FLOAT, 
    {PDDI_SP_CBV_BLEND_VALUE, SHADE_FLOAT(&SetMultiCBVBlendValue)},
    {PDDI_SP_NULL , NULL}
};


d3dSimpleShader::d3dSimpleShader(d3dContext* c) :
    d3dShader(c), texture(NULL),
    useMultiCBV(false), multiCBVBlendValue(0x00ffffff), multiCBVBlendColour(0xffffffff),
    multiCBVBlendMode(PDDI_MULTI_CBV_BLEND_MODULATE),
    multiCBVBlendSetA(D3DTA_DIFFUSE),
    multiCBVBlendSetB(D3DTA_SPECULAR)
{ /**/ }

d3dSimpleShader::~d3dSimpleShader()
{
    if(texture)
        texture->Release();
}


const char* d3dSimpleShader::GetType(void)
{
    static char type [] = "simple";
    return type;
}

int  d3dSimpleShader::GetPasses()
{
    return 1;
}

void d3dSimpleShader::GetShaderInfo(d3dShaderInfo* info)
{
    *info = shaderInfo;
}

void d3dSimpleShader::SetPass(int pass)
{
    // Set the textures
    // one stage, one texture
    if(useMultiCBV)
    {
        int modulateTextureStage = 0;
        
        switch(multiCBVBlendMode)
        {
            case PDDI_MULTI_CBV_BLEND_NONE:  // texture * SetA
            {   
                d3d->SetTextures(1, texture);
                if(multiCBVBlendSetA == D3DTA_TFACTOR) d3d->SetTextureFactor(multiCBVBlendColour);

                d3d->SetTextureCombine(0, D3DTOP_MODULATE, D3DTA_TEXTURE, multiCBVBlendSetA);
                SetupTextureSampling(0);
            }
            break;
            case PDDI_MULTI_CBV_BLEND_ADD:       // SetA + f * SetB
            case PDDI_MULTI_CBV_BLEND_SUBTRACT:  // SetA - f * SetB
            {   
                d3d->SetTextures(3, NULL, NULL, texture);
                d3d->SetTextureFactor(multiCBVBlendValue);
               
                d3d->SetTextureCombine(0, D3DTOP_MODULATE, multiCBVBlendSetB, D3DTA_TFACTOR|D3DTA_ALPHAREPLICATE);
                SetupTextureSampling(0);
                
                if(multiCBVBlendMode == PDDI_MULTI_CBV_BLEND_ADD)
                {
                    d3d->SetTextureCombine(1, D3DTOP_ADD, multiCBVBlendSetA, D3DTA_CURRENT);
                }
                else
                {
                    d3d->SetTextureCombine(1, D3DTOP_SUBTRACT, multiCBVBlendSetA, D3DTA_CURRENT);
                }
                SetupTextureSampling(1);
                
                modulateTextureStage = 2;
            }
            break;
            case PDDI_MULTI_CBV_BLEND_MODULATE:  // SetA * ( (1-f) * 1 + f * SetB
            {
                d3d->SetTextures(3, NULL, NULL, texture);
                d3d->SetTextureFactor(multiCBVBlendValue);

                d3d->SetTextureCombine(0, D3DTOP_MODULATE, multiCBVBlendSetA, multiCBVBlendSetB);
                SetupTextureSampling(0);
                
                d3d->SetTextureCombine(1, D3DTOP_BLENDFACTORALPHA, D3DTA_CURRENT, multiCBVBlendSetA);
                SetupTextureSampling(1);

                modulateTextureStage = 2;
            }
            break;
            case PDDI_MULTI_CBV_BLEND_INTERPOLATE:  // (1-f) * SetA + f * SetB
            default:
            {
                d3d->SetTextures(2, NULL, texture);
                d3d->SetTextureFactor(multiCBVBlendValue);
                
                d3d->SetTextureCombine(0, D3DTOP_BLENDFACTORALPHA, multiCBVBlendSetB, multiCBVBlendSetA);
                SetupTextureSampling(0);
                
                modulateTextureStage = 1;
            }
            break;
        }
        if(modulateTextureStage > 0)  // Common last stage, modulate result with texture.
        {
            d3d->SetTextureColourCombine(modulateTextureStage, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT);
            if((multiCBVBlendSetA == D3DTA_TFACTOR) || (multiCBVBlendSetB == D3DTA_TFACTOR))
            {
                // Blend alpha is invalid so just use texture alpha.
                d3d->SetTextureAlphaCombine1(modulateTextureStage, D3DTOP_SELECTARG1, D3DTA_TEXTURE); 
            }
            else
            {
                d3d->SetTextureAlphaCombine(modulateTextureStage, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT);
            }
            SetupTextureSampling(modulateTextureStage);
            d3d->SetTextureCoord(modulateTextureStage, 0); // Set last stage to use UV set 0.
        }
    }
    else
    {
        d3d->SetTextures(1, texture);
        
        // Set up texture combine
        // hard coded to modulation (including alpha)
        d3d->SetTextureCombine(0, D3DTOP_MODULATE, D3DTA_DIFFUSE, D3DTA_TEXTURE);
        SetupTextureSampling(0);
    }
    
    // setup default stuff
    SetupShading();
    SetupAlphaBlend();
}


//-------------------------------------------------------------------
void d3dSimpleShader::SetTexture(pddiTexture* t)
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

//----------------------------------------------------------------------
// Multi CBV functions
//--------------------

void d3dSimpleShader::EnableMultiCBV(int enable)
{
    useMultiCBV = (enable!=0);
}

void d3dSimpleShader::SetMultiCBVBlendValue(float value)
{
    int alpha = rmt::Clamp( (int)(255.0f * value), 0, 255);
    multiCBVBlendValue.SetAlpha(alpha);
}

void d3dSimpleShader::SetMultiCBVBlendColour(pddiColour colour)
{
    multiCBVBlendColour = colour;

    // Also add the colour to the blend value as required by hacks to perform
    // multi cbv blends without a vertex shader.
    multiCBVBlendValue.SetRed(colour.Red());
    multiCBVBlendValue.SetGreen(colour.Green());
    multiCBVBlendValue.SetBlue(colour.Blue());
}

void d3dSimpleShader::SetMultiCBVBlendMode(int mode)
{
    multiCBVBlendMode = (pddiMultiCBVBlendMode)mode;
}

void d3dSimpleShader::SetMultiCBVBlendSetA(int setA)
{
    multiCBVBlendSetA = GetMultiCBVBlendArg((pddiMultiCBVBlendOperand)setA);
}

void d3dSimpleShader::SetMultiCBVBlendSetB(int setB)
{
    multiCBVBlendSetB = GetMultiCBVBlendArg((pddiMultiCBVBlendOperand)setB);
}

// Return the D3D register holding a given operand.
DWORD d3dSimpleShader::GetMultiCBVBlendArg(pddiMultiCBVBlendOperand arg) const
{
    switch(arg)
    {
        case PDDI_MULTI_CBV_SET2:
        {
            return D3DTA_SPECULAR;
        }
        break;
        
        case PDDI_MULTI_CBV_BLEND_COLOUR:
        {
            return D3DTA_TFACTOR;
        }
        break;

        case PDDI_MULTI_CBV_SET1:
        default:
        {
            return D3DTA_DIFFUSE;
        }
        break;
    }

    return D3DTA_DIFFUSE;
}

