//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _TOONSHADER_HPP_
#define _TOONSHADER_HPP_

#include "..\pddi.hpp"
#include "shader.hpp"
#include "direct3d.hpp"

class d3dContext;
class d3dTexture;

class d3dToonShader : public d3dShader
{
public:
    d3dToonShader(d3dContext* c);
    ~d3dToonShader();
    const char* GetType(void);
    int  GetPasses();
    
    void PreRender(void);
    void PostRender(void);
protected:
    //
    // methods
    //
    void SetPass(int pass = 0);

    void GetCubeVector(int i, int cubesize, int x, int y, pddiVector* vector);
    void CreateCubeMap();
    void CreateNormalMap();
    
    pddiShadeIntTable*  GetIntTable(void) { return intTable;}
    pddiShadeColourTable*  GetColourTable(void)  { return colourTable;}

    void SetToonStyle(int);

    // texturing
    d3dTexture* cubeMap;     // this is used to do an edge-detection hack
    d3dTexture* normalMap;   // this is used to do perspective correction for sillhouettes
    int specularEnabled;
  
    // alpha blending
    pddiBlendMode blendMode;
    pddiBlendFactor srcBlend;
    pddiBlendFactor destBlend;
    bool alphaTest;
    pddiCompareMode alphaCompare;
    
    static pddiShadeIntTable   intTable[];
    static pddiShadeColourTable colourTable[];
    
    
    pddiMatrix matTrans;
    //hardware texture capabilities
    enum ToonStyle { TOON_DEFAULT, TOON_SILHOUETTE, TOON_DEPTH};
    ToonStyle m_style; //0 = default, 1 = silhouette, 2 = depth
};

#endif
