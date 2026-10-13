/*===========================================================================
    $Workfile:: device.cpp           $
    $Revision:: 59                   $
    $Author:: Neall                  $
    26-Apr-96 Created by Neall

    Copyright (c)1996-2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include "device.hpp"
#include "display.hpp"
#include "context.hpp"
#include "texture.hpp"
#include "prim.hpp"
#include "util.hpp"
#include "shaders/shadersetup.hpp"
#include "../base/debug.hpp"


char DEVICE_DESCRIPTION[] = "Direct3D 8";

static d3dDevice* static_d3d = NULL;

// initialisation function 
__declspec(dllexport) int pddiCreate(int versionMajor, int versionMinor, pddiDevice** device)
{
    if((versionMajor != PDDI_VERSION_MAJOR) || (versionMinor != PDDI_VERSION_MINOR))
    {
        *device = 0;
        return PDDI_VERSION_ERROR;
    }

    if(!static_d3d)
    {
        static_d3d = new d3dDevice(Direct3DCreate8(D3D_SDK_VERSION));
    }
    *device = static_d3d;
    return PDDI_OK;
}

d3dDevice::d3dDevice(LPDIRECT3D8 d3d)
{
    direct3D = d3d;

    context = NULL;

    nDisplay = 0;
    displayInfo = NULL;

    d3dShaderSetup(direct3D);
}

d3dDevice::~d3dDevice()
{
    for(int i=0; i < nDisplay; i++)
    {
        delete[] displayInfo[i].modeInfo;
    }
    delete[] displayInfo;
    displayInfo = NULL;

    direct3D->Release();

    static_d3d = NULL;
}

void d3dDevice::GetLibraryInfo(pddiLibInfo* info)
{
    info->libID = PDDI_LIBID_DIRECT3D;
    info->versionMajor = PDDI_VERSION_MAJOR;
    info->versionMinor = PDDI_VERSION_MINOR;
    info->versionBuild = 0;
    strcpy(info->description, DEVICE_DESCRIPTION);
}

int d3dDevice::GetDisplayInfo(pddiDisplayInfo** info)
{
    if(!displayInfo)
    {
        int totalDisplay = direct3D->GetAdapterCount();
        displayInfo = new pddiDisplayInfo[totalDisplay];

        nDisplay = 0;
        for(int i = 0; i < totalDisplay; i++)
        {
            D3DADAPTER_IDENTIFIER8 identity;
            D3DCAPS8 devCaps;

            direct3D->GetAdapterIdentifier(i, D3DENUM_NO_WHQL_LEVEL , &identity);
            if(direct3D->GetDeviceCaps(i, D3DDEVTYPE_HAL, &devCaps) != D3D_OK)
                continue;

            displayInfo[nDisplay].id = i;;
            strcpy(displayInfo[nDisplay].description, identity.Description);
            displayInfo[nDisplay].pci = identity.DeviceId;
            displayInfo[nDisplay].vendor = identity.VendorId;
            displayInfo[nDisplay].fullscreenOnly = 0;
            displayInfo[nDisplay].caps = 0;

            int totalModes = direct3D->GetAdapterModeCount(i);
            displayInfo[nDisplay].modeInfo = new pddiModeInfo[totalModes];
            int usefulModes = 0;

            for(int j = 0; j < totalModes; j++)
            {
                D3DDISPLAYMODE mode;
                direct3D->EnumAdapterModes(i, j, &mode);
                displayInfo[nDisplay].modeInfo[usefulModes].width  = mode.Width;
                displayInfo[nDisplay].modeInfo[usefulModes].height = mode.Height;
                    
                displayInfo[nDisplay].modeInfo[usefulModes].bpp = DisplayBitDepth(mode.Format);

                if(displayInfo[nDisplay].modeInfo[usefulModes].bpp != 0)
                    usefulModes++;
            }
            displayInfo[nDisplay].nDisplayModes = usefulModes;
            nDisplay++;
        }
    }

    *info = displayInfo;
    return nDisplay;
}

void d3dDevice::SetCurrentContext(pddiRenderContext* c)
{
    context = c;
}

pddiRenderContext* d3dDevice::GetCurrentContext()
{
    return context;
}

pddiDisplay* d3dDevice::NewDisplay(int id)
{
    pddiDisplayInfo *dummy;
    GetDisplayInfo(&dummy);  // make sure we've enumerated displays

    PDDIASSERT((id >= 0) && (id < nDisplay));

    PDDIASSERT(direct3D);
    return new d3dDisplay(&displayInfo[id], direct3D);
}

pddiRenderContext* d3dDevice::NewRenderContext(pddiDisplay* display)
{
    d3dContext* context = new d3dContext((d3dDisplay*)display, this);
    bool goodness = context->Init();
    PDDIASSERT(goodness);
    return context;
}

pddiTexture* d3dDevice::NewTexture(pddiTextureDesc* desc)
{
    d3dTexture* tex = new d3dTexture((d3dContext*)context);
    if(desc->GetVolume())
    {
        if(!tex->CreateVolume( desc->GetSizeX(), desc->GetSizeY(), desc->GetSizeZ(), 
                                desc->GetBitDepth(), desc->GetAlphaDepth(), desc->GetMipMapCount(),desc->GetType(),desc->GetUsage()))
        {
            lastError = tex->GetLastError();
            delete tex;
            return NULL;
        }
    }
    else
    {
        if(!tex->Create(desc->GetSizeX(), desc->GetSizeY(), desc->GetBitDepth(), 
                         desc->GetAlphaDepth(), desc->GetMipMapCount(),desc->GetType(),desc->GetUsage()))
        {
            lastError = tex->GetLastError();
            delete tex;
            return NULL;
        }
    }
    return tex;
}

pddiPrimBuffer* d3dDevice::NewPrimBuffer(pddiPrimBufferDesc* desc)
{
    d3dPrimBuffer* buffer = new d3dPrimBuffer((d3dContext*)context);
    buffer->Create(desc->GetPrimType(), desc->GetVertexFormat(), 0, desc->GetVertexCount(), desc->GetIndexCount(),
        desc->GetVertexProgram());

    lastError = buffer->GetLastError();
    if(lastError != PDDI_OK)
    {
        delete buffer;
        return NULL;
    }
    return buffer;
}

pddiShader* d3dDevice::NewShader(const char* name, const char* aux)
{
    return pddiBaseShader::AllocateShader(context, name, NULL);
}

void d3dDevice::AddCustomShader(const char* name, const char* aux)
{
}

