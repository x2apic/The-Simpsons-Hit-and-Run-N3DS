//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _DISPLAY_HPP_
#define _DISPLAY_HPP_
#include <pddi/pddi.hpp>

#include <vector>

class gxmContext;
class gxmWrapper;
struct SDL_mutex;

class gxmDisplay : public pddiDisplay
{
public:
    gxmDisplay(pddiDisplayInfo* info);
    ~gxmDisplay();

    // cross-platform functions
    bool InitDisplay(const pddiDisplayInit*);
    bool InitDisplay(int x, int y, int bpp);

    pddiDisplayInfo* GetDisplayInfo(void);

    int GetHeight(void);
    int GetWidth(void);
    int GetDepth(void);
    pddiDisplayMode GetDisplayMode(void);
    int GetNumColourBuffer(void);
    unsigned GetBufferMask(void);

    unsigned GetFreeTextureMem(void);

    void SwapBuffers(void);

    unsigned Screenshot(pddiColour* buffer, int nBytes);

    // Win32 specific functions
    long  ProcessWindowMessage(SDL_Window* wnd, const SDL_WindowEvent* event);
    void  SetWindow(SDL_Window* wnd);

    // GXM specific functions

    SceGxmMultisampleMode GetMSAAMode(void);
    SceGxmContext* GetGXMContext(void) { return gxm; }
    const SceGxmRenderTarget* GetRenderTarget(void) { return renderTarget; }
    const SceGxmDepthStencilSurface* GetDepthSurface(void) { return &depthSurface; }
    const SceGxmColorSurface* GetColorSurface(void) { return &displaySurface[backBufferIndex]; }
    SceGxmSyncObject* GetFragementSyncObj(void) { return displayBufferSync[backBufferIndex]; }

    // internal functions
    
    void BeginTiming();
    float EndTiming();

    void SetContext(gxmContext* c) {context = c;}
    bool ExtBGRA(void) { return extBGRA;}
    bool CheckExtension(const char*);
    bool HasReset(void) { return reset; }

    void SetGamma(float r, float g, float b);
    void GetGamma(float* r, float* g, float* b);

protected:
    // Data structure to pass through the display queue
    typedef struct DisplayData
    {
        void* address;				///< Framebuffer address
        uint32_t width;				///< Framebuffer width
        uint32_t height;			///< Framebuffer height
        uint32_t strideInPixels;	///< Framebuffer stride in pixels
        uint32_t flipMode;			///< From #FlipMode
    } DisplayData;

    static void displayCallback( const void* callbackData );

private:
    pddiDisplayMode mode;
    int winWidth;
    int winHeight;
    int winBitDepth;
    SceGxmMultisampleMode msaaMode;

    pddiDisplayInit displayInit;
    pddiDisplayInfo* displayInfo;
    uint32_t backBufferIndex = 0;
    uint32_t frontBufferIndex = 0;

    gxmContext* context;

    unsigned short initialGammaRamp[3][256];
    float gammaR,gammaG,gammaB;
    SceGxmContextParams contextParams;

    SDL_Window* win;
    SceGxmContext* gxm;
    SceGxmRenderTarget* renderTarget;

    void* depthBufferData;
    SceGxmDepthStencilSurface depthSurface;

    std::vector<void*> displayBufferData;
    std::vector<SceGxmColorSurface> displaySurface;
    std::vector<SceGxmSyncObject*> displayBufferSync;

    bool extBGRA;
    bool reset;

    float beginTime;

    void* vdmRingBuffer;
    void* vertexRingBuffer;
    void* fragmentRingBuffer;
    SceUID fragmentUsseRingBufferUid;
};

#endif
