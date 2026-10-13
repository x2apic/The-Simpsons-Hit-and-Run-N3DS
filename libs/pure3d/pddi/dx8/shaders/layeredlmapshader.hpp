//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _LAYEREDLMAPSHADER_HPP_
#define _LAYEREDLMAPSHADER_HPP_

#include"..\pddi.hpp"
#include"layeredshader.hpp"

class d3dContext;
class d3dTexture;

class d3dLayeredLMapShader : public d3dLayeredShader
{
public:
    d3dLayeredLMapShader(d3dContext* c);
    ~d3dLayeredLMapShader();
    
     const char* GetType(void);
     int  GetPasses();

protected:
     d3dTexture*      lightMap;

     void SetPass(int pass = 0);


     pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}

     //texture
     void SetLightMap(pddiTexture* texture);

     static pddiShadeTextureTable textureTable[];
};

#endif
