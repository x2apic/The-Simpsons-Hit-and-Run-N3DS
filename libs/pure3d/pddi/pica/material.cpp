#include <pddi/pica/pica.hpp>
#include <pddi/pica/material.hpp>
#include <pddi/pica/texture.hpp>
#include <pddi/pica/context.hpp>
#include <pddi/pica/statecache.hpp>
#include <pddi/pica/display.hpp>

#include <pddi/base/debug.hpp>
#include <stdio.h>

pddiShadeColourTable picaMat::colourTable[] =
{
    {PDDI_SP_AMBIENT  , SHADE_COLOUR(&picaMat::SetAmbient)},
    {PDDI_SP_DIFFUSE  , SHADE_COLOUR(&picaMat::SetDiffuse)},
    {PDDI_SP_EMISSIVE , SHADE_COLOUR(&picaMat::SetEmissive)},
    {PDDI_SP_SPECULAR , SHADE_COLOUR(&picaMat::SetSpecular)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeTextureTable picaMat::textureTable[] =
{
    {PDDI_SP_BASETEX , SHADE_TEXTURE(&picaMat::SetTexture)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeIntTable picaMat::intTable[] =
{
    {PDDI_SP_UVMODE , SHADE_INT(&picaMat::SetUVMode)},
    {PDDI_SP_FILTER , SHADE_INT(&picaMat::SetFilterMode)},
    {PDDI_SP_SHADEMODE , SHADE_INT(&picaMat::SetShadeMode)},
    {PDDI_SP_ISLIT , SHADE_INT(&picaMat::EnableLighting)},
    {PDDI_SP_BLENDMODE , SHADE_INT(&picaMat::SetBlendMode)},
    {PDDI_SP_ALPHATEST , SHADE_INT(&picaMat::EnableAlphaTest)},
    {PDDI_SP_ALPHACOMPARE , SHADE_INT(&picaMat::SetAlphaCompare)},
    {PDDI_SP_TWOSIDED , SHADE_INT(&picaMat::SetTwoSided)},
    {PDDI_SP_EMISSIVEALPHA , SHADE_INT(&picaMat::SetEmissiveAlpha)},
    {PDDI_SP_COLOURWRITE , SHADE_INT(&picaMat::SetColourWrite)},
    {PDDI_SP_NULL , NULL}
};

pddiShadeFloatTable picaMat::floatTable[] =
{
    {PDDI_SP_SHININESS , SHADE_FLOAT(&picaMat::SetShininess)},
    {PDDI_SP_ALPHACOMPARE_THRESHOLD , SHADE_FLOAT(&picaMat::SetAlphaRef)},
    {PDDI_SP_NULL , NULL}
};

static const GPU_TEXTURE_FILTER_PARAM filterTable[5] =
{
    GPU_NEAREST,
    GPU_LINEAR,
    GPU_NEAREST,
    GPU_LINEAR,
    GPU_LINEAR
};

static const GPU_TEXTURE_WRAP_PARAM uvTable[3] =
{
    GPU_REPEAT,
    GPU_CLAMP_TO_EDGE,
    GPU_CLAMP_TO_EDGE
};

static const GPU_TESTFUNC alphaCompareTable[8] =
{
    GPU_NEVER,
    GPU_ALWAYS,
    GPU_LESS,
    GPU_LEQUAL,
    GPU_GREATER,
    GPU_GEQUAL,
    GPU_EQUAL,
    GPU_NOTEQUAL
};

static const GPU_BLENDEQUATION blendEquation[8] =
{
    GPU_BLEND_ADD,
    GPU_BLEND_ADD,
    GPU_BLEND_ADD,
    GPU_BLEND_REVERSE_SUBTRACT,
    GPU_BLEND_ADD,
    GPU_BLEND_ADD,
    GPU_BLEND_ADD,
    GPU_BLEND_REVERSE_SUBTRACT
};

static const GPU_BLENDFACTOR blendFactor[8][2] =
{
    { GPU_ONE,       GPU_ZERO },
    { GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA },
    { GPU_ONE,       GPU_ONE },
    { GPU_ONE,       GPU_ONE },
    { GPU_DST_COLOR, GPU_ZERO },
    { GPU_DST_COLOR, GPU_SRC_COLOR },
    { GPU_ONE,       GPU_SRC_ALPHA },
    { GPU_SRC_ALPHA, GPU_SRC_ALPHA }
};

picaMat::picaMat(picaContext* c)
{
    context = c;

    for(int i = 0; i < picaMaxPasses; i++)
    {
        texEnv[i].enabled    = false;
        texEnv[i].texture    = NULL;
        texEnv[i].uvSet      = i;
        texEnv[i].texGen     = PDDI_TEXGEN_NONE;
        texEnv[i].uvMode     = PDDI_UV_CLAMP;
        texEnv[i].filterMode = PDDI_FILTER_BILINEAR;

        texEnv[i].lit       = false;
        texEnv[i].twoSided  = false;
        texEnv[i].shadeMode = PDDI_SHADE_GOURAUD;
        texEnv[i].ambient.Set(255, 255, 255);
        texEnv[i].diffuse.Set(255, 255, 255);
        texEnv[i].specular.Set(0, 0, 0);
        texEnv[i].emissive.Set(0, 0, 0);
        texEnv[i].shininess = 0.0f;

        texEnv[i].alphaTest        = false;
        texEnv[i].alphaCompareMode = PDDI_COMPARE_GREATEREQUAL;
        texEnv[i].alphaBlendMode   = PDDI_BLEND_NONE;
        texEnv[i].alphaRef         = 0.5f;

        texEnv[i].writeMask = PDDI_WRITE_ALL;
    }
    texEnv[0].enabled = true;
    pass = 0;
    dirty = true;

    lastError = PDDI_OK;
}

picaMat::~picaMat()
{
    for(int i = 0; i < picaMaxPasses; i++)
        if(texEnv[i].texture)
            texEnv[i].texture->Release();
}

const char* picaMat::GetType(void)
{
    static char simple[] = "simple";
    return simple;
}

int picaMat::GetPasses(void)
{
    return 1;
}

void picaMat::SetPass(int p)
{
    SetDevPass(p);
}

void picaMat::SetTexture(pddiTexture* t)
{
    if(t == texEnv[pass].texture)
        return;

    if(texEnv[pass].texture)
        texEnv[pass].texture->Release();

    texEnv[pass].texture = (picaTexture*)t;

    if(texEnv[pass].texture)
        texEnv[pass].texture->AddRef();

    dirty = true;
}

void picaMat::SetUVMode(int mode)     { texEnv[pass].uvMode = (pddiUVMode)mode; dirty = true; }
void picaMat::SetFilterMode(int mode) { texEnv[pass].filterMode = (pddiFilterMode)mode; dirty = true; }
void picaMat::SetShadeMode(int shade) { texEnv[pass].shadeMode = (pddiShadeMode)shade; }
void picaMat::SetTwoSided(int b)      { texEnv[pass].twoSided = b != 0; dirty = true; }
void picaMat::EnableLighting(int b)   { texEnv[pass].lit = b != 0; }

void picaMat::SetAmbient(pddiColour a)       { texEnv[pass].ambient = a; }
void picaMat::SetDiffuse(pddiColour colour)  { texEnv[pass].diffuse = colour; }
void picaMat::SetSpecular(pddiColour c)      { texEnv[pass].specular = c; }

void picaMat::SetEmissive(pddiColour c)
{
    texEnv[pass].emissive = c;
    SetEmissiveAlpha(c.Alpha());
}

void picaMat::SetEmissiveAlpha(int alpha)
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

void picaMat::SetShininess(float power)  { texEnv[pass].shininess = power; }
void picaMat::SetBlendMode(int mode)     { texEnv[pass].alphaBlendMode = (pddiBlendMode)mode; dirty = true; }
void picaMat::EnableAlphaTest(int b)     { texEnv[pass].alphaTest = b != 0; dirty = true; }
void picaMat::SetAlphaCompare(int c)     { texEnv[pass].alphaCompareMode = pddiCompareMode(c); dirty = true; }
void picaMat::SetAlphaRef(float ref)     { texEnv[pass].alphaRef = ref; dirty = true; }
void picaMat::SetColourWrite(int mask)   { texEnv[pass].writeMask = mask; dirty = true; }

int picaMat::CountDevPasses(void)
{
    return 1;
}

void picaMat::SetDevPass(unsigned)
{
    const int i = 0;

    dirty = false;

    if(!g_picaInFrame)
    {
        dirty = true;
        return;
    }

    C3D_Tex* hwTex = texEnv[i].texture ? texEnv[i].texture->GetC3DTexture() : NULL;

    if(hwTex)
    {
        C3D_TexSetFilter(hwTex, filterTable[texEnv[i].filterMode],
                                filterTable[texEnv[i].filterMode]);
        C3D_TexSetWrap(hwTex, uvTable[texEnv[i].uvMode], uvTable[texEnv[i].uvMode]);
        picaTexBind0(hwTex);
    }

    const int env0Mode = hwTex ? 1 : 2;
    if(g_picaState.env0Mode != env0Mode)
    {
        C3D_TexEnv* env0 = C3D_GetTexEnv(0);
        C3D_TexEnvInit(env0);
        if(hwTex)
        {
            C3D_TexEnvSrc(env0, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env0, C3D_Both, GPU_MODULATE);
        }
        else
        {
            C3D_TexEnvSrc(env0, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
            C3D_TexEnvFunc(env0, C3D_Both, GPU_REPLACE);
        }
        g_picaState.env0Mode = env0Mode;
    }

    if(!g_picaState.passStagesSet)
    {
        for(int stage = 1; stage < 6; stage++)
        {
            C3D_TexEnv* env = C3D_GetTexEnv(stage);
            C3D_TexEnvInit(env);
            C3D_TexEnvSrc(env, C3D_Both, GPU_PREVIOUS, GPU_PREVIOUS, GPU_PREVIOUS);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
        }
        g_picaState.passStagesSet = true;
    }

    if(texEnv[i].alphaTest)
    {
        int ref = (int)(texEnv[i].alphaRef * 255.0f);
        if(ref < 0)   ref = 0;
        if(ref > 255) ref = 255;
        picaAlphaTest(true, alphaCompareTable[texEnv[i].alphaCompareMode], ref);
    }
    else
    {
        picaAlphaTest(false, GPU_ALWAYS, 0);
    }

    picaAlphaBlend(blendEquation[texEnv[i].alphaBlendMode],
                   blendEquation[texEnv[i].alphaBlendMode],
                   blendFactor[texEnv[i].alphaBlendMode][0],
                   blendFactor[texEnv[i].alphaBlendMode][1],
                   blendFactor[texEnv[i].alphaBlendMode][0],
                   blendFactor[texEnv[i].alphaBlendMode][1]);

    context->EnableCulling(!texEnv[i].twoSided);
    context->SetMaterialWriteMask(texEnv[i].writeMask);
}

void picaMat::PreRender(void)
{
    const int i = 0;

    if(dirty)
    {
        SetDevPass(0);
    }

    context->SetVertexUniforms(texEnv[i].lit,
                               texEnv[i].ambient,
                               texEnv[i].diffuse,
                               texEnv[i].emissive);
}
