//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SIMPLESHADER_HPP_
#define _SIMPLESHADER_HPP_

#include"..\pddi.hpp"
#include"shader.hpp"

class d3dContext;
class d3dTexture;

class d3dSimpleShader : public d3dShader
{
public:
    d3dSimpleShader(d3dContext* c);
    ~d3dSimpleShader();

    const char* GetType(void);
    int  GetPasses();

    void GetShaderInfo(d3dShaderInfo*);

protected:
    void SetPass(int pass = 0);

    static pddiShadeColourTable colourTable[];
    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
    static pddiShadeFloatTable floatTable[];

    pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
    pddiShadeIntTable*     GetIntTable(void)     { return intTable;}
    pddiShadeFloatTable*   GetFloatTable(void)   { return floatTable;}
    pddiShadeColourTable*  GetColourTable(void)  { return colourTable;}
//   virtual pddiShadeVectorTable*  GetVectorTable(void) { return NULL;}
//   virtual pddiShadeMatrixTable*  GetMatrixTable(void) { return NULL;}

    // texture
    void SetTexture(pddiTexture* texture);

    // Multi CBV functions
    void EnableMultiCBV(int enable);
    void SetMultiCBVBlendValue(float value);
    void SetMultiCBVBlendColour(pddiColour colour);
    void SetMultiCBVBlendMode(int mode);
    void SetMultiCBVBlendSetA(int setA);
    void SetMultiCBVBlendSetB(int setB);

    DWORD GetMultiCBVBlendArg(pddiMultiCBVBlendOperand arg) const;

    // Members.
    d3dTexture* texture;
    bool       useMultiCBV;         // Use multiple vertex colours.
    pddiColour multiCBVBlendValue;  // How much of each selcted colour.
    pddiColour multiCBVBlendColour; // A replacement to using one of the vertex colours.
    pddiMultiCBVBlendMode    multiCBVBlendMode; // How to combine the selected colours.
    DWORD multiCBVBlendSetA; // Either a vertex colour set or the blend colour.
    DWORD multiCBVBlendSetB; // Either a vertex colour set or the blend colour.
};

#endif
