//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _LIGHTMAPSHADER_HPP_
#define _LIGHTMAPSHADER_HPP_

#include"..\pddi.hpp"
#include"shader.hpp"

class d3dContext;
class d3dTexture;

class d3dLightMapShader : public d3dShader
{
public:
    d3dLightMapShader(d3dContext* c);
    ~d3dLightMapShader();
    
     const char* GetType(void);
     int  GetPasses(void);

protected:
    //
    // methods
    //
    void SetPass(int pass = 0);

    // texture
    void SetTexture(pddiTexture* texture);
    void SetLightMap(pddiTexture* texture);
    
    pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
    pddiShadeIntTable* GetIntTable(void) {return intTable;}

    //
    // members
    //
    // texturing
    d3dTexture*      texture;
    d3dTexture*      lightMap;

    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
};

#endif
