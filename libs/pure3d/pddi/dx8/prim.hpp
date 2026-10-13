/*===========================================================================
    prim.hpp
    02-Nov-00 Created by Neall

    Direct3D primitive rendering interface

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _PRIM_HPP
#define _PRIM_HPP

#include "context.hpp"
#include "../base/baseshader.hpp"
#include "../base/debug.hpp"

//-------------------------------------------------------------------
class d3dPrimBuffer : public pddiPrimBuffer
{
public:
    virtual ~d3dPrimBuffer();
    d3dPrimBuffer(d3dContext* context);

    void Create(pddiPrimType type, unsigned vertexFormat,
        unsigned compressionHint, int maxVertex, int maxIndex,
        const char* vertexProgram);

    void CreateStripList(pddiPrimType type, unsigned fmt,
        int maxVertex, int nStrip, int* strLen);

    pddiPrimBufferStream* Lock();
    void Unlock(pddiPrimBufferStream* stream);

    unsigned char* LockIndexBuffer();
    void UnlockIndexBuffer(int count);

    bool CheckMemImageVersion(int version);
    void* LockMemImage(unsigned int);
    void UnlockMemImage();
    unsigned GetMemImageLength();
    void SetMemImageParam(unsigned param, unsigned value);

    void SetNumVertices(int count);
    void SetNumIndices(int count);
    void SetIndices(unsigned short* indices, int count);

    void Display();

    void SetUsedSize(int size) {useSize = size;}
    void SetVertexProgram(d3dVertexProgram* d) {vertexProgram = d;}

private:
    friend class d3dPrimBufferStream;

    LPDIRECT3DDEVICE8 d3d;
    D3DPRIMITIVETYPE primitiveType;
    pddiPrimType primTypePDDI;
    unsigned vertexMask;
    int maxVertex;
    int maxIndex;
    int nIndex;
    int useSize;
    LPDIRECT3DINDEXBUFFER8 indices;
    LPDIRECT3DVERTEXBUFFER8 buffer;

    bool lockedVertex;
    bool lockedIndex;
    bool lockedMemImage;
    d3dContext* context;
    d3dVertexProgram* vertexProgram;
};


#endif /* _PRIM_HPP */

