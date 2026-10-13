//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDISHADER_HPP_
#define _PDDISHADER_HPP_

#include "../pdditype.hpp"
#include "../base/baseshader.hpp"
#include "direct3d.hpp"
#include "texture.hpp"
#include "state.hpp"
#include "display.hpp"

class d3dShader : public pddiBaseShader
{
public:
    d3dShader(d3dContext* c); 

    virtual void GetShaderInfo(d3dShaderInfo*);

    void SetTexture(LPDIRECT3DDEVICE8 d3d, int stage, pddiTexture* texture)
    {
        if(texture)
        {
            ((d3dTexture*)texture)->SetTexture(stage);
        }
        else
        {
            d3d->SetTexture(stage, NULL);
        }
    }

    //
    //  Funcitons to setup parameters in default ways 
    //
    void SetupAlphaBlend(void)
    {
        d3d->SetAlphaBlend(blendMode);
        d3d->SetAlphaTest(alphaTest, alphaCompare);
        ((d3dDisplay*)context->GetDisplay())->GetD3DDevice()->SetRenderState(D3DRS_ALPHAREF, (DWORD)(alphaRef * 255));
    }

    void SetupShading(void)
    {      
        // set the shading model
        d3d->SetShadeMode(shadeMode);
        d3d->SetMaterial(isLit, twoSided, &shaderInfo);
    }

    void SetupTextureSampling(unsigned stage, unsigned ttf = D3DTTFF_DISABLE)
    {
        // set texture sampling modes
        // using coord set for stage, uv and filter moes are user-set
        d3d->SetTextureCoord(stage, stage);
        d3d->SetTextureTransform(stage, ttf);
        d3d->SetTextureAddress(stage, uvMode);
        d3d->SetTextureFilter(stage, filterMode);
    }

    //
    //  Set functions called by shader param dispatch
    //

    // alpha blend
    void SetBlendMode(int mode);
    void EnableAlphaTest(int enable);
    void SetAlphaCompare(int compare);
    void SetAlphaRef(float ref);

    // texture sampling
    void SetUVMode(int mode);
    void SetFilterMode(int mode);

    // shading
    void SetShadeMode(int shade);
    void SetTwoSided(int enable);
    void EnableLighting(int enable);
    void SetAmbient(pddiColour colour);
    void SetDiffuse(pddiColour colour);
    void SetEmissive(pddiColour);
    void SetEmissiveAlpha(int);
    void SetSpecular(pddiColour);
    void SetShininess(float power);

    void EnableSkinMode( bool b ){ skinMode = b; }
    bool InSkinMode( ){ return skinMode; }


    // Turns a normalized vector into RGBA form. Used to encode vectors into a height map
    static DWORD VectortoRGBA(pddiVector* v, FLOAT fHeight)
    {
         DWORD r = (DWORD)( 127.0f * v->x + 128.0f );
         DWORD g = (DWORD)( 127.0f * v->y + 128.0f );
         DWORD b = (DWORD)( 127.0f * v->z + 128.0f );
         DWORD a = (DWORD)( 255.0f * fHeight );
     
         return( (a<<24L) + (r<<16L) + (g<<8L) + (b<<0L) );
    }

protected:

    d3dContext* context;
    d3dState* d3d;

    // alpha blending
    pddiBlendMode blendMode;
    pddiBlendFactor srcBlend;
    pddiBlendFactor destBlend;
    bool alphaTest;
    pddiCompareMode alphaCompare;
    float alphaRef;

    // texturing
    pddiTextureGen textureGen;
    pddiUVMode uvMode;
    pddiFilterMode filterMode;

    // lighting
    bool isLit;
    pddiShadeMode shadeMode;
    d3dShaderInfo shaderInfo;

    bool twoSided;
    bool skinMode;
};

#define PDDID3D_STDSHADERPARAM_COLOUR \
    {PDDI_SP_AMBIENT  , SHADE_COLOUR(&SetAmbient)},\
    {PDDI_SP_DIFFUSE  , SHADE_COLOUR(&SetDiffuse)},\
    {PDDI_SP_EMISSIVE , SHADE_COLOUR(&SetEmissive)},\
    {PDDI_SP_SPECULAR , SHADE_COLOUR(&SetSpecular)}

#define PDDID3D_STDSHADERPARAM_INT \
    {PDDI_SP_UVMODE , SHADE_INT(&SetUVMode)},\
    {PDDI_SP_FILTER , SHADE_INT(&SetFilterMode)},\
    {PDDI_SP_SHADEMODE , SHADE_INT(&SetShadeMode)},\
    {PDDI_SP_ISLIT , SHADE_INT(&EnableLighting)},\
    {PDDI_SP_BLENDMODE , SHADE_INT(&SetBlendMode)},\
    {PDDI_SP_ALPHATEST , SHADE_INT(&EnableAlphaTest)},\
    {PDDI_SP_ALPHACOMPARE , SHADE_INT(&SetAlphaCompare)},\
    {PDDI_SP_TWOSIDED , SHADE_INT(&SetTwoSided)},\
    {PDDI_SP_EMISSIVEALPHA , SHADE_INT(&SetEmissiveAlpha)}

#define PDDID3D_STDSHADERPARAM_FLOAT \
    {PDDI_SP_SHININESS , SHADE_FLOAT(&SetShininess)},\
    {PDDI_SP_ALPHACOMPARE_THRESHOLD , SHADE_FLOAT(&SetAlphaRef)}

#endif


