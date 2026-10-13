#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "projtexshader2.hpp"
#include "direct3d.hpp"


pddiShadeColourTable d3dProjTextureShader2::colourTable[] = 
{
    PDDID3D_STDSHADERPARAM_COLOUR,
    { PDDI_SP_NULL,     NULL }
};

pddiShadeTextureTable d3dProjTextureShader2::textureTable[] = 
{
    { PDDI_SP_BASETEX, SHADE_TEXTURE(SetBaseTexture) },
    { PDDI_SP_PROJTEX, SHADE_TEXTURE(SetProjTexture) },
    { PDDI_SP_NULL,    NULL }
};

pddiShadeIntTable d3dProjTextureShader2::intTable[] = 
{
    PDDID3D_STDSHADERPARAM_INT,
    { PDDI_SP_NULL,         NULL }
};

pddiShadeFloatTable d3dProjTextureShader2::floatTable[] = 
{
    PDDID3D_STDSHADERPARAM_FLOAT,
    { PDDI_SP_NULL , NULL}
};

pddiShadeVectorTable d3dProjTextureShader2::vectorTable[ ] =
{
    { PDDI_SP_PROJVEC, SHADE_VECTOR(SetProjVector) },
    { PDDI_SP_NULL,         NULL }
};

pddiShadeMatrixTable d3dProjTextureShader2::matrixTable[] =
{
    { PDDI_SP_PROJMAT, SHADE_MATRIX(SetProjMatrix) },
    { PDDI_SP_NULL,         NULL }
};

d3dProjTextureShader2::d3dProjTextureShader2(d3dContext* c) :
    d3dShader(c)
{ 
    baseTex = NULL;
    projTex = NULL;
}

d3dProjTextureShader2::~d3dProjTextureShader2()
{
    if(baseTex)
        baseTex->Release();
    if(projTex)
        projTex->Release();
}


const char* d3dProjTextureShader2::GetType(void)
{
    static char type [] = "projtex2";
    return type;
}

int  d3dProjTextureShader2::GetPasses()
{
    return 1;
}


void d3dProjTextureShader2::SetPass(int pass)
{
    d3d->SetTextures(2, baseTex, projTex);

    d3d->SetTextureCombine(0, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEXTURE);
    d3d->SetTextureCoord(0, 0);
    d3d->SetTextureTransform(0, D3DTTFF_DISABLE);
    d3d->SetTextureAddress(0, PDDI_UV_TILE);
    d3d->SetTextureFilter(0, filterMode);

    d3d->SetTextureCombine(1, D3DTOP_MODULATE, D3DTA_CURRENT, D3DTA_TEXTURE);
    d3d->SetTextureAddress(1, PDDI_UV_CLAMP);
    d3d->SetTextureFilter(1, filterMode);

    SetupShading();
    SetupAlphaBlend();
}

//-------------------------------------------------------------------
void d3dProjTextureShader2::SetBaseTexture(pddiTexture* t)
{
    if(t)
        t->AddRef();

    if(baseTex)
        baseTex->Release();

    baseTex = (d3dTexture*)t;
}

//-------------------------------------------------------------------
void d3dProjTextureShader2::SetProjTexture(pddiTexture* t)
{
    if(t)
        t->AddRef();

    if(projTex)
        projTex->Release();

    projTex = (d3dTexture*)t;
}


//--
void d3dProjTextureShader2::SetProjVector( const pddiVector &vector )
{
    projVector.Set( vector.x, vector.y, vector.z );
}

//---
void d3dProjTextureShader2::SetProjMatrix( const pddiMatrix & matrix )
{
 
    for( int i = 0; i < 4; ++i )
        for( int j = 0; j < 4; ++j )
            projMatrix.m[ i ][ j ] = matrix.m[ i ][ j ];
}

void d3dProjTextureShader2::PreRender(void)
{
#ifdef _XBOX
    LPDIRECT3DDEVICE8 m_pd3dDevice = context->GetDisplay()->GetD3DDevice();

    // Some basic constants
    float vZero[4] = { 0.0f,0.0f,0.0f,0.f };
    
    // Set the vertex shader constants
    m_pd3dDevice->SetVertexShaderConstant(  0, &vZero,    1 );
    m_pd3dDevice->SetVertexShaderConstant(  1, &projVector,    1 );
    m_pd3dDevice->SetVertexShaderConstant(  2, &projMatrix, 4 );
#endif
}

void d3dProjTextureShader2::PostRender(void)
{
    // undo point sprite enabling
    LPDIRECT3DDEVICE8 d3d = context->GetDisplay()->GetD3DDevice();
    d3d->SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
    d3d->SetRenderState( D3DRS_POINTSCALEENABLE,  FALSE );
}