/*===========================================================================
    prim.cpp
    02-Nov-00 Created by Neall

    Direct3D primitive rendering implementation

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include "direct3d.hpp"
#include "prim.hpp"
#include "util.hpp"
#include "display.hpp"
#include "vertprog.hpp"
#include "../base/baseshader.hpp"
#include "primstream.hpp"

// pddiPrimType
static D3DPRIMITIVETYPE primTable[] =
{
    D3DPT_TRIANGLELIST,
    D3DPT_TRIANGLESTRIP,
    D3DPT_LINELIST,
    D3DPT_LINESTRIP,
    D3DPT_POINTLIST,
};

// change this every time the format of the memory image changes
const int MEM_IMAGE_VERSION = 0x00020001;

//-------------------------------------------------------------------
class d3dPrimBufferStream : public pddiPrimBufferStream
{
public:
    d3dPrimBufferStream();
    void Lock()   { PDDIASSERT(!locked); locked = true; }
    void Unlock() { PDDIASSERT(locked); locked = false; }

    void SetBasePtr(void* basePtr, d3dVertexProgram* vs, int bufferCount, int offset);

    inline void Position(float x, float y, float z);
    inline void Normal(float x, float y, float z);
    inline void Colour(pddiColour colour, int channel = 0);
    inline void TexCoord1(float u, int channel = 0) {}
    inline void TexCoord2(float u, float v, int channel = 0);
    inline void TexCoord3(float u, float v, float s, int channel = 0) {}
    inline void TexCoord4(float u, float v, float s, float t, int channel = 0) {}
    inline void Specular(pddiColour colour);

    inline void SkinIndices(unsigned, unsigned = 0, unsigned = 0, unsigned = 0);
    inline void SkinWeights(float, float = 0.0f, float = 0.0f);

    inline void Vertex(pddiVector* v, pddiColour c);
    inline void Vertex(pddiVector* v, pddiVector* n);
    inline void Vertex(pddiVector* v, pddiVector2* uv);
    inline void Vertex(pddiVector* v, pddiColour c, pddiVector2* uv);
    inline void Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv);

    inline void Next() { NextVertex(); }
 
    bool locked;

private:
    d3dVertexProgram* vertexProgram;

    void* basePtr;
    int nVertex;
    int maxVertex;
    int stride;
    int nColourSets;
    int nUV;
    pddiVector* coord;
    float* normal;
    unsigned* compressedNormal;
    pddiColour* colour;
    pddiColour* specular;
    pddiVector2* uv;
    unsigned char* skinIndices;
    pddiVector* skinWeights;

    void NextVertex();
};


// We can only lock a fixed number of buffers at a time.  This should save some memory
// where there are thousands of prim groups laying around.
const int nStaticPrimBufferStreams = 4;
static d3dPrimBufferStream primBufferStreams[nStaticPrimBufferStreams];

static d3dPrimBufferStream* GetPrimBufferStream(void)
{
    for(int i = 0; i < nStaticPrimBufferStreams; i++)
    {
        if(!primBufferStreams[i].locked)
            return &primBufferStreams[i];
    }
    PDDIASSERT(0 && "no availible prim buffer stream for locking");
    return NULL;
}


//----------------------------------------------------------------------
d3dPrimBufferStream::d3dPrimBufferStream() :
    locked(false), basePtr(NULL), nVertex(0), maxVertex(0), stride(0),
    coord(NULL), normal(NULL), compressedNormal(NULL), colour(NULL),
    specular(NULL), uv(NULL)
{
}

void d3dPrimBufferStream::SetBasePtr(void* base, d3dVertexProgram* vs, int bufferCount, int offset)
{
    basePtr = base;
    nVertex = 0;
    maxVertex = bufferCount;
    normal = NULL;
    colour = NULL;
    specular = 0;
    uv = NULL;
    skinWeights = NULL;
    skinIndices = NULL;

    vertexProgram = vs;
    unsigned vertexMask = vertexProgram->GetFormat();

    nColourSets = PddiNumColourSets(vertexMask);
    nUV = vertexMask & 0xf;
    
    char* bufferPtr = (char*)basePtr;

    coord = (pddiVector*)basePtr;
    stride = vertexProgram->GetStride();
    bufferPtr += sizeof(pddiVector);

    if(vertexMask & PDDI_V_WEIGHTS)
    {
        skinWeights = (pddiVector*)bufferPtr;
        bufferPtr += sizeof(pddiVector);
    } 

    if(vertexMask & PDDI_V_INDICES)
    {
        skinIndices = (unsigned char*)bufferPtr;
        bufferPtr += sizeof(unsigned);
    }

    if(vertexMask & PDDI_V_NORMAL)
    {
        normal = (float*)bufferPtr;
        bufferPtr += sizeof(float) * 3;
    }

    if(vertexMask & PDDI_V_COLOUR)
    {
        colour = (pddiColour*)bufferPtr;
        bufferPtr += sizeof(unsigned);
    }
    else if((vertexMask & PDDI_V_COLOUR2) && (nColourSets > 0))
    {
        colour = (pddiColour*)bufferPtr;
        bufferPtr += (nColourSets*sizeof(unsigned));
    }

    if(vertexMask & PDDI_V_SPECULAR)
    {
        specular = (pddiColour*)bufferPtr;
        bufferPtr += sizeof(unsigned);
    }

    if(nUV)
    {
        uv = (pddiVector2*)bufferPtr;
        bufferPtr += (nUV*sizeof(pddiVector2));
    }
}

void d3dPrimBufferStream::NextVertex()
{
    PDDIASSERT(nVertex < maxVertex);
    nVertex++;

    coord = (pddiVector*)((char*)coord + stride);

    if(compressedNormal)
        compressedNormal = (unsigned*)((char*)compressedNormal + stride);

    if(normal)
        normal = (float*)((char*)normal + stride);

    if(colour)
        colour = (pddiColour*)((char*)colour + stride);

    if(specular)
        specular = (pddiColour*)((char*)specular + stride);

    if(uv)
        uv = (pddiVector2*)((char*)uv + stride);

    if(skinWeights)
        skinWeights = (pddiVector*)((char*)skinWeights + stride);

    if(skinIndices)
        skinIndices = (unsigned char*)((char*)skinIndices + stride);
}

void d3dPrimBufferStream::Position(float x, float y, float z)
{
    coord->x = x;
    coord->y = y;
    coord->z = z;
    NextVertex();
}

void d3dPrimBufferStream::Normal(float x, float y, float z)
{
    if(compressedNormal)
    {
        unsigned tmpx, tmpy, tmpz;

        float len = (float)sqrt(x*x + y*y + z*z) * 1.001f;
        x /= len;
        y /= len;
        z /= len;

        tmpx = (int(x * 0x7fffffff) >> 21) & (0xffffffff >> 21);
        tmpy = (int(y * 0x7fffffff) >> 21) & (0xffffffff >> 21);
        tmpy <<= 11;
        tmpz = (int(z * 0x7fffffff) >> 22) & (0xffffffff >> 22);
        tmpz <<= 22;

        *compressedNormal = tmpz | tmpy | tmpx;
    }
    else
    {
        normal[0] = x;
        normal[1] = y;
        normal[2] = z;
    }
}

void d3dPrimBufferStream::Colour(pddiColour c, int channel)
{
    colour[channel] = c;
}

void d3dPrimBufferStream::TexCoord2(float u, float v, int channel)
{
    uv[channel].u = u;
    uv[channel].v = v;
}

void d3dPrimBufferStream::Specular(pddiColour colour)
{
    *specular = colour;
}

void d3dPrimBufferStream::SkinIndices(unsigned a, unsigned b, unsigned c, unsigned d)
{
    skinIndices[0] = (unsigned char)c*3;
    skinIndices[1] = (unsigned char)b*3;
    skinIndices[2] = (unsigned char)a*3;
    skinIndices[3] = (unsigned char)d*3;
}

void d3dPrimBufferStream::SkinWeights(float a, float b, float c)
{

    skinWeights->x = a;
    skinWeights->y = b;
    skinWeights->z = c;
}

void d3dPrimBufferStream::Vertex(pddiVector* v, pddiColour c)
{
    *coord = *v;
    colour[0] = c;
    NextVertex();
}

void d3dPrimBufferStream::Vertex(pddiVector* v, pddiVector* n)
{
    *coord = *v;
    Normal(n->x, n->y, n->z);
    NextVertex();
}

void d3dPrimBufferStream::Vertex(pddiVector* v, pddiVector2* t)
{
    *coord = *v;
    uv[0] = *t;
    NextVertex();
}

void d3dPrimBufferStream::Vertex(pddiVector* v, pddiColour c, pddiVector2* t)
{
    *coord = *v;
    colour[0] = c;
    uv[0] = *t;
    NextVertex();
}

void d3dPrimBufferStream::Vertex(pddiVector* v, pddiVector* n, pddiVector2* t)
{
    *coord = *v;
    Normal(n->x, n->y, n->z);
    uv[0] = *t;
    NextVertex();
}

//-------------------------------------------------------------------
// Surface handles have to be > 0, but can be assigned arbitrarily.
const unsigned SURFACEHANDLEBASE = 1;

d3dPrimBuffer::d3dPrimBuffer(d3dContext* c) :
    buffer(NULL), 
    vertexMask(0), 
    maxVertex(0), 
    maxIndex(0), 
    nIndex(0), 
    useSize(0),
    indices(NULL),
    lockedVertex(false),
    lockedIndex(false),
    lockedMemImage(false)
{
    context = c;
    d3d = context->GetDisplay()->GetD3DDevice();
}

d3dPrimBuffer::~d3dPrimBuffer()
{
    if(indices)
    {
        pddiMemRegFree(indices);
        indices->Release();
    }

    if(buffer)
    {
        pddiMemRegFree(buffer);
        buffer->Release();
    }

    context->ADD_STAT(PDDI_STAT_BUFFERED_COUNT, -1);
    context->ADD_STAT(PDDI_STAT_BUFFERED_ALLOC, -float((vertexProgram->GetStride() * maxVertex) + (nIndex * 2)) / 1024.0f);
}

void d3dPrimBuffer::Create(pddiPrimType type, unsigned format, unsigned h, int nVertex, int nIndex, const char* vertexProgramName)
{
    static char dummy[] = "";
    static char skin[] = "skin_hktoon";
    static char skinob[] = "skin_onebone_hktoon";

    const char* name = dummy;

    PDDIASSERT(!lockedVertex);
    PDDIASSERT(!lockedIndex);
    primitiveType = primTable[type];
    primTypePDDI = type;
    vertexMask = format;

    if(vertexMask & PDDI_V_INDICES)
    {
        name = (vertexMask & PDDI_V_WEIGHTS) ? skin : skinob;
    }

    if(vertexProgramName)
    {
        name = vertexProgramName;
    }

    vertexProgram = context->GetVertexProgram(name, type, format, pddiExtVertexProgram::COMPRESSED);

    SetNumVertices(nVertex);
    SetNumIndices(nIndex);

    context->ADD_STAT(PDDI_STAT_BUFFERED_COUNT, 1);
    context->ADD_STAT(PDDI_STAT_BUFFERED_ALLOC, float((vertexProgram->GetStride() * maxVertex) + (nIndex * 2)) / 1024.0f);
}

void d3dPrimBuffer::CreateStripList(pddiPrimType type, unsigned fmt, int maxVertex, int nStrip, int* strLen)
{
    PDDIASSERT(0);
}

void d3dPrimBuffer::SetNumVertices(int count)
{
    PDDIASSERT(!lockedVertex);
    maxVertex = count;

    if(buffer)
    {
        buffer->Release();
        pddiMemRegFree(buffer);
    }

    DWORD usage = D3DUSAGE_WRITEONLY;

    if( ( vertexMask & PDDI_V_INDICES ) & (!context->IsHardwareVertexShader( )) )          //skin
        usage |= D3DUSAGE_SOFTWAREPROCESSING;

    HRESULT res = d3d->CreateVertexBuffer(vertexProgram->GetStride() * maxVertex,
                                                      usage,
                                                      0,
                                                      D3DPOOL_MANAGED,
                                                      &buffer);
    pddiMemRegAlloc(buffer, vertexProgram->GetStride() * maxVertex);
    DDVERIFY(res);
}

void d3dPrimBuffer::SetNumIndices(int count)
{
    PDDIASSERT(!lockedIndex);
    maxIndex = count;

    if(indices)
    {
        indices->Release();
        pddiMemRegFree(indices);
    }

    indices = NULL;

    if(count)
    {
        DWORD usage = D3DUSAGE_WRITEONLY;

        if( ( vertexMask & PDDI_V_INDICES ) & (!context->IsHardwareVertexShader( )) )          //skin
            usage |= D3DUSAGE_SOFTWAREPROCESSING;

        HRESULT res = d3d->CreateIndexBuffer(count * sizeof(unsigned short), usage, D3DFMT_INDEX16, D3DPOOL_MANAGED, &indices);
        DDVERIFY(res);
        pddiMemRegAlloc(indices, count * sizeof(unsigned short));
    }
}

pddiPrimBufferStream* d3dPrimBuffer::Lock()
{
    unsigned char* basePtr = 0;
    HRESULT res = buffer->Lock(0,0,&basePtr, 0);
    DDVERIFY(res);
    lockedVertex = true;
    d3dPrimBufferStream* stream =  GetPrimBufferStream();
    stream->Lock();
    stream->SetBasePtr(basePtr, vertexProgram, maxVertex, 0);
    return stream;
}

void d3dPrimBuffer::Unlock(pddiPrimBufferStream* stream)
{
    PDDIASSERT(lockedVertex);
    lockedVertex = false;
    ((d3dPrimBufferStream*)stream)->Unlock();
    buffer->Unlock();
}

unsigned char* d3dPrimBuffer::LockIndexBuffer()
{
    PDDIASSERT(maxIndex>0);
    unsigned char* indexData;
    HRESULT rc = indices->Lock(0, maxIndex * sizeof(unsigned short), &indexData, 0);
    DDVERIFY(rc);
    lockedIndex = true;
    return indexData;
}

void d3dPrimBuffer::UnlockIndexBuffer(int count)
{
    PDDIASSERT(lockedIndex);
    lockedIndex = false;
    indices->Unlock();

    nIndex = count;
}

bool d3dPrimBuffer::CheckMemImageVersion(int version) 
{ 
    return version == MEM_IMAGE_VERSION;
}

void* d3dPrimBuffer::LockMemImage(unsigned int size) 
{ 
    PDDIASSERT(size == GetMemImageLength());

    unsigned char* basePtr;
    HRESULT res = buffer->Lock(0,0,&basePtr, 0);
    DDVERIFY(res);

    lockedMemImage = true;

    return basePtr;
}

void d3dPrimBuffer::UnlockMemImage() 
{
    PDDIASSERT(lockedMemImage);
    lockedMemImage = false;
    buffer->Unlock();
}

unsigned d3dPrimBuffer::GetMemImageLength() 
{ 
    return vertexProgram->GetStride() * maxVertex;
}

void d3dPrimBuffer::SetMemImageParam(unsigned param, unsigned value) 
{ 
    /**/ 
}

void d3dPrimBuffer::SetIndices(unsigned short* id, int count)
{
    PDDIASSERT(maxIndex >= count);
    unsigned char* indexData;

    HRESULT rc = indices->Lock(0, count * sizeof(unsigned short), &indexData, 0);
    DDVERIFY(rc);

    memcpy(indexData, id, count * sizeof(unsigned short));
    indices->Unlock();

    nIndex = count;
}

void d3dPrimBuffer::Display()
{
    unsigned tmp;
    if(useSize)
    {
        if(nIndex > 0)
        {
            tmp = nIndex;
            nIndex = useSize;
        }
        else
        {
            tmp = maxVertex;
            maxVertex = useSize;
        }
    }

    PDDIASSERT(!lockedVertex);
    PDDIASSERT(!lockedIndex);

    if(nIndex > 0)
    {
        d3d->SetIndices(indices, 0);
        d3d->SetVertexShader(vertexProgram->GetD3DVS());
        d3d->SetStreamSource(0, buffer, vertexProgram->GetStride());
        HRESULT res = d3d->DrawIndexedPrimitive(primitiveType, 0, maxVertex, 0, VertsToPrims(primTypePDDI, nIndex));
        DDVERIFY(res);
    }
    else  
    {
        d3d->SetVertexShader(vertexProgram->GetD3DVS());
        d3d->SetStreamSource(0, buffer, vertexProgram->GetStride());
        HRESULT res = d3d->DrawPrimitive(primitiveType, 0, VertsToPrims(primTypePDDI, maxVertex));
        DDVERIFY(res);
    }

#ifdef PDDI_TRACK_STATS
//   if(!supressPrimTally)
    {
        context->ADD_STAT(PDDI_STAT_BUFFERED_PRIM_CALLS, 1);
        if(nIndex > 0)
        {
                context->ADD_STAT(PDDI_STAT_BUFFERED_INDEXED_PRIM, (float)VertsToPrims(primTypePDDI, nIndex));
                context->ADD_STAT(PDDI_STAT_BUFFERED_INDEXED_PRIM_VERT, (float)nIndex);
        }
        else
        {
                context->ADD_STAT(PDDI_STAT_BUFFERED_PRIM, (float)VertsToPrims(primTypePDDI, maxVertex));
                context->ADD_STAT(PDDI_STAT_BUFFERED_PRIM_VERT, (float)maxVertex);
        }
    }
#endif

    if(useSize)
    {
        if(nIndex > 0)
        {
            nIndex = tmp;
        }
        else
        {
            maxVertex = tmp;
        }
    }
}
