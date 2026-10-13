//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PRIMSTREAM_HPP_
#define _PRIMSTREAM_HPP_

#include "../pddi.hpp"
#include <string.h>
#include "direct3d.hpp"

class pddiBaseShader;
class d3dContext;
class d3dVertexProgram;

class d3dBasePrimStream : public pddiPrimStream
{
public:
    virtual void Reset(bool duplicate) = 0;
    virtual void Flush() = 0;

    inline void Coord(float x, float y, float z);
    inline void Normal(float x, float y, float z);
    inline void Binormal(float x, float y, float z) { };
    inline void Tangent(float x, float y, float z) { };
    inline void Colour(pddiColour colour, int channel = 0);
    inline void UV(float u, float v, int channel = 0);
    inline void Specular(pddiColour colour);

    inline void Vertex(pddiVector* v, pddiColour c);
    inline void Vertex(pddiVector* v, pddiVector* n);
    inline void Vertex(pddiVector* v, pddiVector2* uv);
    inline void Vertex(pddiVector* v, pddiColour c, pddiVector2* uv);
    inline void Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv);
    
protected:
    int stride;
    pddiVector* coord;
    pddiVector* normal;
    pddiColour* colour;
    pddiVector2* uv;

    int maxVertexCount;
    int nVertex;
    int nDup;
    char dupBuf[256]; // buffer to store vertices of broken strips

    inline void NextVertex();
};

// all the inline functions follow
inline void d3dBasePrimStream::NextVertex()
{
    nVertex++;
    if(nVertex == maxVertexCount)
    {
        if(nDup > 0)
        {
            // Keep track of trailing vertices in case we have to duplicate
            // them in the next buffer.
            memcpy(dupBuf, (char*)coord - stride*(nDup-1), stride*nDup);
        }
        Flush();
        Reset(true);
    }
    else
    {
        coord = (pddiVector*)((char*)coord + stride);
        if(normal)
          normal = (pddiVector*)((char*)normal + stride);
        if(colour)
            colour = (pddiColour*)((char*)colour + stride);
        if(uv)
            uv = (pddiVector2*)((char*)uv + stride);
    }
}

inline void d3dBasePrimStream::Coord(float x, float y, float z)
{
    coord->x = x;
    coord->y = y;
    coord->z = z;
    NextVertex();
}

inline void d3dBasePrimStream::Normal(float x, float y, float z)
{
    normal->x = x;
    normal->y = y;
    normal->z = z;
}

inline void d3dBasePrimStream::Colour(pddiColour c, int channel)
{
    colour[channel] = c;
}

inline void d3dBasePrimStream::UV(float u, float v, int channel)
{
    uv[channel].u = u;
    uv[channel].v = v;
}

inline void d3dBasePrimStream::Specular(pddiColour colour)
{
//   not yet
}

inline void d3dBasePrimStream::Vertex(pddiVector* v, pddiColour c)
{
    *coord = *v;
    colour[0] = c;
    NextVertex();
}

inline void d3dBasePrimStream::Vertex(pddiVector* v, pddiVector* n)
{
    *coord = *v;
    *normal = *n;
    NextVertex();
}

inline void d3dBasePrimStream::Vertex(pddiVector* v, pddiVector2* t)
{
    *coord = *v;
    uv[0] = *t;
    NextVertex();
}

inline void d3dBasePrimStream::Vertex(pddiVector* v, pddiColour c, pddiVector2* t)
{
    *coord = *v;
    colour[0] = c;
    uv[0] = *t;
    NextVertex();
}

inline void d3dBasePrimStream::Vertex(pddiVector* v, pddiVector* n, pddiVector2* t)
{
    *coord = *v;
    *normal = *n;
    uv[0] = *t;
    NextVertex();
}

//-------------------------------------------------------------------
const int VERTEX_BUFFER_SIZE = 258 * 2 * 32;
const int PRIM_STREAM_BUFFER_COUNT = 8;

class d3dPrimStream : public d3dBasePrimStream
{
public:
    d3dPrimStream(d3dContext* d3d);
    ~d3dPrimStream();

    void Reset(bool dup);
    void Flush();
    void Begin(pddiPrimType primType, unsigned vertexType, pddiBaseShader* shader, unsigned expected);
    void End();

    void SetVertexProgram(d3dVertexProgram* d) {setVertexProgram = d;}
    
    pddiPrimType GetPddiPrimType() { return primTypePDDI; }
    unsigned GetVertexMask()   { return vertexMask; }
        
private:
    d3dContext* context; 
    LPDIRECT3DDEVICE8 d3d;
    LPDIRECT3DVERTEXBUFFER8 bufferRing[PRIM_STREAM_BUFFER_COUNT];
    LPDIRECT3DVERTEXBUFFER8 buffer;

    bool locked;

    D3DPRIMITIVETYPE primitiveType;
    pddiPrimType primTypePDDI;
    unsigned vertexMask;
    int bufferIdx;
    int nColourSets;
    int nUV;
    pddiBaseShader* shader;

    d3dVertexProgram* setVertexProgram;
    d3dVertexProgram* vertexProgram;

    int expected;

    void NextBuffer();
};


#endif
