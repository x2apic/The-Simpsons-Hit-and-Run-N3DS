//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _LAYEREDSHADER_HPP_
#define _LAYEREDSHADER_HPP_

#include"..\pddi.hpp"
#include"shader.hpp"

class d3dContext;
class d3dTexture;

class d3dLayeredShader : public d3dShader
{
public:
    d3dLayeredShader(d3dContext* c);
    ~d3dLayeredShader();
    
     const char* GetType(void);
     int  GetPasses();

protected:
    //
    // methods
    //
    void SetPass(int pass = 0);

    // texture
    void SetTexture(pddiTexture* texture);
    void SetTopTexture(pddiTexture* texture);
    void SetTexBlendMode( int mode );

    //apply cbv to both layer or not
    void EnableCBV2Layers( int flag ){ twoLayersCBV = ( flag != 0 ); }

    pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
    pddiShadeIntTable*     GetIntTable(void)     { return intTable;}
    pddiShadeFloatTable*   GetFloatTable(void)   { return floatTable;}
    pddiShadeColourTable*  GetColourTable(void)  { return colourTable;}

    //
    // members
    //
    // texturing
    d3dTexture*      texture;
    d3dTexture*      topText;
    pddiBlendMode    texBlendMode;
    bool             twoLayersCBV;

    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
    static pddiShadeColourTable colourTable[ ];
    static pddiShadeFloatTable  floatTable[ ];

    int GetBlendOp( pddiBlendMode mode );
};

#endif
