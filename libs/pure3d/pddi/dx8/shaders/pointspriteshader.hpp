//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _POINTSPRITESHADER_HPP_
#define _POINTSPRITESHADER_HPP_

#include"..\pddi.hpp"
#include"shader.hpp"

class d3dContext;
class d3dTexture;

class d3dPointSpriteShader : public d3dShader
{
public:
    d3dPointSpriteShader(d3dContext* c);
    ~d3dPointSpriteShader();
    
    const char* GetType(void);
    int  GetPasses();

protected:
    //
    // methods
    //
    void SetPass(int pass = 0);
    void PreRender(void);
    void PostRender(void);

    // texture
    void SetTexture(pddiTexture* texture);
    void SetScaleEnable(int);
    void SetSize(float);
    void SetSizeMin(float);
    void SetSizeMax(float);
    void SetScaleA(float);
    void SetScaleB(float);
    void SetScaleC(float);
    void SetScaleOffset(float);

    pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
    pddiShadeIntTable* GetIntTable(void) { return intTable;}
    pddiShadeFloatTable* GetFloatTable(void) { return floatTable;}

    //
    // members
    //
    // texturing
    d3dTexture*      texture;

    // point sprite properties
    bool scale;
    float size, minSize, maxSize;
    float a,b,c;
    float scaleOffset;

    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
    static pddiShadeFloatTable floatTable[];
};

#endif
