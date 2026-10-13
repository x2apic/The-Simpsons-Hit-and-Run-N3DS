//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <pddi/gxm/gxm.hpp>
#include <pddi/gxm/material.hpp>
#include <pddi/gxm/texture.hpp>
#include <pddi/gxm/context.hpp>
#include <pddi/gxm/display.hpp>
#include <pddi/gxm/program.hpp>

#include <vector>
#include <microprofile.h>
#include <SDL.h>

pddiShadeColourTable gxmMat::colourTable[] = 
{
    {PDDI_SP_AMBIENT  , SHADE_COLOUR(&gxmMat::SetAmbient)},
    {PDDI_SP_DIFFUSE  , SHADE_COLOUR(&gxmMat::SetDiffuse)},
    {PDDI_SP_EMISSIVE , SHADE_COLOUR(&gxmMat::SetEmissive)},
    {PDDI_SP_SPECULAR , SHADE_COLOUR(&gxmMat::SetSpecular)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeTextureTable gxmMat::textureTable[] = 
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&gxmMat::SetTexture)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeIntTable gxmMat::intTable[] = 
{
    {PDDI_SP_UVMODE , SHADE_INT(&gxmMat::SetUVMode)},
    {PDDI_SP_FILTER , SHADE_INT(&gxmMat::SetFilterMode)},
    {PDDI_SP_SHADEMODE , SHADE_INT(&gxmMat::SetShadeMode)},
    {PDDI_SP_ISLIT , SHADE_INT(&gxmMat::EnableLighting)},
    {PDDI_SP_BLENDMODE , SHADE_INT(&gxmMat::SetBlendMode)},
    {PDDI_SP_ALPHATEST , SHADE_INT(&gxmMat::EnableAlphaTest)},
    {PDDI_SP_ALPHACOMPARE , SHADE_INT(&gxmMat::SetAlphaCompare)},
    {PDDI_SP_TWOSIDED , SHADE_INT(&gxmMat::SetTwoSided)},
    {PDDI_SP_EMISSIVEALPHA , SHADE_INT(&gxmMat::SetEmissiveAlpha)},
    {PDDI_SP_COLOURWRITE , SHADE_INT(&gxmMat::SetColourWrite)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeFloatTable gxmMat::floatTable[] = 
{
    {PDDI_SP_SHININESS , SHADE_FLOAT(&gxmMat::SetShininess)},
    {PDDI_SP_ALPHACOMPARE_THRESHOLD , SHADE_FLOAT(&gxmMat::SetAlphaRef)},
    {PDDI_SP_NULL , NULL}
};

SceGxmTextureFilter filterMagTable[5] =
{
    SCE_GXM_TEXTURE_FILTER_POINT,
    SCE_GXM_TEXTURE_FILTER_LINEAR,
    SCE_GXM_TEXTURE_FILTER_POINT,
    SCE_GXM_TEXTURE_FILTER_LINEAR,
    SCE_GXM_TEXTURE_FILTER_LINEAR
};

// GL_NEAREST_MIPMAP_LINEAR not used
SceGxmTextureFilter filterMinTable[5] =
{
    SCE_GXM_TEXTURE_FILTER_POINT,
    SCE_GXM_TEXTURE_FILTER_LINEAR,
    SCE_GXM_TEXTURE_FILTER_POINT,//GL_NEAREST_MIPMAP_NEAREST,
    SCE_GXM_TEXTURE_FILTER_LINEAR,//GL_LINEAR_MIPMAP_NEAREST,
    SCE_GXM_TEXTURE_FILTER_LINEAR,//GL_LINEAR_MIPMAP_LINEAR
};

SceGxmTextureAddrMode uvTable[3] =
{
    SCE_GXM_TEXTURE_ADDR_REPEAT,
    SCE_GXM_TEXTURE_ADDR_CLAMP,
    SCE_GXM_TEXTURE_ADDR_CLAMP
};

SceGxmBlendFunc alphaBlendFunc[8] =
{
    { SCE_GXM_BLEND_FUNC_NONE },            //PDDI_BLEND_NONE,
    { SCE_GXM_BLEND_FUNC_ADD },             //PDDI_BLEND_ALPHA,
    { SCE_GXM_BLEND_FUNC_ADD },             //PDDI_BLEND_ADD,
    { SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT },//PDDI_BLEND_SUBTRACT,
    { SCE_GXM_BLEND_FUNC_ADD },             //PDDI_BLEND_MODULATE,
    { SCE_GXM_BLEND_FUNC_ADD },             //PDDI_BLEND_MODULATE2,
    { SCE_GXM_BLEND_FUNC_ADD },             //PDDI_BLEND_ADDMODULATEALPHA,
    { SCE_GXM_BLEND_FUNC_REVERSE_SUBTRACT } //PDDI_BLEND_SUBMODULATEALPHA
};

SceGxmBlendFactor alphaBlendTable[8][2] =
{
    { SCE_GXM_BLEND_FACTOR_ONE,       SCE_GXM_BLEND_FACTOR_ZERO },                //PDDI_BLEND_NONE,
    { SCE_GXM_BLEND_FACTOR_SRC_ALPHA, SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA }, //PDDI_BLEND_ALPHA,
    { SCE_GXM_BLEND_FACTOR_ONE,       SCE_GXM_BLEND_FACTOR_ONE },                 //PDDI_BLEND_ADD,
    { SCE_GXM_BLEND_FACTOR_ONE,       SCE_GXM_BLEND_FACTOR_ONE },                 //PDDI_BLEND_SUBTRACT,
    { SCE_GXM_BLEND_FACTOR_DST_COLOR, SCE_GXM_BLEND_FACTOR_ZERO },                //PDDI_BLEND_MODULATE,
    { SCE_GXM_BLEND_FACTOR_DST_COLOR, SCE_GXM_BLEND_FACTOR_SRC_COLOR },           //PDDI_BLEND_MODULATE2,
    { SCE_GXM_BLEND_FACTOR_ONE,       SCE_GXM_BLEND_FACTOR_SRC_ALPHA },           //PDDI_BLEND_ADDMODULATEALPHA,
    { SCE_GXM_BLEND_FACTOR_SRC_ALPHA, SCE_GXM_BLEND_FACTOR_SRC_ALPHA }            //PDDI_BLEND_SUBMODULATEALPHA
};

gxmMat::gxmMat(gxmContext* c) 
{
    context = c;
    valid = false;
    program = nullptr;
    shader = nullptr;

    for(int i = 0; i < gxmMaxPasses; i++)
    {
        texEnv[i].enabled = false;
        texEnv[i].texture = NULL;
        texEnv[i].uvSet = i;
        texEnv[i].texGen = PDDI_TEXGEN_NONE;
        texEnv[i].uvMode = PDDI_UV_CLAMP;
        texEnv[i].filterMode = PDDI_FILTER_BILINEAR;

        texEnv[i].lit = false;
        texEnv[i].twoSided = false;
        texEnv[i].shadeMode = PDDI_SHADE_GOURAUD;
        texEnv[i].ambient.Set(255,255,255);
        texEnv[i].diffuse.Set(255,255,255);
        texEnv[i].specular.Set(0,0,0);
        texEnv[i].emissive.Set(0,0,0);
        texEnv[i].shininess = 0.0f;

    //   srcBlend = PDDI_BF_ONE;
    //   destBlend = PDDI_BF_ZERO;

        texEnv[i].alphaTest = false;
        texEnv[i].alphaCompareMode = PDDI_COMPARE_GREATEREQUAL;
        texEnv[i].alphaBlendMode = PDDI_BLEND_NONE;
        texEnv[i].alphaRef = 0.5f;

        texEnv[i].writeMask = PDDI_WRITE_ALL;
    }
    texEnv[0].enabled = true;
    pass = 0;
}

gxmMat::~gxmMat() 
{
    for(int i = 0; i < gxmMaxPasses; i++)
        if(texEnv[i].texture)
            texEnv[i].texture->Release();

    if(program)
        program->Release();
}


const char* gxmMat::GetType(void)
{
    static char simple[] = "simple";
    return simple;
}

int gxmMat::GetPasses(void)
{
    return 1;
}

void gxmMat::SetPass(int pass)
{
    SetDevPass(pass);
}

void gxmMat::SetTexture(pddiTexture* t) 
{
    if(t == texEnv[pass].texture)
        return;

    if((texEnv[pass].texture != nullptr) != (t != nullptr))
        valid = false;

    if(texEnv[pass].texture)
        texEnv[pass].texture->Release();

    texEnv[pass].texture = (gxmTexture*)t;

    if(texEnv[pass].texture)
        texEnv[pass].texture->AddRef();
}

void gxmMat::SetUVMode(int mode) 
{
    texEnv[pass].uvMode = (pddiUVMode)mode;
}

void gxmMat::SetFilterMode(int mode) 
{
    texEnv[pass].filterMode = (pddiFilterMode)mode;
}

void gxmMat::SetShadeMode(int shade) 
{
    texEnv[pass].shadeMode = (pddiShadeMode)shade;
}

void gxmMat::SetTwoSided(int b)
{
    texEnv[pass].twoSided = b != 0;
}

void gxmMat::EnableLighting(int b)
{
    texEnv[pass].lit = b != 0;
}

void gxmMat::SetAmbient(pddiColour a) 
{
    texEnv[pass].ambient = a;
}

void gxmMat::SetDiffuse(pddiColour colour) 
{
    texEnv[pass].diffuse = colour;
}

void gxmMat::SetSpecular(pddiColour c) 
{
    texEnv[pass].specular = c;
}

void gxmMat::SetEmissive(pddiColour c) 
{
    texEnv[pass].emissive = c;
    SetEmissiveAlpha(c.Alpha());
}

void gxmMat::SetEmissiveAlpha(int alpha)
{
    texEnv[pass].diffuse.SetAlpha(alpha);
    if(alpha < 255)
    {
        texEnv[pass].specular.SetAlpha(0);
        texEnv[pass].ambient.SetAlpha(0);
        texEnv[pass].emissive.SetAlpha(0);
    }
    else
    {
        texEnv[pass].specular.SetAlpha(255);
        texEnv[pass].ambient.SetAlpha(255);
        texEnv[pass].emissive.SetAlpha(255);
    }
}

void gxmMat::SetShininess(float power) 
{
    texEnv[pass].shininess = power;
}

void gxmMat::SetBlendMode(int mode) 
{
    valid = valid && texEnv[pass].alphaBlendMode == (pddiBlendMode)mode;
    texEnv[pass].alphaBlendMode = (pddiBlendMode)mode;
}

void gxmMat::EnableAlphaTest(int b) 
{
    valid = valid && texEnv[pass].alphaTest == b != 0;
    texEnv[pass].alphaTest = b != 0;
}

void gxmMat::SetAlphaCompare(int compare) 
{
    texEnv[pass].alphaCompareMode = pddiCompareMode(compare);
}

void gxmMat::SetAlphaRef(float ref) 
{
    texEnv[pass].alphaRef = ref;
}

void gxmMat::SetColourWrite(int mask)
{
    valid = valid && texEnv[pass].writeMask == mask;
    texEnv[pass].writeMask = mask;
}

int gxmMat::CountDevPasses(void) 
{
    return 1;
}

void gxmMat::SetDevPass(unsigned pass)
{
    int i = 0;
    SceGxmContext* gxm = context->GetDisplay()->GetGXMContext();

    if(!valid)
    {
        if(program)
            program->Release();
        program = context->GetFragmentProgram(&texEnv[i]);
        program->AddRef();

        if(texEnv[i].alphaBlendMode == PDDI_BLEND_NONE && texEnv[i].writeMask == PDDI_WRITE_ALL)
        {
            shader = program->PatchFragmentShader(nullptr, context->GetDisplay()->GetMSAAMode());
        }
        else
        {
            SceGxmBlendInfo blend = {
                (uint8_t)texEnv[i].writeMask,
                alphaBlendFunc[texEnv[i].alphaBlendMode],
                alphaBlendFunc[texEnv[i].alphaBlendMode],
                alphaBlendTable[texEnv[i].alphaBlendMode][0],
                alphaBlendTable[texEnv[i].alphaBlendMode][1],
                alphaBlendTable[texEnv[i].alphaBlendMode][0],
                alphaBlendTable[texEnv[i].alphaBlendMode][1]
            };
            shader = program->PatchFragmentShader(&blend, context->GetDisplay()->GetMSAAMode());
        }
        valid = true;
    }

    sceGxmSetFragmentProgram(gxm, shader);

    if(texEnv[i].texture)
    {
        SceGxmTexture* texture = texEnv[i].texture->GetGXMTexture();

        CHK_GXM(sceGxmTextureSetMagFilter(texture, filterMagTable[texEnv[i].filterMode]));
        CHK_GXM(sceGxmTextureSetMinFilter(texture, filterMinTable[texEnv[i].filterMode]));
        CHK_GXM(sceGxmTextureSetUAddrMode(texture, uvTable[texEnv[i].uvMode]));
        CHK_GXM(sceGxmTextureSetVAddrMode(texture, uvTable[texEnv[i].uvMode]));
        CHK_GXM(sceGxmSetFragmentTexture(gxm, 0, texture));
    }

    context->EnableCulling(!texEnv[i].twoSided);
}

void gxmMat::PreRender()
{
    int i = 0;
    if(program)
        context->SetTextureEnvironment(&texEnv[i], program);
}
