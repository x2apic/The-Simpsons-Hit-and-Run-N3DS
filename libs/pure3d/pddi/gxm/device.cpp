//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <pddi/gxm/gxm.hpp>
#include <pddi/gxm/device.hpp>
#include <pddi/gxm/display.hpp>
#include <pddi/gxm/context.hpp>
#include <pddi/gxm/texture.hpp>
#include <pddi/gxm/material.hpp>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <io.h>
#include <pddi/base/debug.hpp>
#include <SDL.h>

#define PDDI_GXM_BUILD 36

// Helper macro to align a value
#define ALIGN(x, a)					(((x) + ((a) - 1)) & ~((a) - 1))

static gxmDevice gblDevice;

char libName [] = "GXM";

#ifdef PDDI_USE_ASSERTS
#include <raddebug.hpp>
const char* pddiGxmErrorString(unsigned int err)
{
    switch(err)
    {
#define GXM_ERROR(x) case x: return #x
        GXM_ERROR(SCE_GXM_ERROR_UNINITIALIZED);
        GXM_ERROR(SCE_GXM_ERROR_ALREADY_INITIALIZED);
        GXM_ERROR(SCE_GXM_ERROR_OUT_OF_MEMORY);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_VALUE);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_POINTER);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_ALIGNMENT);
        GXM_ERROR(SCE_GXM_ERROR_NOT_WITHIN_SCENE);
        GXM_ERROR(SCE_GXM_ERROR_WITHIN_SCENE);
        GXM_ERROR(SCE_GXM_ERROR_NULL_PROGRAM);
        GXM_ERROR(SCE_GXM_ERROR_UNSUPPORTED);
        GXM_ERROR(SCE_GXM_ERROR_PATCHER_INTERNAL);
        GXM_ERROR(SCE_GXM_ERROR_RESERVE_FAILED);
        GXM_ERROR(SCE_GXM_ERROR_PROGRAM_IN_USE);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_INDEX_COUNT);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_POLYGON_MODE);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_PRECISION);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_SAMPLER_RESULT_TYPE_COMPONENT_COUNT);
        GXM_ERROR(SCE_GXM_ERROR_UNIFORM_BUFFER_NOT_RESERVED);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_AUXILIARY_SURFACE);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_PRECOMPUTED_DRAW);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_PRECOMPUTED_VERTEX_STATE);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_PRECOMPUTED_FRAGMENT_STATE);
        GXM_ERROR(SCE_GXM_ERROR_DRIVER);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_TEXTURE);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_TEXTURE_DATA_POINTER);
        GXM_ERROR(SCE_GXM_ERROR_INVALID_TEXTURE_PALETTE_POINTER);
#undef  GXM_ERROR
    }
    rDebugPrintf("Unknown error: (0x%08x)", err);
    return "Unknown error";
}

bool pddiGxmAssert(const char* file, int line, unsigned int err, const char* cond)
{
    if(err != SCE_OK && pddiAssertFailed(file, line, cond, pddiGxmErrorString(err), "GXM"))
        pddiBreak();
    return err == SCE_OK;
}

#endif

int pddiCreate(int versionMajor, int versionMinor, pddiDevice** device)
{
    if((versionMajor != PDDI_VERSION_MAJOR) || (versionMinor != PDDI_VERSION_MINOR))
    {
        *device = NULL;
        return PDDI_VERSION_ERROR;
    }

    *device = &gblDevice;
    return PDDI_OK;
}

//--------------------------------------------------------------
gxmDevice::gxmDevice() 
{
    nDisplays = 0;
    displayInfo = NULL;
}

//--------------------------------------------------------------
gxmDevice::~gxmDevice()
{
    for( int i = 0; i < nDisplays; i++ )
    {
        delete[] displayInfo[i].modeInfo;
    }
    delete[] displayInfo;
    displayInfo = NULL;
}

//--------------------------------------------------------------
void gxmDevice::GetLibraryInfo(pddiLibInfo* info)
{
    info->versionMajor = PDDI_VERSION_MAJOR;
    info->versionMinor = PDDI_VERSION_MINOR;
    info->versionBuild = PDDI_GXM_BUILD;
    info->libID = PDDI_LIBID_VITA;
    strcpy( info->description, libName );
}

unsigned gxmDevice::GetCaps()
{
    return 0;
}

int gxmDevice::GetDisplayInfo(pddiDisplayInfo** info)
{
    *info = displayInfo;

    if (displayInfo)
    {
        return nDisplays;
    }

#if SDL_MAJOR_VERSION < 3
    int totalDisplay = SDL_GetNumVideoDisplays();
#else
    int totalDisplay;
    SDL_DisplayID* displayIds = SDL_GetDisplays(&totalDisplay);
#endif
    displayInfo = new pddiDisplayInfo[totalDisplay];

    nDisplays = 0;
    for(int i = 0; i < totalDisplay; i++)
    {
#if SDL_MAJOR_VERSION < 3
        SDL_DisplayMode devMode;
        const char* displayName = SDL_GetDisplayName(i);
        int totalModes = SDL_GetNumDisplayModes(i);
        if (!displayName || totalModes <= 0)
            continue;
#else
        const char* displayName = SDL_GetDisplayName(displayIds[i]);
        int totalModes;
        SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(displayIds[i], &totalModes);
        if (!displayName || !modes)
            continue;
#endif

        displayInfo[nDisplays].id = i;
        strcpy(displayInfo[0].description, displayName);
        displayInfo[nDisplays].pci = 0;
        displayInfo[nDisplays].vendor = 0;
        displayInfo[nDisplays].fullscreenOnly = false;
        displayInfo[nDisplays].caps = 0;

        int nModes = 0;
        pddiModeInfo* displayModes = new pddiModeInfo[totalModes];
#if SDL_MAJOR_VERSION < 3
        for(int j = 0; j < totalModes; j++)
        {
            if(SDL_GetDisplayMode(j, j, &devMode) == 0)
            {
                displayModes[nModes].width = devMode.w;
                displayModes[nModes].height = devMode.h;
                displayModes[nModes].bpp = 32;
                nModes++;
            }
        }
#else
        for(int j = 0; j < totalModes; j++)
        {
            displayModes[nModes].width = modes[j]->w;
            displayModes[nModes].height = modes[j]->h;
            displayModes[nModes].bpp = 32;
            nModes++;
        }
        SDL_free(modes);
#endif

        displayInfo[nDisplays].modeInfo = displayModes;
        displayInfo[nDisplays].nDisplayModes = nModes;
        nDisplays++;
    }
#if SDL_MAJOR_VERSION > 2
    SDL_free(displayIds);
#endif

    return nDisplays;
}

const char* gxmDevice::GetDeviceDescription()
{
    return libName;
}

void gxmDevice::SetCurrentContext(pddiRenderContext* c)
{
    context = c;
}

pddiRenderContext* gxmDevice::GetCurrentContext(void)
{
    return context;
}

pddiDisplay *gxmDevice::NewDisplay(int id)
{
    pddiDisplayInfo* dummy;
    GetDisplayInfo(&dummy);

    PDDIASSERT(id < nDisplays);
    gxmDisplay* display = new gxmDisplay(&displayInfo[id]);

    if(display->GetLastError() != PDDI_OK)
    {
        delete display;
        return NULL;
    }

    return (pddiDisplay *)display;
}
//--------------------------------------------------------------
pddiRenderContext *gxmDevice::NewRenderContext(pddiDisplay* display)
{
    gxmContext* context = new gxmContext(this, (gxmDisplay*)display);

    if(context->GetLastError() != PDDI_OK)
    {
        delete context;
        return NULL;
    }
    return context;
}

//--------------------------------------------------------------
pddiTexture* gxmDevice::NewTexture(pddiTextureDesc* desc)
{
    gxmTexture* tex = new gxmTexture((gxmContext*)context);
    if(!tex->Create(desc->GetSizeX(), desc->GetSizeY(), desc->GetBitDepth(), 
                     desc->GetAlphaDepth(), desc->GetMipMapCount(),desc->GetType(),desc->GetUsage()))
    {
        lastError = tex->GetLastError();
        delete tex;
        return NULL;
    }
    return tex;
}
//--------------------------------------------------------------
pddiShader *gxmDevice::NewShader(const char* name, const char*) 
{ 
    gxmMat* mat= new gxmMat((gxmContext*)context);
    if(mat->GetLastError() != PDDI_OK) {
        delete mat;
        return NULL;
    }

    return mat;
}

pddiPrimBuffer *gxmDevice::NewPrimBuffer(pddiPrimBufferDesc* desc) 
{ 
    return new gxmPrimBuffer((gxmContext*)context, desc->GetPrimType(), desc->GetVertexFormat(), desc->GetVertexCount(), desc->GetIndexCount());;
}

void gxmDevice::AddCustomShader(const char* name, const char* aux)
{
}

void gxmDevice::Release(void)
{
}

void* gxmDevice::vertexUsseAlloc(uint32_t size, SceUID* uid, uint32_t* usseOffset)
{
    int err = SCE_OK;

    // align to memblock alignment for LPDDR
    size = ALIGN(size, 4096);

    // allocate some memory
    *uid = sceKernelAllocMemBlock("vertexUsse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
    PDDIASSERT(*uid >= SCE_OK);

    // grab the base address
    void* mem = NULL;
    err = sceKernelGetMemBlockBase(*uid, &mem);
    PDDIASSERT(err == SCE_OK);

    // map as vertex USSE code for the GPU
    err = sceGxmMapVertexUsseMemory(mem, size, usseOffset);
    PDDIASSERT(err == SCE_OK);

    // done
    return mem;
}

void gxmDevice::vertexUsseFree(SceUID uid)
{
    int err = SCE_OK;

    // grab the base address
    void* mem = NULL;
    err = sceKernelGetMemBlockBase(uid, &mem);
    PDDIASSERT(err == SCE_OK);

    // unmap memory
    err = sceGxmUnmapVertexUsseMemory(mem);
    PDDIASSERT(err == SCE_OK);

    // free the memory block
    err = sceKernelFreeMemBlock(uid);
    PDDIASSERT(err == SCE_OK);
}

void* gxmDevice::fragmentUsseAlloc(uint32_t size, SceUID* uid, uint32_t* usseOffset)
{
    int err = SCE_OK;

    // align to memblock alignment for LPDDR
    size = ALIGN(size, 4096);

    // allocate some memory
    *uid = sceKernelAllocMemBlock("fragmentUsse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
    PDDIASSERT(*uid >= SCE_OK);

    // grab the base address
    void* mem = NULL;
    err = sceKernelGetMemBlockBase(*uid, &mem);
    PDDIASSERT(err == SCE_OK);

    // map as fragment USSE code for the GPU
    err = sceGxmMapFragmentUsseMemory(mem, size, usseOffset);
    PDDIASSERT(err == SCE_OK);

    // done
    return mem;
}

void gxmDevice::fragmentUsseFree(SceUID uid)
{
    int err = SCE_OK;

    // grab the base address
    void* mem = NULL;
    err = sceKernelGetMemBlockBase(uid, &mem);
    PDDIASSERT(err == SCE_OK);

    // unmap memory
    err = sceGxmUnmapFragmentUsseMemory(mem);
    PDDIASSERT(err == SCE_OK);

    // free the memory block
    err = sceKernelFreeMemBlock(uid);
    PDDIASSERT(err == SCE_OK);
}
