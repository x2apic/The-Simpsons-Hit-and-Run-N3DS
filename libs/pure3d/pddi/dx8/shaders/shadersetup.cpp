//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "device.hpp"
#include "display.hpp"
#include "context.hpp"
#include "texture.hpp"
#include "prim.hpp"
#include "util.hpp"
#include "shadersetup.hpp"
#include "../base/baseshader.hpp"
#include "simpleshader.hpp"
#include "errorshader.hpp"
#include "lightmapshader.hpp"
#include "reflectshader.hpp"
#include "projectedtextureshader.hpp"
#include "pointspriteshader.hpp"
#include "layeredshader.hpp"
#include "layeredlmapshader.hpp"
#include "hktoonshader.hpp"

static pddiBaseShader* AllocErrorShader      ( pddiRenderContext* c, const char*, const char*)           { return new d3dErrorShader      ( (d3dContext*)c); }
static pddiBaseShader* AllocSimpleShader     ( pddiRenderContext* c, const char*, const char*)           { return new d3dSimpleShader     ( (d3dContext*)c); }
static pddiBaseShader* AllocLightmapShader   ( pddiRenderContext* c, const char* name, const char* aux)  { return new d3dLightMapShader   ( (d3dContext*)c); }
static pddiBaseShader* AllocReflShader       ( pddiRenderContext* c, const char* name, const char* aux)  { return new d3dReflShader       ( (d3dContext*)c); }
static pddiBaseShader* AllocProjectedTextureShader ( pddiRenderContext* c, const char* name, const char* aux)  { return new d3dProjectedTextureShader ( (d3dContext*)c); }
static pddiBaseShader* AllocPointSpriteShader ( pddiRenderContext* c, const char* name, const char* aux)  { return new d3dPointSpriteShader ( (d3dContext*)c); }
static pddiBaseShader* AllocLayeredShader ( pddiRenderContext* c, const char* name, const char* aux)  { return new d3dLayeredShader ( (d3dContext*)c); }
static pddiBaseShader* AllocLayeredLMapShader ( pddiRenderContext* c, const char* name, const char* aux)  { return new d3dLayeredLMapShader ( (d3dContext*)c); }
static pddiBaseShader* AllocToonVertexShader       ( pddiRenderContext* c, const char* name, const char* aux)  { return new d3dHKToonShader       ( (d3dContext*)c); }

void d3dShaderSetup(LPDIRECT3D8 d3d)
{
    D3DCAPS8 caps;
    d3d->GetDeviceCaps(D3DADAPTER_DEFAULT ,D3DDEVTYPE_HAL,&caps);

    pddiBaseShader::InstallShader("error",                 AllocErrorShader, NULL);
    pddiBaseShader::InstallShader("simple",                AllocSimpleShader, NULL);
    pddiBaseShader::InstallShader("lightweight",           AllocSimpleShader, NULL);
    pddiBaseShader::InstallShader("lightmap",              AllocLightmapShader, NULL);

    // If the video card has 2 or less maximum simultaneous textures per stage
    // it probably only has 2 or less "working" texture stages. Reflection shader requires 3 stages.
    // To be safe, if the card doesn't support 3 or more simultaneous textures
    // we will disable the reflection shader even though maximum blend stages says 8
    if ( caps.MaxSimultaneousTextures > 2 )
    {
        pddiBaseShader::InstallShader("environment",           AllocReflShader, NULL);
        pddiBaseShader::InstallShader("spheremap",             AllocReflShader, NULL);
    }
    else
    {
        pddiBaseShader::InstallShader("environment",           AllocSimpleShader, NULL);
        pddiBaseShader::InstallShader("spheremap",             AllocSimpleShader, NULL);
    }

    pddiBaseShader::InstallShader("projtex",               AllocProjectedTextureShader, NULL);
    pddiBaseShader::InstallShader("pointsprite",           AllocPointSpriteShader, NULL);
    pddiBaseShader::InstallShader("layered",               AllocLayeredShader, NULL);
    pddiBaseShader::InstallShader("layeredlmap",           AllocLayeredLMapShader, NULL);

    // If pixel shader version is version 1.1 or greater then toon shading will work, otherwise
    // default to the simple shader
    UINT ps_major = D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion);
    UINT ps_minor = D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion);
    if ( caps.PixelShaderVersion < D3DPS_VERSION( 1,1 ) ) 
    {        
        pddiBaseShader::InstallShader("toon",                  AllocSimpleShader, NULL);
        pddiBaseShader::InstallShader("hctune",                AllocSimpleShader, NULL);
    }
    else
    {
        pddiBaseShader::InstallShader("toon",                  AllocToonVertexShader, NULL);
        pddiBaseShader::InstallShader("hctune",                AllocToonVertexShader, NULL);
    }
}
