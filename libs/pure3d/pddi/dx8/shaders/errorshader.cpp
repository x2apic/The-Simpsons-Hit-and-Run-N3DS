//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "errorshader.hpp"
#include "context.hpp"

#include "display.hpp"
#include "texture.hpp"
#include "direct3d.hpp"

d3dErrorShader::d3dErrorShader(d3dContext* c) :
    d3dShader(c)
{ 
    isLit = true;
    shaderInfo.ambient.Set(0,0,0);
    shaderInfo.diffuse.Set(0,0,0);
    shaderInfo.specular.Set(0,0,0);
    shaderInfo.emissive.Set(255,128,255);
}

const char* d3dErrorShader::GetType(void)
{
    static char type [] = "error";
    return type;
}

int  d3dErrorShader::GetPasses()
{
    return 1;
}

void d3dErrorShader::SetPass(int pass)
{
    d3d->SetTextures(0, NULL);
    SetupShading();
}



