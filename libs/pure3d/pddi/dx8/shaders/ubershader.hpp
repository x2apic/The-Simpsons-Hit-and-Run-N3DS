//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _UBERSHADER_HPP
#define _UBERSHADER_HPP

#include "..\pddi.hpp"
#include "texture.hpp"
#include"shader.hpp"

class d3dShaderStageData
{
public:
    enum Constants {
        NUM_STAGES = 4
    };

    d3dShaderStageData( );
    ~d3dShaderStageData();
    
    void SetTexture( int stage, pddiTexture *tex );
    void SetUVMap( int stage, int map );
    void SetUVMode( int stage, int mode );
    void SetFilterMode( int stage, int f );
    void SetColourOp( int stage, int op );
    void SetAlphaOp( int stage, int op );
    void SetConstant( int stage, int constant );
    void SetColourArg1( int stage, int arg1 );
    void SetColourArg2( int stage, int arg1 );
    void SetAlphaArg1( int stage, int arg1 );
    void SetAlphaArg2( int stage, int arg1 );
    void SetTransform( int stage, int trans );

    int stageNum;

    pddiTexture*   textures[NUM_STAGES];
    unsigned       uvMap[NUM_STAGES];
    pddiUVMode     uvMode[NUM_STAGES];
    pddiFilterMode filters[NUM_STAGES];

    //color blending and alpha blending
    int colourOp[NUM_STAGES];       //format: pddiBlendMode
    int alphaOp[NUM_STAGES];        //format: pddiBlendMode

    DWORD colourArg1[NUM_STAGES];
    DWORD colourArg2[NUM_STAGES];
    DWORD alphaArg1[NUM_STAGES];
    DWORD alphaArg2[NUM_STAGES];
    DWORD textureTransform[NUM_STAGES];

    //constants 
    int textureFactor[NUM_STAGES];

    //mask: indicate if factor or transform is on/off for a stage
    int mask[NUM_STAGES];
    
};

class d3dUberShader : public d3dShader
{
public:

    d3dUberShader( d3dContext *c );
    ~d3dUberShader( );

    //inherit from pddiBaseShader
    virtual bool SetInt( unsigned int param, int );
    virtual bool SetTexture( unsigned int param, pddiTexture *tex );

    virtual int GetPasses( ){ return 1; };

protected:
    d3dShaderStageData stageData;
    
    virtual void SetPass( int pass );

};

#endif