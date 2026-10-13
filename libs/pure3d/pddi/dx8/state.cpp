//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "state.hpp"

d3dState::Filter d3dState::filterTable[5] =
{
    { D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_NONE },  // PDDI_TEXF_NONE
    { D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE },  // PDDI_TEXF_BILINEAR
    { D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_POINT }, // PDDI_TEXF_MIPMAP
    { D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_POINT }, // PDDI_TEXF_MIPMAP_BILINEAR
    { D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR } // PDDI_TEXF_MIPMAP_TRILINEAR
};

d3dState::Blend d3dState::alphaBlendTable[8] =
{
    { false, D3DBLENDOP_ADD,           PDDI_BF_ONE,      PDDI_BF_ZERO               }, // PDDI_BLEND_NONE
    { true,  D3DBLENDOP_ADD,           PDDI_BF_SRCALPHA, PDDI_BF_ONE_MINUS_SRCALPHA }, // PDDI_BLEND_ALPHA
    { true,  D3DBLENDOP_ADD,           PDDI_BF_ONE,      PDDI_BF_ONE                }, // PDDI_BLEND_ADD
    { true,  D3DBLENDOP_REVSUBTRACT,   PDDI_BF_ONE,      PDDI_BF_ONE                }, // PDDI_BLEND_SUBTRACT
    { true,  D3DBLENDOP_ADD,           PDDI_BF_DEST,     PDDI_BF_ZERO               }, // PDDI_BLEND_MODULATE
    { true,  D3DBLENDOP_ADD,           PDDI_BF_DEST,     PDDI_BF_SRC                }, // PDDI_BLEND_MODULATE2
    { true,  D3DBLENDOP_ADD,           PDDI_BF_DEST,     PDDI_BF_SRCALPHA           }, // PDDI_BLEND_ADDMODULATEALPHA
    { true,  D3DBLENDOP_REVSUBTRACT,   PDDI_BF_SRCALPHA, PDDI_BF_SRCALPHA           }  // PDDI_BLEND_SUBMODULATEALPHA
};

D3DBLEND d3dState::blendFactorTable[11] = 
{
    D3DBLEND_ZERO,
    D3DBLEND_ONE,
    D3DBLEND_SRCCOLOR,
    D3DBLEND_INVSRCCOLOR,
    D3DBLEND_DESTCOLOR,   
    D3DBLEND_INVDESTCOLOR,
    D3DBLEND_SRCALPHA,
    D3DBLEND_INVSRCALPHA,
    D3DBLEND_DESTALPHA,   
    D3DBLEND_INVDESTALPHA,
    D3DBLEND_SRCALPHASAT
};

D3DTEXTUREADDRESS d3dState::uvTable[2] =
{
    D3DTADDRESS_WRAP,
    D3DTADDRESS_CLAMP
};

D3DCMPFUNC d3dState::compareTable[8] =
{
        D3DCMP_NEVER,
        D3DCMP_ALWAYS,
        D3DCMP_LESS,
        D3DCMP_LESSEQUAL,
        D3DCMP_GREATER,
        D3DCMP_GREATEREQUAL,
        D3DCMP_EQUAL,
        D3DCMP_NOTEQUAL
};

D3DSHADEMODE d3dState::shadeTable[2] = 
{
        D3DSHADE_FLAT,
        D3DSHADE_GOURAUD
};

d3dState::d3dState(d3dContext* c, LPDIRECT3DDEVICE8 device)
{
    context = c;

    d3d = device;

    SetDefault();
}

d3dState::~d3dState()
{
}

void d3dState::SetDefault(void)
{
    unsigned i;
    for(i = 0; i < NUM_STAGES; i++)
    {
        textures[i] = NULL;
        filter[i] = PDDI_FILTER_NONE;
        uvMap[i] = i;
        uvMode[i] = PDDI_UV_TILE;

        colourOp[i] = alphaOp[i] = D3DTOP_DISABLE;
        colourArg1[i] = alphaArg1[i] = D3DTA_CURRENT;
        colourArg2[i] = alphaArg2[i] = D3DTA_TEXTURE;
        colourArg3[i] = alphaArg3[i] = D3DTA_CURRENT;

        result[i] = D3DTA_CURRENT;
        
        texTransform[i] = D3DTTFF_DISABLE;
    }
    colourOp[NUM_STAGES] = alphaOp[NUM_STAGES] = D3DTOP_DISABLE;

    shadeMode = PDDI_SHADE_GOURAUD;

    alphaBlendEnable = false;
    alphaBlendSrc = PDDI_BF_ONE;
    alphaBlendDest = PDDI_BF_ZERO;
    alphaTest = false;
    alphaCompare = PDDI_COMPARE_GREATEREQUAL;

    isLit = twoSided = false;

    textureFactor = 0;
    alphaRef = 0;

    pixelShader = true;

    envMat[0] = 1.0f;
    envMat[1] = 0.0f;
    envMat[2] = 0.0f;
    envMat[3] = 1.0f;


    LoadAll();
}

void d3dState::LoadAll(void) 
{
    unsigned i;

    for(i = 0; i < NUM_STAGES; i++)
    {
        if(textures[i])
        {
            ((d3dTexture*)textures[i])->SetTexture(i);
        }
        else
        {
            d3d->SetTexture(i, NULL);
        }

        d3d->SetTextureStageState(i, D3DTSS_MINFILTER, filterTable[filter[i]].minFilter);
        d3d->SetTextureStageState(i, D3DTSS_MAGFILTER, filterTable[filter[i]].magFilter);
        d3d->SetTextureStageState(i, D3DTSS_MIPFILTER, filterTable[filter[i]].mipFilter);
        d3d->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
        d3d->SetTextureStageState(i, D3DTSS_ADDRESSU, uvTable[uvMode[i]]);
        d3d->SetTextureStageState(i, D3DTSS_ADDRESSV, uvTable[uvMode[i]]);
        d3d->SetTextureStageState(i, D3DTSS_ADDRESSW, uvTable[uvMode[i]]);

        d3d->SetTextureStageState(i, D3DTSS_COLORARG1, colourArg1[i]);
        d3d->SetTextureStageState(i, D3DTSS_COLOROP,   colourOp[i]);
        d3d->SetTextureStageState(i, D3DTSS_COLORARG2, colourArg2[i]);
        d3d->SetTextureStageState(i, D3DTSS_ALPHAARG1, alphaArg1[i]);
        d3d->SetTextureStageState(i, D3DTSS_ALPHAOP,   alphaOp[i]);
        d3d->SetTextureStageState(i, D3DTSS_ALPHAARG2, alphaArg2[i]);

        d3d->SetTextureStageState(i, D3DTSS_COLORARG0, colourArg3[i]);
        d3d->SetTextureStageState(i, D3DTSS_ALPHAARG0, alphaArg3[i]);

        d3d->SetTextureStageState(i, D3DTSS_RESULTARG, result[i]);

        d3d->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, texTransform[i]);
    }
    d3d->SetTextureStageState(NUM_STAGES, D3DTSS_COLOROP,   colourOp[NUM_STAGES]);
    d3d->SetTextureStageState(NUM_STAGES, D3DTSS_ALPHAOP,   alphaOp[NUM_STAGES]);

    d3d->SetRenderState(D3DRS_LIGHTING, isLit);
    d3d->SetRenderState(D3DRS_SPECULARENABLE, isLit);
    d3d->SetRenderState(D3DRS_SHADEMODE, shadeTable[shadeMode]);

    d3d->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlendEnable);
    d3d->SetRenderState(D3DRS_SRCBLEND, blendFactorTable[alphaBlendSrc]);
    d3d->SetRenderState(D3DRS_DESTBLEND, blendFactorTable[alphaBlendDest]);
    d3d->SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);
    d3d->SetRenderState(D3DRS_ALPHAFUNC, compareTable[alphaCompare]);
    d3d->SetRenderState(D3DRS_TEXTUREFACTOR, textureFactor);
    d3d->SetRenderState(D3DRS_ALPHAREF, alphaRef);

    d3d->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE); 

    d3d->SetPixelShader(NULL); 

    d3d->SetTextureStageState(2, D3DTSS_BUMPENVMAT00, F2DW(envMat[0]));
    d3d->SetTextureStageState(2, D3DTSS_BUMPENVMAT01, F2DW(envMat[1]));
    d3d->SetTextureStageState(2, D3DTSS_BUMPENVMAT10, F2DW(envMat[2]));
    d3d->SetTextureStageState(2, D3DTSS_BUMPENVMAT11, F2DW(envMat[3]));
}

    /*
    float fBias = 2.0f;
    d3ddev->SetTextureStageState(0, D3DTSS_MIPMAPLODBIAS, *((LPDWORD) (&fBias)));
    */

