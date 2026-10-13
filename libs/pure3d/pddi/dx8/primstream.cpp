//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "primstream.hpp"
#include "context.hpp"
#include "display.hpp"
#include "vertprog.hpp"
#include "shaders\shader.hpp"

// pddiPrimType
static D3DPRIMITIVETYPE primTable[] =
{
    D3DPT_TRIANGLELIST,
    D3DPT_TRIANGLESTRIP,
    D3DPT_LINELIST,
    D3DPT_LINESTRIP,
    D3DPT_POINTLIST,
};

// number of vertices that have to be copied into the next vertex
// buffer when a stripped primitive spans multiple buffers
static int dupTable[] =
{
    0, // D3DPT_TRIANGLELIST,
    2, // D3DPT_TRIANGLESTRIP,
    0, // D3DPT_LINELIST,
    1, // D3DPT_LINESTRIP,
    0, // D3DPT_POINTLIST
};

//----------------------------------------------------------------------

d3dPrimStream::d3dPrimStream(d3dContext* c) :
    d3d(0), locked(false), context(c)
{
    nVertex = 0, 
    stride = 0;
    coord = NULL;
    normal = NULL;
    colour = NULL;
    uv = NULL;

    setVertexProgram = 0;

    d3d = context->GetDisplay()->GetD3DDevice();
    bufferIdx = 0;

    for(int i=0; i < PRIM_STREAM_BUFFER_COUNT; i++)
    {
        HRESULT res = d3d->CreateVertexBuffer(VERTEX_BUFFER_SIZE, 0 /*D3DUSAGE_WRITEONLY*/, 0, D3DPOOL_MANAGED, &bufferRing[i]);
        DDVERIFY(res);
    }

    buffer = bufferRing[bufferIdx];

    vertexMask = 0;
}

d3dPrimStream::~d3dPrimStream()
{
    for(int i=0; i < PRIM_STREAM_BUFFER_COUNT; i++)
    {
        bufferRing[i]->Release();
    }
}

void d3dPrimStream::Begin(pddiPrimType type, unsigned vMask, pddiBaseShader* s, unsigned e)
{
    expected = e;

    if(setVertexProgram)
    {
        vertexProgram = setVertexProgram;
    }
    else
    {
        vertexProgram = context->GetVertexProgram("", type, vMask, 0);
    }

    PDDIASSERT(vertexProgram->GetFormat() == vMask);

    nColourSets = PddiNumColourSets(vMask);
    nUV = vMask & 0xf;

    maxVertexCount = VERTEX_BUFFER_SIZE / vertexProgram->GetStride();

    // vertex count needs to be even and divisible by 3 or stuff will split badly across buffers
    maxVertexCount = (maxVertexCount / 6) * 6;

    primitiveType = primTable[type];
    primTypePDDI = type;
    vertexMask = vMask;
    nDup = dupTable[type];
    nVertex = 0;
    shader = s;
    Reset(false);

#ifdef PDDI_TRACK_STATS
//      if(!supressPrimTally)
        {
            context->ADD_STAT(PDDI_STAT_STREAMED_PRIM_CALLS, 1);
        }
#endif
}

void d3dPrimStream::Reset(bool dup)
{
    unsigned char* bufferPtr = NULL;
    HRESULT res = buffer->Lock(0, 0, &bufferPtr, 0);
    DDVERIFY(res);
    locked = true;

    if(dup && (nDup > 0))
    {
        // Copy some vertices into the next buffer since we might have
        // flushed right in the middle of a strip.  We copy one vertex for
        // a linestrip, and two for a tristrip.
        memcpy(bufferPtr, dupBuf, stride*nDup);
        bufferPtr += stride*nDup;
        nVertex = nDup;
        expected += nDup;
    }
    else
    {
        nVertex = 0;
        expected -= nVertex;
    }

    coord = (pddiVector*)bufferPtr;
    stride = sizeof(float)*3;
    bufferPtr += sizeof(pddiVector);

    if(vertexProgram->GetFormat() & PDDI_V_NORMAL)
    {
        normal = (pddiVector*)bufferPtr;
        stride += sizeof(float)*3;
        bufferPtr += sizeof(float)*3;
    }

    if(vertexProgram->GetFormat() & PDDI_V_COLOUR)
    {
        colour = (pddiColour*)bufferPtr;
        stride += sizeof(unsigned);
        bufferPtr += sizeof(unsigned);
    }
    else if((vertexProgram->GetFormat() & PDDI_V_COLOUR2) && (nColourSets > 0))
    {
        colour = (pddiColour*)bufferPtr;
        stride += sizeof(unsigned)*nColourSets;
        bufferPtr += sizeof(unsigned)*nColourSets;
    }

    if(nUV)
    {
        uv = (pddiVector2*)bufferPtr;
        stride += sizeof(float)*2*nUV;
        bufferPtr += sizeof(float)*2*nUV;
    }
}

void d3dPrimStream::Flush()
{
    if(locked)
    {
        buffer->Unlock();
        locked = false;
    }

    if((nVertex <= 0) || ((primitiveType == D3DPT_TRIANGLESTRIP) && (nVertex <= 2)) || ((primitiveType == D3DPT_LINESTRIP) && (nVertex <= 1)))
    {
        expected -= nVertex;
        nVertex = 0;
        return;
    }

    for(int i = 0; i < shader->GetPasses(); i++)
    {
        shader->SetMaterial(i);
        d3d->SetVertexShader(vertexProgram->GetD3DVS());
        d3d->SetStreamSource(0, buffer, stride);
        HRESULT res = d3d->DrawPrimitive(primitiveType, 0, VertsToPrims(primTypePDDI,nVertex));
        DDVERIFY(res);

#ifdef PDDI_TRACK_STATS
//      if(!supressPrimTally)
        {
            context->ADD_STAT(PDDI_STAT_STREAMED_PRIM, (float)VertsToPrims(primTypePDDI, nVertex));
            context->ADD_STAT(PDDI_STAT_STREAMED_PRIM_VERT, (float)nVertex);
        }
#endif

    }

    NextBuffer();
    expected -= nVertex;
    nVertex = 0;
}

void d3dPrimStream::End()
{
    Flush();
    PDDIASSERT(expected == 0);
}

void d3dPrimStream::NextBuffer()
{
    if(++bufferIdx == PRIM_STREAM_BUFFER_COUNT)
    {
        bufferIdx = 0;
    }
    buffer = bufferRing[bufferIdx];
}


