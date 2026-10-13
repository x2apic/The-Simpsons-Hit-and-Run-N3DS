//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _ERRORSHADER_HPP_
#define _ERRORSHADER_HPP_

#include"..\pddi.hpp"
#include"shader.hpp"

class d3dContext;
class d3dTexture;

class d3dErrorShader : public d3dShader
{
public:
    d3dErrorShader(d3dContext* c);

    const char* GetType(void);
    int  GetPasses();

protected:
    void SetPass(int pass = 0);
};

#endif
