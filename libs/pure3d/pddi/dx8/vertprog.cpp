//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "vertprog.hpp"
#include "context.hpp"
#include "display.hpp"
#include "direct3d.hpp"
#include "../pddiext.hpp"
#include "util.hpp"
#include "../base/debug.hpp"
#include "resource.h"

#ifdef VSH_TEST
#include "vsh/skin.h"
#include "vsh/skin_onebone.h"
#include "vsh/skin_hktoon.h"
#include "vsh/skin_onebone_hktoon.h"
#endif

#define DEF_TEMPLATE(x,y) { vsTemplates[nTemplates].name = Hash(x); vsTemplates[nTemplates].tokens = y; vsTemplates[nTemplates ].nPrograms = 0; nTemplates++;}
#define ADD_DECL(x,y)  {decl[curDecl++] = x; stride += y;}

// Hash() - a hashing function that uses the "hashpjw"
// algorithm.  Taken from Dragon book, p436
inline unsigned long Hash(const char *x)
{
    unsigned long h = 0;
    unsigned long g;

    while(*x != 0)
    {
        h = (h << 4) + *x++;
        if ((g = h & 0xf0000000) != 0)
        {
            h = (h ^ (g >> 24)) ^ g;
        }
    }
    return h;
}

d3dVertexProgramManager::d3dVertexProgramManager(d3dContext* c)
{
    context = c;
    nTemplates = 0;

#ifdef VSH_TEST   
    DEF_TEMPLATE( "", 0 );
    DEF_TEMPLATE("skin", skinVertexShaderTokens);
    DEF_TEMPLATE("skin_onebone", skin_oneboneVertexShaderTokens);
    DEF_TEMPLATE("skin_hktoon", skin_hktoonVertexShaderTokens);
    DEF_TEMPLATE("skin_onebone_hktoon", skin_onebone_hktoonVertexShaderTokens);
#else
    AssembleVertexShader( NULL );    
    AssembleVertexShader( "skin" );
#endif

    PDDIASSERT(nTemplates <= maxTemplates);
}

d3dVertexProgramManager::~d3dVertexProgramManager()
{   
#ifndef VSH_TEST
    for(unsigned i = 0; i < nTemplates; i++){
        if( vsTemplates[ i ].tokens )
            vsTemplates[ i ].tokens->Release( );
    }   
#endif
}


void d3dVertexProgramManager::AssembleVertexShader( const char * vsName )
{
    if( vsName == NULL ){   //declare a null vertex shader as the header
        vsTemplates[ nTemplates ].name = Hash( "" );
        vsTemplates[ nTemplates ].tokens = NULL;
        vsTemplates[ nTemplates ].nPrograms = 0; 
        nTemplates++;
    }
    else{
        //resource name is the same as vertex shader name
        vsTemplates[ nTemplates ].name = Hash( vsName ); 
        HMODULE hr = GetModuleHandle( "pddidx8d" );
#ifndef VSH_TEST
/*
        LPD3DXBUFFER error;
        HRSRC hvsh = FindResource( hr, MAKEINTRESOURCE(IDR_VSH3), "vsh" );
        HGLOBAL hshader = LoadResource( hr, hvsh );

        DDVERIFY( D3DXAssembleShaderFromResource( NULL, MAKEINTRESOURCE(IDR_VSH3), 
        0, NULL, &(vsTemplates[nTemplates].tokens), NULL ) );  // assemble shader code

        char * msg;

        if( error )
            msg = (char*) error->GetBufferPointer( );

*/

        DDVERIFY( D3DXAssembleShaderFromFile( "D:\\personal_branch\\development\\hongzhi\\pure3d\\pddi\\dx8\\vsh\\skin.vsh", 
            0, NULL, &(vsTemplates[nTemplates].tokens), NULL ) );  // assemble shader code
#endif
        vsTemplates[ nTemplates ].nPrograms = 0; 
        nTemplates++;
    }

}

void d3dVertexProgramManager::Reset(void)
{
    for(unsigned i = 0; i < nTemplates; i++)
    {
        for(int j=0; j < vsTemplates[i].nPrograms; j++)
        {
            vsTemplates[i].programs[j].vertexShader = 0;
        }
        //vsTemplates[i].nPrograms = 0;
    }
}

d3dVertexProgram* d3dVertexProgramManager::GetVertexProgram(const char* name, pddiPrimType primType, unsigned vertexMask, unsigned aux)
{
    Template* base = &vsTemplates[0];
    unsigned id = Hash(name);
    bool compressed = (aux & pddiExtVertexProgram::COMPRESSED) != 0;

    for(unsigned i = 0; i < nTemplates; i++)
    {
        if(vsTemplates[i].name == id)
        {
            base = &vsTemplates[i];
            break;
        }
    }

    for(unsigned i = 0; i < (unsigned)base->nPrograms; i++)
    {
        if((primType == base->programs[i].GetPrimType()) && (vertexMask == base->programs[i].GetFormat()) && (aux == base->programs[i].GetAuxFormat()))
        {
            int tmp;
            if( i == 6 )
                tmp = 0;

            return &base->programs[i];
        }
    }

    PDDIASSERT(base->nPrograms < maxPrograms);
    d3dVertexProgram* program = &base->programs[base->nPrograms];
    program->Setup( context, NULL );
    base->nPrograms++;

    DWORD decl[64];
    unsigned curDecl = 0;
    unsigned stride = 0;

    ADD_DECL(D3DVSD_STREAM( 0 ), 0);
    ADD_DECL(D3DVSD_REG( D3DVSDE_POSITION,  D3DVSDT_FLOAT3 ), 12);

    unsigned uvCount = vertexMask & 0xf;

    if(vertexMask & PDDI_V_WEIGHTS)
    {
 
        ADD_DECL(D3DVSD_REG( D3DVSDE_BLENDWEIGHT,  D3DVSDT_FLOAT3 ), 12);

    }

    if(vertexMask & PDDI_V_INDICES)
    {
        ADD_DECL(D3DVSD_REG( D3DVSDE_BLENDINDICES,  D3DVSDT_D3DCOLOR ), 4)
    }

    if(vertexMask & PDDI_V_NORMAL)
    {

        ADD_DECL(D3DVSD_REG( D3DVSDE_NORMAL,  D3DVSDT_FLOAT3 ), 12);

    }

    if(vertexMask & PDDI_V_COLOUR)
    {
        ADD_DECL(D3DVSD_REG( D3DVSDE_DIFFUSE,  D3DVSDT_D3DCOLOR ), 4)
    }

    if(vertexMask & PDDI_V_COLOUR2)
    {
        ADD_DECL(D3DVSD_REG( D3DVSDE_DIFFUSE,  D3DVSDT_D3DCOLOR ), 4)
        ADD_DECL(D3DVSD_REG( D3DVSDE_SPECULAR ,  D3DVSDT_D3DCOLOR ), 4)
    }

    if(vertexMask & PDDI_V_SPECULAR)
    {
        ADD_DECL(D3DVSD_REG( D3DVSDE_SPECULAR ,  D3DVSDT_D3DCOLOR ), 4)
    }

    for(unsigned i = 0; i < uvCount; i++)
    {
        ADD_DECL(D3DVSD_REG( D3DVSDE_TEXCOORD0 + i,  D3DVSDT_FLOAT2 ), 8);
    }

    if(vertexMask & PDDI_V_SIZE)
    {
        ADD_DECL(D3DVSD_REG( 14,  D3DVSDT_FLOAT1 ), 4)
    }
    if(vertexMask & PDDI_V_W)
    {
        ADD_DECL(D3DVSD_REG( 13,  D3DVSDT_FLOAT1 ), 4)
    }
	if( vertexMask & PDDI_V_BINORMAL )
	{
        ADD_DECL(D3DVSD_REG( 6,  D3DVSDT_FLOAT3 ), 12);		//bi-normal to v6
    }
	if( vertexMask & PDDI_V_TANGENT )
	{
        ADD_DECL(D3DVSD_REG( 15,  D3DVSDT_FLOAT3 ), 12);	//tangent to v15
    }

    ADD_DECL(D3DVSD_END(), 0);

    unsigned long vs = 0;

/*
    if( vertexMask & PDDI_V_INDICES )
        context->DetectSkinVertexProcessor(  );
*/
    DWORD usage = 0;

    if( !context->IsHardwareVertexShader( ) && ( vertexMask & PDDI_V_INDICES ) )
    {
        usage |= D3DUSAGE_SOFTWAREPROCESSING ;    
        context->GetDisplay()->GetD3DDevice()->SetRenderState(D3DRS_SOFTWAREVERTEXPROCESSING , TRUE);
    }

/*
#ifndef VSH_TEST
    if( base->tokens ){
        LPVOID stream = base->tokens->GetBufferPointer( );
        DDVERIFY(context->GetDisplay()->GetD3DDevice()->CreateVertexShader(decl,  (DWORD*)(stream), &vs, usage));
    }
    else
        DDVERIFY(context->GetDisplay()->GetD3DDevice()->CreateVertexShader(decl,  NULL, &vs, usage));
#else
    DDVERIFY(context->GetDisplay()->GetD3DDevice()->CreateVertexShader(decl,  (unsigned long*)base->tokens, &vs, usage));
#endif
*/
    program->SetData(primType, vertexMask, aux, stride, vs);
    program->SetVsData( decl, (unsigned long*)base->tokens, usage );
    return program;
}


void d3dVertexProgramManager::AddVertexProgram( const char* name )
{
    unsigned id = Hash( name );
    for(unsigned i = 0; i < nTemplates; i++)
    {
        if(vsTemplates[i].name == id)
        {
            // Program already added
            return;
        }
    }

    AssembleVertexShader( name );
}
