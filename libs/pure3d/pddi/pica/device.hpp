#ifndef PDDI_PICA_DEVICE_HPP
#define PDDI_PICA_DEVICE_HPP

#include <pddi/pica/pica.hpp>

class picaDevice : public pddiDevice
{
public:
    picaDevice();
    ~picaDevice();

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

protected:
    pddiRenderContext* context;

    int nDisplays;
    pddiDisplayInfo* displayInfo;
    pddiModeInfo     modeInfo;
};
#endif
