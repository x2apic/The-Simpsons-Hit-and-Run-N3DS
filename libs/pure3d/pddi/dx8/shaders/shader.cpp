//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "shader.hpp"
#include "context.hpp"
#include "state.hpp"

d3dShader::d3dShader(d3dContext* c) : 
    context(c), // weak references to context and state, don't need to addref
    d3d(c->GetState()) 
{
    textureGen = PDDI_TEXGEN_NONE;
    uvMode = PDDI_UV_CLAMP;
    filterMode = PDDI_FILTER_BILINEAR;

    shadeMode = PDDI_SHADE_GOURAUD;
    isLit = false;
    shaderInfo.ambient.Set(255,255,255);
    shaderInfo.diffuse.Set(0,0,0);
    shaderInfo.specular.Set(0,0,0);
    shaderInfo.emissive.Set(0,0,0);
    shaderInfo.shininess = 0.0f;

    blendMode = PDDI_BLEND_NONE;
    srcBlend = PDDI_BF_ONE;
    destBlend = PDDI_BF_ZERO;
    alphaTest = false;
    alphaCompare = PDDI_COMPARE_GREATEREQUAL;
    alphaRef = 0.5f;
    
    twoSided = false;
    skinMode = false;
}

void d3dShader::GetShaderInfo(d3dShaderInfo* info)
{
    *info = shaderInfo;
}

void d3dShader::SetBlendMode(int mode)
{
    blendMode = (pddiBlendMode)mode;
}

void d3dShader::EnableAlphaTest(int enable)
{
    alphaTest = (enable != 0);
}

void d3dShader::SetAlphaCompare(int compare)
{
    alphaCompare = (pddiCompareMode)compare;
}

void d3dShader::SetAlphaRef(float ref)
{
    alphaRef = ref > 0.0f ? (ref < 1.0f ? ref : 1.0f) : 0.0f;
}

void d3dShader::SetUVMode(int mode)
{
    
    uvMode = (pddiUVMode)mode;
}

void d3dShader::SetFilterMode(int mode)
{
    
    filterMode = (pddiFilterMode)mode;
}

void d3dShader::SetShadeMode(int shade)
{
    
    shadeMode = (pddiShadeMode)shade;
}

void d3dShader::SetTwoSided(int ts)
{
    twoSided = ts != 0;
}

void d3dShader::EnableLighting(int enable)
{
    
    isLit = (enable != 0);
}

void d3dShader::SetAmbient(pddiColour colour)
{
    
    shaderInfo.ambient = colour;
}

void d3dShader::SetDiffuse(pddiColour colour)
{
    
    shaderInfo.diffuse = colour;
}

void d3dShader::SetSpecular(pddiColour colour)
{
    
    shaderInfo.specular = colour;
}


void d3dShader::SetEmissive(pddiColour colour)
{
    
    shaderInfo.emissive = colour;
    SetEmissiveAlpha(colour.Alpha());
}

void d3dShader::SetEmissiveAlpha(int alpha)
{
    shaderInfo.diffuse.SetAlpha(alpha);
    if(alpha < 255)
    {
        shaderInfo.specular.SetAlpha(0);
        shaderInfo.ambient.SetAlpha(0);
        shaderInfo.emissive.SetAlpha(0);
    }
    else
    {
        shaderInfo.specular.SetAlpha(255);
        shaderInfo.ambient.SetAlpha(255);
        shaderInfo.emissive.SetAlpha(255);
    }
}

void d3dShader::SetShininess(float power)
{
    
    shaderInfo.shininess = power;
}



