//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


/*
D3D Toon Shader
Jan 31, 01        rogrady@radical.ca

The idea here is taken from S. Dietrich's paper (Toons and Texturing) at http://www.nvidia.com
The technique is as follows: construct a cube map which normalizes XYZ vectors and converts them into RGBA colors
We then have a texture containing 3D positions, essentially.  We likewise have a 'normal' map, which represents
all the normal vectors about the origin, with <0, 0, -1> being at the origin, and <-1/sqrt(3), -1/sqrt(3), -1/sqrt(3) >
being at the top-left corner, and the vectors inbetween are interpolated.  These are also converted to RGBA, so that 
they can be stored in a texture.
This is an approximation to the view vector.  Then we can take the dotproduct of the
'view' with the 'position' of the object, which is essentially a backface culling strategy.  This colors our object
in grayscale, with black edges.  The whiter the object, the more front-facing it is.  An object is black if its dotproduct
is very low: the angle between the face normal and the view vector is 90 degrees. (cos (90) = 0).
But from this stage, all we want are the black edges- the outline.
So how can we get rid of the non-black pixels?  
The way to do this is to realize that the dotproduct operation fills the alpha channel with the result as well as the color
channels.  So, a black color will correspond to a very low alpha value.  Therefore, we can use alpha testing to test away
all pixels with alpha (and color) values greater than some low threshold.
That is the end of the first pass.

In the second pass, we render the object in a flat colour and blend it with the results of the previous stage.  This gives
us a two-tone look which is very cartoony.


Things to add:  -support for separate specifiable shadow color (requires additional pass, i think)
                     -option for depth impression (don't alpha test, do additive blend with grayscale)
*/
const float TOON_CUBE_MAP_RESOLUTION=256.0f;


#include "context.hpp"
#include "display.hpp"
#include "texture.hpp"
#include "shaderdata.hpp"
#include "shader.hpp"
#include "toonshader.hpp"
#include "direct3d.hpp"

pddiShadeIntTable d3dToonShader::intTable[] =
{
    PDDID3D_STDSHADERPARAM_INT,
    {PDDI_SP_ALPHATEST , SHADE_INT(&EnableAlphaTest)},
    {PDDI_SP_ALPHACOMPARE , SHADE_INT(&SetAlphaCompare)},
    {PDDI_SP_TOONSTYLE, SHADE_INT(&SetToonStyle)},
    {PDDI_SP_NULL, NULL}
};

pddiShadeColourTable d3dToonShader::colourTable[] = 
{
    {PDDI_SP_DIFFUSE  , SHADE_COLOUR(&SetDiffuse)},
    {PDDI_SP_NULL , NULL}
};

//-----------------------------------------------------------------------------
//
// d3dToonShader::d3dToonShader
//
//-----------------------------------------------------------------------------
d3dToonShader::d3dToonShader(d3dContext* c) :
    d3dShader(c),
    cubeMap(NULL),
    normalMap(NULL)
{
    //test for device caps
    LPDIRECT3DDEVICE8 d3d = context->GetDisplay()->GetD3DDevice();

    // check we have the stuff we need
    D3DCAPS8 pCaps;
    d3d->GetDeviceCaps(&pCaps);
    PDDIASSERT((pCaps.TextureOpCaps & D3DTEXOPCAPS_DOTPRODUCT3) && ( pCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP ));

    //set up texture projection matrix - provides perspective-independent xforms for normalMap (view vectors)
    matTrans.m[0][0] = 0.5f;
    matTrans.m[0][1] = matTrans.m[0][2] = matTrans.m[0][3] = 0.0f;
    matTrans.m[1][1] = 0.5f;
    matTrans.m[1][0] = matTrans.m[1][2] = matTrans.m[1][3] = 0.0f;
    matTrans.m[2][0] = 0.5f;
    matTrans.m[2][1] = 0.5f;
    matTrans.m[2][2] = 1.0f;
    matTrans.m[2][3] = 0.0f;
    matTrans.m[3][0] = matTrans.m[3][1] = matTrans.m[3][2]  = matTrans.m[3][3]  = 0.0f;

    m_style = TOON_DEFAULT;
    CreateNormalMap();
    CreateCubeMap();
}

d3dToonShader::~d3dToonShader()
{
    if (cubeMap)
        cubeMap->Release();
    if (normalMap)
        normalMap->Release();
}

//-----------------------------------------------------------------------------
//
// d3dToonShader::GetType
//
//-----------------------------------------------------------------------------
const char* d3dToonShader::GetType(void)
{
    static char type [] = "toon";
    return type;
}

//-----------------------------------------------------------------------------
//
// d3dToonShader::GetPasses
//
//-----------------------------------------------------------------------------
int  d3dToonShader::GetPasses()
{
    return (m_style == TOON_SILHOUETTE ? 1 : 2);    //a silhouette only takes 1 pass , depth & normal take 2
}

void d3dToonShader::PreRender(void)
{
    context->PushMatrix(PDDI_MATRIX_TEXTURE1);
}

void d3dToonShader::PostRender(void)
{
    context->PopMatrix(PDDI_MATRIX_TEXTURE1);
}

void d3dToonShader::SetPass(int pass)
{
    DWORD alphaTestVal = 0x00000060;
    if (m_style == TOON_DEFAULT)
        alphaTestVal = 0x00000080;

    ToonStyle tmp = m_style;

    // second pass of DEFAULT is same as SILHOUETTE
    if((pass == 1) && (m_style == TOON_DEFAULT))
    {
        tmp = TOON_SILHOUETTE;
    }

    switch(tmp)
    {
        case TOON_DEFAULT:
            {
                D3DCOLOR color = D3DCOLOR_ARGB(shaderInfo.diffuse.Alpha(), shaderInfo.diffuse.Red(), shaderInfo.diffuse.Green(), shaderInfo.diffuse.Blue());

                //render object with specified color
                d3d->SetTextures(1, NULL);
                d3d->SetTextureFactor(color);

                d3d->SetTextureCombine(0, D3DTOP_SELECTARG1, D3DTA_TFACTOR, D3DTA_TEXTURE);
                SetupTextureSampling(0);

                d3d->SetAlphaTest(false);
                d3d->SetAlphaBlend(PDDI_BLEND_NONE);
                d3d->SetShadeMode(PDDI_SHADE_FLAT);
            }
            break;
        case TOON_SILHOUETTE:
            {
                d3d->SetTextures(2, cubeMap, normalMap);

                d3d->SetTextureCombine(0, D3DTOP_SELECTARG2, D3DTA_CURRENT, D3DTA_TEXTURE);
                d3d->SetTextureCoord(0, D3DTSS_TCI_CAMERASPACENORMAL);
                d3d->SetTextureTransform(0, D3DTTFF_COUNT3);
                d3d->SetTextureAddress(0, uvMode);
                d3d->SetTextureFilter(0, filterMode);

                d3d->SetTextureCombine(1, D3DTOP_DOTPRODUCT3, D3DTA_CURRENT, D3DTA_TEXTURE);
                d3d->SetTextureCoord(1, D3DTSS_TCI_CAMERASPACEPOSITION);
                d3d->SetTextureTransform(1, D3DTTFF_COUNT2);
                d3d->SetTextureAddress(1, uvMode);
                d3d->SetTextureFilter(1, filterMode);
                context->LoadMatrix(PDDI_MATRIX_TEXTURE1, &matTrans);

                d3d->SetAlphaTest(true, PDDI_COMPARE_LESSEQUAL, alphaTestVal);
                d3d->SetAlphaBlend(true, D3DBLENDOP_ADD, PDDI_BF_ZERO, PDDI_BF_ZERO); //we don't draw anything but black
                d3d->SetMaterial(false, twoSided, NULL);
            }
            break;
        case TOON_DEPTH:
            {
                if (pass == 0)
                {
                    d3d->SetTextures(2, cubeMap, normalMap);

                    d3d->SetTextureCombine(0, D3DTOP_SELECTARG2, D3DTA_CURRENT, D3DTA_TEXTURE);
                    d3d->SetTextureCoord(0, D3DTSS_TCI_CAMERASPACENORMAL);
                    d3d->SetTextureTransform(0, D3DTTFF_COUNT3);
                    d3d->SetTextureAddress(0, uvMode);
                    d3d->SetTextureFilter(0, filterMode);

                    d3d->SetTextureCombine(1, D3DTOP_DOTPRODUCT3, D3DTA_CURRENT, D3DTA_TEXTURE);
                    d3d->SetTextureCoord(1, D3DTSS_TCI_CAMERASPACEPOSITION);
                    d3d->SetTextureTransform(1, D3DTTFF_COUNT2);
                    d3d->SetTextureAddress(1, uvMode);
                    d3d->SetTextureFilter(1, filterMode);
                    context->LoadMatrix(PDDI_MATRIX_TEXTURE1, &matTrans);
                    
                    d3d->SetAlphaTest(false);
                    d3d->SetAlphaBlend(true, D3DBLENDOP_ADD, PDDI_BF_ONE, PDDI_BF_ZERO); //we don't draw anything but black
                    d3d->SetMaterial(false, twoSided, NULL);
                }  //end pass 0
                else if (pass == 1)
                {
                    D3DCOLOR color = D3DCOLOR_ARGB(shaderInfo.diffuse.Alpha(), shaderInfo.diffuse.Red(), shaderInfo.diffuse.Green(), shaderInfo.diffuse.Blue());

                        //render object with specified color
                    d3d->SetTextures(1, NULL);
                    d3d->SetTextureFactor(color);
                    d3d->SetTextureCombine(0, D3DTOP_SELECTARG1, D3DTA_TFACTOR, D3DTA_TEXTURE);

                    SetupTextureSampling(0);
                    d3d->SetAlphaTest(false);
                    d3d->SetAlphaBlend(true, D3DBLENDOP_ADD, PDDI_BF_SRC, PDDI_BF_DEST);
                    d3d->SetShadeMode(PDDI_SHADE_FLAT);
                }
            }
            break;
    }

}  //end SetPass


void d3dToonShader::GetCubeVector(int i, int cubesize, int x, int y, pddiVector* vector)
{
  float s, t, sc, tc, mag;

  s = ((float)x + 0.5f) / (float)cubesize;
  t = ((float)y + 0.5f) / (float)cubesize;
  sc = s*2.0f - 1.0f;
  tc = t*2.0f - 1.0f;

  switch (i) {
  case 0:
     vector->x = 1.0f;
     vector->y = -tc;
     vector->z = -sc;
     break;
  case 1:
     vector->x = -1.0f;
     vector->y = -tc;
     vector->z = sc;
     break;
  case 2:
     vector->x = sc;
     vector->y = 1.0f;
     vector->z = tc;
     break;
  case 3:
     vector->x = sc;
     vector->y = -1.0f;
     vector->z = -tc;
     break;
  case 4:
     vector->x = sc;
     vector->y = -tc;
     vector->z = 1.0f;
     break;
  case 5:
     vector->x = -sc;
     vector->y = -tc;
     vector->z = -1.0f;
     break;
  }

  mag = 1.0f/(float)sqrt(vector->x*vector->x + vector->y*vector->y + vector->z*vector->z);
  vector->x *= mag;
  vector->y *= mag;
  vector->z *= mag;
}


void d3dToonShader::CreateNormalMap()
{
    D3DLOCKED_RECT  d3dlr;
    D3DSURFACE_DESC descBack;
    HRESULT d3dok;
    int i,j;
    
    LPDIRECT3DTEXTURE8 d3dNormalMap;       //this is used to do perspective correction for sillhouettes

    if (normalMap)
        normalMap->Release();
    
    LPDIRECT3DDEVICE8 d3d = context->GetDisplay()->GetD3DDevice();
    
    // get format of back buffer
    IDirect3DSurface8* pBackBuffer = NULL;
    d3dok = d3d->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    if (d3dok != D3D_OK)
        return;
    pBackBuffer->GetDesc(&descBack);
    pBackBuffer->Release();
    
    //create the normalMap- 128x128 grid of interpolated normals, with origin at <0,0,-1>.

    d3dok = d3d->CreateTexture((unsigned int)TOON_CUBE_MAP_RESOLUTION, (unsigned int) TOON_CUBE_MAP_RESOLUTION, 1, NULL, descBack.Format, D3DPOOL_MANAGED, &d3dNormalMap);
    d3dok = d3dNormalMap->LockRect(0, &d3dlr, NULL, NULL);
    PDDIASSERT(d3dok == D3D_OK);
    DWORD* pPixel = (DWORD*)d3dlr.pBits;
    PDDIASSERT(pPixel);
    pddiVector tempVect;
    const float sqrt3 = 1.0f/(float)sqrt(3);
    
    for( j = 0; j < TOON_CUBE_MAP_RESOLUTION; j++  )
    {
        tempVect.y = -sqrt3 + 2*(float)j/TOON_CUBE_MAP_RESOLUTION*sqrt3;
        for( i = 0; i < TOON_CUBE_MAP_RESOLUTION; i++ )
        {
            tempVect.x = -sqrt3 + 2*(float)i/TOON_CUBE_MAP_RESOLUTION*sqrt3;
            tempVect.z = -(float)sqrt(1- tempVect.x*tempVect.x - tempVect.y*tempVect.y);
            tempVect.Normalize();
            *pPixel++ = VectortoRGBA( &tempVect, 0.5f);
        }
    }
    
    d3dNormalMap->UnlockRect(0);
    
    normalMap = new d3dTexture(context, d3dNormalMap);
}


//CreateCubeMap
//this lets us transform an XYZ vertex coord into an RGB triple, so that we can
//use the hardware DOTPRODUCT3 instruction
//Calls GetCubeVector() which takes an index (s,t,u coords, essentially)
//and transforms them into an XYZ vector
void d3dToonShader::CreateCubeMap()
{
    LPDIRECT3DCUBETEXTURE8 d3dCubeMap;     //this is used to do an edge-detection hack
    if (cubeMap)
        cubeMap->Release();

    int i,j,k;
    D3DLOCKED_RECT  d3dlr;
    D3DSURFACE_DESC descBack;
    HRESULT d3dok;

    LPDIRECT3DDEVICE8 d3d = context->GetDisplay()->GetD3DDevice();
    
    // get format of back buffer
    IDirect3DSurface8* pBackBuffer = NULL;
    
    d3dok = d3d->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    if (d3dok != D3D_OK)
        return;
    pBackBuffer->GetDesc(&descBack);
    pBackBuffer->Release();
    
    //need to allocate from managed rather than default pool so that we can lock it for writing
    d3dok = d3d->CreateCubeTexture((unsigned int)TOON_CUBE_MAP_RESOLUTION, 1, NULL, descBack.Format, D3DPOOL_MANAGED, &d3dCubeMap);
    PDDIASSERT(d3dok == D3D_OK);
    
    // For each world space coord from <0,0,0> to <1,1,1>,
    // generate a corresponding RGB vector
    //we assume all vectors passed in are normalized   
    for (k = 0; k < 6; k++)
    {
        d3dok = d3dCubeMap->LockRect(static_cast<D3DCUBEMAP_FACES>(k), 0, &d3dlr, NULL, NULL);
        PDDIASSERT(d3dok == D3D_OK);
        DWORD* pPixel = (DWORD*)d3dlr.pBits;
        PDDIASSERT(pPixel);
                
        pddiVector tempVect;
        for( j = 0; j < TOON_CUBE_MAP_RESOLUTION; j++  )
        {
            for( i = 0; i < TOON_CUBE_MAP_RESOLUTION; i++ )
            {
                GetCubeVector(k, (unsigned int)TOON_CUBE_MAP_RESOLUTION, i, j, &tempVect);
                *pPixel++ = VectortoRGBA( &tempVect, 0.5f);
            }
        }
        d3dCubeMap->UnlockRect(static_cast<D3DCUBEMAP_FACES>(k), 0);
    }

    cubeMap = new d3dTexture(context, d3dCubeMap);
}

void d3dToonShader::SetToonStyle(int s)
{
    m_style = (ToonStyle)s;
}

