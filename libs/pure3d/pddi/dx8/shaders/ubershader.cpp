//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "direct3d.hpp"
#include "state.hpp"
#include "ubershader.hpp"

const unsigned UBER_MASK_CONSTANT       = 0x0001;
const unsigned UBER_MASK_TRANSFORM      = 0x0002;

///
///     stage data for uber shader
///
d3dShaderStageData::d3dShaderStageData( )
: stageNum( 0 )
{
    for( int i = 0; i < d3dShaderStageData::NUM_STAGES; ++i ){
        textures[ i ] = NULL;

        uvMap[ i ] = i;
        uvMode[ i ] = PDDI_UV_TILE;
        filters[ i ] = PDDI_FILTER_NONE;
        colourOp[ i ] = PDDI_BLEND_MODULATE;
        alphaOp[ i ] = PDDI_BLEND_MODULATE;

        //default value of texture argument is D3DTA_CURRENT op D3DTA_TEXTURE
        colourArg1[ i ] = alphaArg1[ i ] = D3DTA_CURRENT;
        colourArg2[ i ] = alphaArg2[ i ] = D3DTA_TEXTURE;

        textureTransform[ i ] = D3DTTFF_DISABLE;
        textureFactor[ i ] = 0;
    }
    
}

d3dShaderStageData::~d3dShaderStageData( )
{
    for( int i = 0; i < stageNum; ++i ){
        if( textures[ i ] )
            textures[ i ]->Release( );
    }
}

void d3dShaderStageData::SetTexture( int stage, pddiTexture *tex )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    textures[ stage ] = tex;
}

void d3dShaderStageData::SetAlphaOp( int stage, int op )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    alphaOp[ stage ] = op;
}

void d3dShaderStageData::SetColourOp( int stage, int op )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    colourOp[ stage ] = op;
}

void d3dShaderStageData::SetUVMap( int stage, int map )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    uvMap[ stage ] = map;
}

void d3dShaderStageData::SetUVMode( int stage, int mode )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    uvMode[ stage ] = (pddiUVMode) mode;
}

void d3dShaderStageData::SetFilterMode( int stage, int f )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    filters[ stage ] = ( pddiFilterMode )f;
}

void d3dShaderStageData::SetConstant( int stage, int c )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    textureFactor[ stage ] = c;
    mask[ stage ] |= UBER_MASK_TRANSFORM;
}

void d3dShaderStageData::SetColourArg1( int stage, int arg1 )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    colourArg1[ stage ] = arg1;
}

void d3dShaderStageData::SetColourArg2( int stage, int arg2 )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    colourArg2[ stage ] = arg2;
}


void d3dShaderStageData::SetAlphaArg1( int stage, int arg1 )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    alphaArg1[ stage ] = arg1;
}

void d3dShaderStageData::SetAlphaArg2( int stage, int arg2 )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    alphaArg2[ stage ] = arg2;
}

void d3dShaderStageData::SetTransform( int stage, int trans )
{
    PDDIASSERT( stage >= 0 && stage < NUM_STAGES );
    textureTransform[ stage ] = trans;
    mask[ stage ] |= UBER_MASK_TRANSFORM;
}


///
/// uber shader
///
d3dUberShader::d3dUberShader(d3dContext* c) :
    d3dShader(c)
{ 
}

d3dUberShader::~d3dUberShader()
{

}

bool d3dUberShader::SetTexture(unsigned param, pddiTexture* tex)
{
    if( param >= PDDI_SP_TEX1 && param <= PDDI_SP_TEX8 ){
        stageData.SetTexture( param - PDDI_SP_TEX1, tex );
        return true;
    }
    else
        return pddiBaseShader::SetTexture( param, tex );

}

bool d3dUberShader::SetInt( unsigned param, int i )
{
    
    bool result = true;
    if( param >= PDDI_SP_TEXTUREOP1 && param <= PDDI_SP_TEXTUREOP8 ){        //texture op
        stageData.SetColourOp( param - PDDI_SP_TEXTUREOP1, i );
        stageData.SetAlphaOp( param - PDDI_SP_TEXTUREOP1, i );
    }
    else if( param >= PDDI_SP_COLOUROP1 && param <= PDDI_SP_COLOUROP8 )        //colour op
        stageData.SetColourOp( param - PDDI_SP_COLOUROP1,  i );
    else if( param >= PDDI_SP_ALPHAOP1 && param <= PDDI_SP_ALPHAOP8 )     //alpha op
        stageData.SetAlphaOp( param - PDDI_SP_ALPHAOP1, i );
    else if( param >= PDDI_SP_UVMODE1 && param <= PDDI_SP_UVMODE1 )     //UV addressing mode
        stageData.SetUVMode( param - PDDI_SP_UVMODE1, i );
    else if( param >= PDDI_SP_FILTER1 && param <= PDDI_SP_FILTER8 )     //texture filer mode
        stageData.SetFilterMode( param - PDDI_SP_FILTER1, i );
    else if( param >= PDDI_SP_CONSTANT1 && param <= PDDI_SP_CONSTANT8 ) //contants
        stageData.SetConstant( param - PDDI_SP_CONSTANT1, i );
    else if( param == PDDI_SP_STAGENUM )
        stageData.stageNum = i;
    else  //otherwise, we call the supper class's SetInt to dispatch parameter
        result = pddiBaseShader::SetInt( param, i );

    return result;
}

void d3dUberShader::SetPass(int pass)
{

    for( int i = 0; i < stageData.stageNum; ++i ){
        d3d->SetTexture( i, stageData.textures[ i ] );

        //test if texture factor is used for this stage
        if( stageData.mask[ i ] & UBER_MASK_CONSTANT )
            d3d->SetTextureFactor( stageData.textureFactor[ i ] );

        //uv channel
        d3d->SetTextureCoord( i, stageData.uvMap[ i ] );

        //test if texture UV transform is used for this stage
        if( stageData.mask[ i ] & UBER_MASK_TRANSFORM )
            d3d->SetTextureTransform( i, stageData.textureTransform[ i ] );

        //addressing mode
        d3d->SetTextureAddress( i, stageData.uvMode[ i ] );

        //texture filter
        d3d->SetTextureFilter( i, stageData.filters[ i ] );

        //blending of colour and alpha
        d3d->SetTextureColourCombine( i, stageData.colourOp[ i ], stageData.colourArg1[ i ], stageData.colourArg2[ i ] );
        d3d->SetTextureAlphaCombine( i, stageData.alphaOp[ i ], stageData.alphaArg1[ i ], stageData.alphaArg2[ i ] );
    }

    SetupShading();
    SetupAlphaBlend();
}
