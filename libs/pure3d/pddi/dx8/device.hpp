/*===========================================================================
    $Workfile:: device.hpp           $
    $Revision:: 9                    $
    $Author:: Neall                  $
    26-Apr-96 Created by Neall

    Copyright (c)1995, 1996, 1997, 1998, 1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _DEVICE_HPP
#define _DEVICE_HPP

#include "../pddi.hpp"
#include "direct3d.hpp"

struct ddDisplayInfo;

class d3dDevice : public pddiDevice
{
public:
    d3dDevice(LPDIRECT3D8);
    virtual ~d3dDevice();

    virtual int GetDisplayInfo(pddiDisplayInfo** info);
    virtual void GetLibraryInfo(pddiLibInfo* info);

    PDDI_INTERFACE void SetCurrentContext(pddiRenderContext* context);
    PDDI_INTERFACE pddiRenderContext* GetCurrentContext();

    PDDI_INTERFACE pddiDisplay* NewDisplay(int id);
    PDDI_INTERFACE pddiRenderContext* NewRenderContext(pddiDisplay* display);
    PDDI_INTERFACE pddiTexture* NewTexture(pddiTextureDesc* desc);
    PDDI_INTERFACE pddiPrimBuffer* NewPrimBuffer(pddiPrimBufferDesc* desc);
    PDDI_INTERFACE pddiShader* NewShader(const char* name, const char* aux = NULL);

    virtual void AddCustomShader(const char* name, const char* aux);
private:
    pddiRenderContext* context;

    int nDisplay;
    pddiDisplayInfo* displayInfo;

    LPDIRECT3D8 direct3D;
};

#endif // _DEVICE_HPP
