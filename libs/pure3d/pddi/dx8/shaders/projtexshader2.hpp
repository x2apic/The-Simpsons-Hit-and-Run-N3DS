#ifndef _PROJTEXTURESHADER2_HPP_
#define _PROJTEXTURESHADER2_HPP_

#include "..\pddi.hpp"
#include "shader.hpp"

class d3dContext;
class d3dTexture;

class d3dProjTextureShader2 : public d3dShader
{
public:
    d3dProjTextureShader2(d3dContext* c);
    ~d3dProjTextureShader2();

    const char* GetType(void);
    int  GetPasses();

    //project vector
    void SetProjVector( const pddiVector & vector );
    void SetProjMatrix( const pddiMatrix & matrix );

protected:
    static pddiShadeColourTable colourTable[];
    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
    static pddiShadeFloatTable floatTable[];
    static pddiShadeVectorTable vectorTable[ ];
    static pddiShadeMatrixTable matrixTable[ ];


    void SetPass(int pass = 0);

    pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
    pddiShadeIntTable*     GetIntTable(void)     { return intTable;}
    pddiShadeFloatTable*   GetFloatTable(void)   { return floatTable;}
    pddiShadeColourTable*  GetColourTable(void)  { return colourTable;}
    pddiShadeVectorTable*  GetVectorTable(void)  { return vectorTable;}
    pddiShadeMatrixTable*  GetMatrixTable(void)  { return matrixTable;}

    // texture
    void SetBaseTexture(pddiTexture* texture);
    void SetProjTexture(pddiTexture* texture);

    void PreRender( );
    void PostRender( );

    // texturing
    d3dTexture* baseTex;
    d3dTexture* projTex;

    pddiVector projVector;
    pddiMatrix projMatrix;

};

#endif

