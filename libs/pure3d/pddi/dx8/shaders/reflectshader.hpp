//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _REFLECTPSHADER_HPP_
#define _REFLECTPSHADER_HPP_

#include"..\pddi.hpp"
#include"shader.hpp"

class d3dContext;
class d3dTexture;

class d3dReflShader : public d3dShader
{
public:
     d3dReflShader(d3dContext* c);
     ~d3dReflShader();
    
     const char* GetType(void);
     int  GetPasses();

     void PreRender(void);
     void PostRender(void);

protected:
    //
    // methods
    //
    void SetPass(int pass = 0);

    // texture
    void SetTexture(pddiTexture* texture);
    void SetReflectionMap(pddiTexture* texture);
    void SetEnvBlend(pddiColour c);

    static pddiShadeColourTable colourTable[];
    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
    static pddiShadeFloatTable floatTable[];

    pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
    pddiShadeIntTable*     GetIntTable(void)     { return intTable;}
    pddiShadeFloatTable*   GetFloatTable(void)   { return floatTable;}
    pddiShadeColourTable*  GetColourTable(void)  { return colourTable;}


    // texturing
    d3dTexture*      texture;
    d3dTexture*      reflectionMap;
    pddiColour       envBlend;
};

#endif
