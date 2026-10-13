//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _VERTPROG_HPP_
#define _VERTPROG_HPP_

#include "../pddienum.hpp"
#include "d3dx8core.h"
#include "context.hpp"
#include "display.hpp"


#define VSH_TEST

class d3dContext;
class d3dShader;

const int maxPrograms = 32;
const int maxTemplates = 16;

class d3dVertexProgram
{
public:
    unsigned GetStride(void) { return stride; }
    unsigned GetFormat(void) { return vertexFormat; }
    unsigned GetAuxFormat(void) { return vertexFormatAux; }
    pddiPrimType GetPrimType(void) { return primType; }

    unsigned GetD3DVS(void)
    { 
        if (!vertexShader) // need to rebuild
        {
            DDVERIFY(context->GetDisplay()->GetD3DDevice()->CreateVertexShader(decl,tokens, (DWORD*)&vertexShader, usage));
        }
        PDDIASSERT(vertexShader && "Invalid vertex shader handle");
        return vertexShader;
    }

    virtual void Setup(d3dContext* c, d3dShader*) { context = c;};
    d3dVertexProgram()
    {
        vertexShader = 0;
    }
    ~d3dVertexProgram(){ /* nop */ }

protected:
    friend class d3dVertexProgramManager;

    void SetData(pddiPrimType pr, unsigned vf, unsigned vfa, unsigned s, unsigned sh)
    { 
        primType = pr;
        vertexFormat = vf;
        vertexFormatAux = vfa;
        stride = s;
        vertexShader = sh;
    }

    void SetVsData( const DWORD* new_decl, unsigned long* tok, const DWORD use )
    {
        memcpy( decl, new_decl, sizeof(decl) );
        tokens = tok;
        usage = use;
    }

    pddiPrimType primType;

    unsigned vertexFormat;
    unsigned vertexFormatAux;
    unsigned stride;
    unsigned vertexShader;

    // info needed to rebuild this shader
    d3dContext* context;
    DWORD decl[64];
    DWORD usage;
    unsigned long* tokens;
};

class d3dVertexProgramManager
{
public:
    d3dVertexProgramManager(d3dContext* c);
    ~d3dVertexProgramManager();

    void AssembleVertexShader( const char * vsName );
    void AddVertexProgram( const char* name );
    d3dVertexProgram* GetVertexProgram(const char* name, pddiPrimType primType, unsigned format, unsigned aux);
    void Reset(void);

protected:
    d3dContext* context;

    unsigned nTemplates;

    struct Template
    {
        unsigned name;
#ifndef  VSH_TEST
        LPD3DXBUFFER tokens;
#else
        void * tokens;
#endif
        int nPrograms;
        d3dVertexProgram programs[maxPrograms];
    } vsTemplates[maxTemplates];
};

#endif
