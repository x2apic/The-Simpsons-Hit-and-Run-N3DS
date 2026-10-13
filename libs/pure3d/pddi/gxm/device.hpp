//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _DEVICE_HPP_
#define _DEVICE_HPP_
                
#include <pddi/pddi.hpp>

class gxmWrapper;

class gxmDevice : public pddiDevice
{
public:
    gxmDevice();
    ~gxmDevice();

    void        GetLibraryInfo(pddiLibInfo* info);
    const char* GetDeviceDescription();
    int         GetDisplayInfo(pddiDisplayInfo** info);
    unsigned    GetCaps();

    void SetCurrentContext(pddiRenderContext* context);
    pddiRenderContext* GetCurrentContext();

    pddiDisplay* NewDisplay(int id);
    pddiRenderContext* NewRenderContext(pddiDisplay* display);
    pddiTexture* NewTexture(pddiTextureDesc* desc);
    pddiPrimBuffer* NewPrimBuffer(pddiPrimBufferDesc* desc);
    pddiShader* NewShader(const char* name, const char* aux = NULL);
    
    void AddCustomShader(const char* name, const char* aux = NULL);

    void Release(void);

    static void* vertexUsseAlloc( uint32_t size, SceUID* uid, uint32_t* usseOffset );
    static void  vertexUsseFree( SceUID uid );
    static void* fragmentUsseAlloc( uint32_t size, SceUID* uid, uint32_t* usseOffset );
    static void  fragmentUsseFree( SceUID uid );
protected:
    bool initialized;
    pddiRenderContext* context;

    int nDisplays;
    pddiDisplayInfo* displayInfo;
};
#endif

