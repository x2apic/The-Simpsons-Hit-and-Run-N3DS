//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef STATE_HPP_
#define STATE_HPP_

// Direct3D state abstraction 
#include "..\pddi.hpp"
#include "direct3d.hpp"
#include "context.hpp"
#include "util.hpp"
#include "texture.hpp"

struct d3dShaderInfo
{
    pddiColour diffuse;
    pddiColour specular;
    pddiColour ambient;
    pddiColour emissive;

    float shininess;
};

class d3dState
{
public:
    enum Constants {
        NUM_STAGES = 4
    };

    d3dState(d3dContext* c, LPDIRECT3DDEVICE8 device);
    ~d3dState();

    void SetDefault(void);
    void LoadAll(void);

    struct Filter
    {
        DWORD minFilter;
        DWORD magFilter;
        DWORD mipFilter;
    };

    struct Blend
    {
        bool enable;
        DWORD op;
        pddiBlendFactor src;
        pddiBlendFactor dest;
    };

    static Filter    filterTable[5];
    static Blend     alphaBlendTable[8];
    static D3DBLEND  blendFactorTable[11];
    static D3DTEXTUREADDRESS uvTable[2];
    static D3DCMPFUNC compareTable[8];
    static D3DSHADEMODE shadeTable[2];

    // convert float to DWORD as bit pattern (for a few weird texture stage states)
    static inline DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }

    void SetTextures(unsigned nStages, pddiTexture* tex0, pddiTexture* tex1 = 0, pddiTexture* tex2 = 0, pddiTexture* tex3 = 0)
    {
        SetTexture(0,tex0);
        SetTexture(1,tex1);
        SetTexture(2,tex2);
        SetTexture(3,tex3);

        if(colourOp[nStages] != D3DTOP_DISABLE)
        {
            colourOp[nStages] = D3DTOP_DISABLE;
            d3d->SetTextureStageState(nStages, D3DTSS_COLOROP,   colourOp[nStages]);
        }

        if(alphaOp[nStages] != D3DTOP_DISABLE)
        {
            alphaOp[nStages] = D3DTOP_DISABLE;
            d3d->SetTextureStageState(nStages, D3DTSS_ALPHAOP,   colourOp[nStages]);
        }
    }

    void SetTextureCoord(unsigned stage, unsigned uv)
    {
        if(uvMap[stage] != uv)
        {
            uvMap[stage] = uv;
            d3d->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, uv);
        }
    }

    void SetTextureAddress(unsigned stage, pddiUVMode mode)
    {
        if(uvMode[stage] != mode)
        {
            uvMode[stage] = mode;
            d3d->SetTextureStageState(stage, D3DTSS_ADDRESSU, uvTable[mode]);
            d3d->SetTextureStageState(stage, D3DTSS_ADDRESSV, uvTable[mode]);
            d3d->SetTextureStageState(stage, D3DTSS_ADDRESSW, uvTable[mode]);
        }
    }

    void SetTextureFilter(unsigned stage, pddiFilterMode mode) 
    { 
        if(filter[stage] != mode)
        {
            filter[stage] = mode;
            d3d->SetTextureStageState(stage, D3DTSS_MINFILTER, filterTable[mode].minFilter);
            d3d->SetTextureStageState(stage, D3DTSS_MAGFILTER, filterTable[mode].magFilter);
            d3d->SetTextureStageState(stage, D3DTSS_MIPFILTER, filterTable[mode].mipFilter);
        }
    }

    void SetPixelShader(unsigned p)
    {
        if(pixelShader != p)
        {
            pixelShader = p;
            d3d->SetPixelShader(pixelShader);
        }
    }

    void SetTextureColourCombine1(unsigned i, DWORD op, DWORD arg1)
    {
        if(colourOp[i] != op)
        {
            colourOp[i] = op;
            d3d->SetTextureStageState(i, D3DTSS_COLOROP,   colourOp[i]);
        }

        if(colourArg1[i] != arg1)
        {
            colourArg1[i] = arg1;
            d3d->SetTextureStageState(i, D3DTSS_COLORARG1,   colourArg1[i]);
        }
    }

    void SetTextureColourCombine(unsigned i, DWORD op, DWORD arg1, DWORD arg2)
    {
        SetTextureColourCombine1(i, op, arg1);

        if(colourArg2[i] != arg2)
        {
            colourArg2[i] = arg2;
            d3d->SetTextureStageState(i, D3DTSS_COLORARG2,   colourArg2[i]);
        }
    }

    // Set triadic texture colour blend operation.
    void SetTextureColourCombine3(unsigned i, DWORD op, DWORD arg1, DWORD arg2, DWORD arg3)
    {
        SetTextureColourCombine(i, op, arg1, arg2);

        if(colourArg3[i] != arg3)
        {
            colourArg3[i] = arg3;
            d3d->SetTextureStageState(i, D3DTSS_COLORARG0,   colourArg3[i]);
        }
    }

    void SetTextureAlphaCombine1(unsigned i, DWORD op, DWORD arg1)
    {
        if(alphaOp[i] != op)
        {
            alphaOp[i] = op;
            d3d->SetTextureStageState(i, D3DTSS_ALPHAOP,   alphaOp[i]);
        }

        if(alphaArg1[i] != arg1)
        {
            alphaArg1[i] = arg1;
            d3d->SetTextureStageState(i, D3DTSS_ALPHAARG1,   alphaArg1[i]);
        }
    }

    void SetTextureAlphaCombine(unsigned i, DWORD op, DWORD arg1, DWORD arg2)
    {
        SetTextureAlphaCombine1(i, op, arg1 );
        
        if(alphaArg2[i] != arg2)
        {
            alphaArg2[i] = arg2;
            d3d->SetTextureStageState(i, D3DTSS_ALPHAARG2,   alphaArg2[i]);
        }
    }

    // Set triadic texture alpha blend operation.
    void SetTextureAlphaCombine3(unsigned i, DWORD op, DWORD arg1, DWORD arg2, DWORD arg3)
    {
        SetTextureAlphaCombine(i, op, arg1, arg2);

        if(alphaArg3[i] != arg3)
        {
            alphaArg3[i] = arg3;
            d3d->SetTextureStageState(i, D3DTSS_ALPHAARG0,   alphaArg3[i]);
        }
    }

    void SetTextureCombine1(unsigned i, DWORD op, DWORD arg1)
    {
        SetTextureColourCombine1(i, op, arg1);
        SetTextureAlphaCombine1(i, op, arg1);
    }

    void SetTextureCombine(unsigned i, DWORD op, DWORD arg1, DWORD arg2)
    {
        SetTextureColourCombine(i, op, arg1, arg2);
        SetTextureAlphaCombine(i, op, arg1, arg2);
    }

    // Set triadic texture blend operation.
    void SetTextureCombine3(unsigned i, DWORD op, DWORD arg1, DWORD arg2, DWORD arg3)
    {
        SetTextureColourCombine3(i, op, arg1, arg2, arg3);
        SetTextureAlphaCombine3(i, op, arg1, arg2, arg3);
    }

    void SetTextureCombineResult(unsigned i, DWORD arg)
    {
        if(result[i] != arg)
        {
            result[i] = arg;
            d3d->SetTextureStageState(i, D3DTSS_RESULTARG, result[i]);
        }
    }

    void SetTextureTransform(unsigned i, DWORD op)
    {
        if(texTransform[i] != op)
        {
            texTransform[i] = op;
            d3d->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, texTransform[i]);
        }
    }

    void SetBumpEnvMat(unsigned i, float m00, float m01, float m10, float m11)
    {
        if(m00 != envMat[0])
        {
            envMat[0] = m00;
            d3d->SetTextureStageState(i, D3DTSS_BUMPENVMAT00, F2DW(m00));
        }

        if(m01 != envMat[1])
        {
            envMat[1] = m01;
            d3d->SetTextureStageState(i, D3DTSS_BUMPENVMAT01, F2DW(m01));
        }

        if(m10 != envMat[2])
        {
            envMat[2] = m10;
            d3d->SetTextureStageState(i, D3DTSS_BUMPENVMAT10, F2DW(m10));
        }

        if(m11 != envMat[3])
        {
            envMat[3] = m11;
            d3d->SetTextureStageState(i, D3DTSS_BUMPENVMAT11, F2DW(m11));
        }
    }

    void SetTextureFactor(DWORD factor)
    {
        if(factor != textureFactor)
        {
            textureFactor = factor;
            d3d->SetRenderState(D3DRS_TEXTUREFACTOR, textureFactor);
        }
    }

    void SetShadeMode(pddiShadeMode mode)
    {
        if(mode != shadeMode)
        {
            shadeMode = mode;
            d3d->SetRenderState(D3DRS_SHADEMODE, shadeTable[shadeMode]);
        }
    }

    void SetMaterial(bool lit, bool ts, d3dShaderInfo* info)
    {
        if((twoSided != ts) || (isLit != lit))
        {
            twoSided = ts;
            if(twoSided)
            {
                d3d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            }
            else
            {
                context->ResetCull();
            }
        }

        if(isLit && !lit)
        {
            isLit = false;
            d3d->SetRenderState(D3DRS_LIGHTING, FALSE);
            d3d->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
        }

        if(lit)
        {
            isLit = lit;
            d3d->SetRenderState(D3DRS_LIGHTING, TRUE);
            d3d->SetRenderState(D3DRS_SPECULARENABLE, (info->shininess > 0.0f));

            D3DMATERIAL8 material;
            d3dColourValue(info->diffuse, &material.Diffuse);
            d3dColourValue(info->ambient, &material.Ambient);
            d3dColourValue(info->specular, &material.Specular);
            d3dColourValue(info->emissive, &material.Emissive);
            material.Power = info->shininess;
            d3d->SetMaterial(&material);
        }
    }

    void SetAlphaBlend(bool enable, DWORD op, pddiBlendFactor src, pddiBlendFactor dest)
    {
        if(enable != alphaBlendEnable)
        {
            alphaBlendEnable = enable;
            d3d->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlendEnable);
        }

        if(!alphaBlendEnable)
            return;

        if(op != alphaBlendOp)
        {
            alphaBlendOp = op;
            d3d->SetRenderState(D3DRS_BLENDOP, alphaBlendOp);
        }

        if(src != alphaBlendSrc)
        {
            alphaBlendSrc = src;
            d3d->SetRenderState(D3DRS_SRCBLEND, blendFactorTable[alphaBlendSrc]);
        }

        if(dest != alphaBlendDest)
        {
            alphaBlendDest = dest;
            d3d->SetRenderState(D3DRS_DESTBLEND, blendFactorTable[alphaBlendDest]);
        }
    }

    void SetAlphaBlend(pddiBlendMode mode)
    {
        SetAlphaBlend(alphaBlendTable[mode].enable, alphaBlendTable[mode].op, alphaBlendTable[mode].src, alphaBlendTable[mode].dest);
    }

    void SetAlphaTest(bool on, pddiCompareMode mode = PDDI_COMPARE_GREATEREQUAL, unsigned ref = 0)
    {
        if(alphaTest != on)
        {
            alphaTest = on;
            d3d->SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);
        }

        if(alphaTest && (alphaCompare != mode))
        {
            alphaCompare = mode;
            d3d->SetRenderState(D3DRS_ALPHAFUNC, compareTable[alphaCompare]);
        }

        if(alphaTest && (alphaRef != ref))
        {
            alphaRef = ref;
            d3d->SetRenderState(D3DRS_ALPHAREF, alphaRef);
        }
    }

    void SetTexture(unsigned stage, pddiTexture* tex)
    {
        if(tex != textures[stage])
        {
            textures[stage] = tex;
            if(tex)
            {
                ((d3dTexture*)tex)->SetTexture(stage);
            }
            else
            {
                d3d->SetTexture(stage, NULL);
            }
        }
    }

    bool IsTwoSided(void) { return twoSided;}

protected:


    d3dContext* context;
    LPDIRECT3DDEVICE8 d3d;

    pddiTexture*   textures[NUM_STAGES];
    unsigned       uvMap[NUM_STAGES];
    pddiUVMode     uvMode[NUM_STAGES];
    pddiFilterMode filter[NUM_STAGES];

    pddiShadeMode  shadeMode;

    bool  alphaBlendEnable;
    DWORD alphaBlendOp;
    pddiBlendFactor alphaBlendSrc;
    pddiBlendFactor alphaBlendDest;
    bool alphaTest;
    pddiCompareMode alphaCompare;

    DWORD colourOp[NUM_STAGES + 1];
    DWORD alphaOp[NUM_STAGES + 1];
    DWORD colourArg1[NUM_STAGES];
    DWORD colourArg2[NUM_STAGES];
    DWORD colourArg3[NUM_STAGES];
    DWORD alphaArg1[NUM_STAGES];
    DWORD alphaArg2[NUM_STAGES];
    DWORD alphaArg3[NUM_STAGES];
    DWORD result[NUM_STAGES];

    DWORD texTransform[NUM_STAGES];

    bool isLit;
    bool twoSided;

    DWORD textureFactor;

    DWORD alphaRef;

    unsigned pixelShader;

    float envMat[4];
};

#endif
