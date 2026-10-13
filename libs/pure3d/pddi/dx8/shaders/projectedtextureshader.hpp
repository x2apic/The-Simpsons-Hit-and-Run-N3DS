//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PROJECTEDTEXTURESHADER_HPP_
#define _PROJECTEDTEXTURESHADER_HPP_

#include "..\pddi.hpp"
#include "shader.hpp"

class d3dContext;
class d3dTexture;

class d3dProjectedTextureShader : public d3dShader
{
public:
    d3dProjectedTextureShader(d3dContext* c);
    ~d3dProjectedTextureShader();

    const char* GetType(void);
    int  GetPasses();

protected:
    static pddiShadeColourTable colourTable[];
    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
    static pddiShadeFloatTable floatTable[];
    static pddiShadeMatrixTable matrixTable[];

    void SetPass(int pass = 0);
    void PreRender(void);
    void PostRender(void);

    pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
    pddiShadeIntTable*     GetIntTable(void)     { return intTable;}
    pddiShadeFloatTable*   GetFloatTable(void)   { return floatTable;}
    pddiShadeColourTable*  GetColourTable(void)  { return colourTable;}
    pddiShadeMatrixTable*  GetMatrixTable(void)  { return matrixTable;}

    // texture
    void SetBaseTexture(pddiTexture* texture);
    void SetProjTexture(pddiTexture* texture);
    // projector's matrix (precombine camera to projector space matrix with perspective projection)
    void SetProjMatrix(const pddiMatrix& matrix); 

    // texturing
    d3dTexture* baseTex;
    d3dTexture* projTex;

    // projector's matrix (precombine camera to projector space matrix with perspective projection)
    pddiMatrix projMat;
};

#endif
