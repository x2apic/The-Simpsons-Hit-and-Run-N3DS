#include <pddi/pica/pica.hpp>
#include <pddi/pica/device.hpp>
#include <pddi/pica/display.hpp>
#include <pddi/pica/context.hpp>
#include <pddi/pica/texture.hpp>
#include <pddi/pica/material.hpp>

#include <pddi/base/debug.hpp>

#include <string.h>

#define PDDI_PICA_BUILD 1

static picaDevice gblDevice;

static char libName[] = "PICA200";

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

picaDevice::picaDevice()
{
    context     = NULL;
    nDisplays   = 0;
    displayInfo = NULL;
}

picaDevice::~picaDevice()
{
    delete[] displayInfo;
    displayInfo = NULL;
}

void picaDevice::GetLibraryInfo(pddiLibInfo* info)
{
    info->versionMajor = PDDI_VERSION_MAJOR;
    info->versionMinor = PDDI_VERSION_MINOR;
    info->versionBuild = PDDI_PICA_BUILD;
    info->libID = PDDI_LIBID_STUB;
    strcpy(info->description, libName);
}

unsigned picaDevice::GetCaps()
{
    return 0;
}

int picaDevice::GetDisplayInfo(pddiDisplayInfo** info)
{
    if(displayInfo == NULL)
    {
        displayInfo = new pddiDisplayInfo[1];

        modeInfo.width  = PICA_SCREEN_WIDTH;
        modeInfo.height = PICA_SCREEN_HEIGHT;
        modeInfo.bpp    = 32;

        displayInfo[0].id = 0;
        strcpy(displayInfo[0].description, "Nintendo 3DS top screen");
        displayInfo[0].pci = 0;
        displayInfo[0].vendor = 0;
        displayInfo[0].fullscreenOnly = true;
        displayInfo[0].caps = 0;
        displayInfo[0].nDisplayModes = 1;
        displayInfo[0].modeInfo = &modeInfo;

        nDisplays = 1;
    }

    *info = displayInfo;
    return nDisplays;
}

const char* picaDevice::GetDeviceDescription()
{
    return libName;
}

void picaDevice::SetCurrentContext(pddiRenderContext* c)
{
    context = c;
}

pddiRenderContext* picaDevice::GetCurrentContext(void)
{
    return context;
}

pddiDisplay* picaDevice::NewDisplay(int id)
{
    pddiDisplayInfo* dummy;
    GetDisplayInfo(&dummy);

    PDDIASSERT(id < nDisplays);
    if(id >= nDisplays)
        id = 0;

    picaDisplay* display = new picaDisplay(&displayInfo[id]);

    if(display->GetLastError() != PDDI_OK)
    {
        delete display;
        return NULL;
    }

    return (pddiDisplay*)display;
}

pddiRenderContext* picaDevice::NewRenderContext(pddiDisplay* display)
{
    picaContext* ctx = new picaContext(this, (picaDisplay*)display);

    if(ctx->GetLastError() != PDDI_OK)
    {
        delete ctx;
        return NULL;
    }
    return ctx;
}

pddiTexture* picaDevice::NewTexture(pddiTextureDesc* desc)
{
    picaTexture* tex = new picaTexture((picaContext*)context);
    if(!tex->Create(desc->GetSizeX(), desc->GetSizeY(), desc->GetBitDepth(),
                    desc->GetAlphaDepth(), desc->GetMipMapCount(),
                    desc->GetType(), desc->GetUsage()))
    {
        lastError = tex->GetLastError();
        delete tex;
        return NULL;
    }
    return tex;
}

pddiShader* picaDevice::NewShader(const char* name, const char*)
{
    picaMat* mat = new picaMat((picaContext*)context);
    if(mat->GetLastError() != PDDI_OK)
    {
        delete mat;
        return NULL;
    }
    return mat;
}

pddiPrimBuffer* picaDevice::NewPrimBuffer(pddiPrimBufferDesc* desc)
{
    return new picaPrimBuffer((picaContext*)context, desc->GetPrimType(),
                              desc->GetVertexFormat(), desc->GetVertexCount(),
                              desc->GetIndexCount());
}

void picaDevice::AddCustomShader(const char* name, const char* aux)
{
}

void picaDevice::Release(void)
{
}
