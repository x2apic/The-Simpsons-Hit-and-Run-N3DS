#ifndef PDDI_PICA_DISPLAY_HPP
#define PDDI_PICA_DISPLAY_HPP

#include <pddi/pica/pica.hpp>

struct SDL_Window;

class picaDisplay : public pddiDisplay
{
public:
    picaDisplay(pddiDisplayInfo* info);
    ~picaDisplay();

    long ProcessWindowMessage(SDL_Window* wnd, const SDL_WindowEvent* event);
    void SetWindow(SDL_Window* wnd);

    pddiDisplayInfo* GetDisplayInfo(void);

    bool InitDisplay(const pddiDisplayInit*);
    bool InitDisplay(int x, int y, int bpp);

    int GetHeight(void);
    int GetWidth(void);
    int GetDepth(void);
    pddiDisplayMode GetDisplayMode(void);
    int GetNumColourBuffer(void);
    unsigned GetBufferMask(void);

    unsigned GetFreeTextureMem(void);

    void SwapBuffers(void);

    unsigned Screenshot(pddiColour* buffer, int nBytes);

    void  SetContext(picaContext* c) { context = c; }
    bool  HasReset(void) { return reset; }

    void  BeginTiming(void);
    float EndTiming(void);

    C3D_RenderTarget* GetTarget(void) { return target; }

    void BeginFrame(void);

    bool BeginBottomTarget(void);
    void EndBottomTarget(void);
    void EndFrame(void);

private:
    pddiDisplayMode  mode;
    pddiDisplayInfo* displayInfo;
    pddiDisplayInit  displayInit;

    picaContext* context;

    int winWidth;
    int winHeight;
    int winBitDepth;

    C3D_RenderTarget* target;
    C3D_RenderTarget* targetRight;
    C3D_RenderTarget* targetBottom;
    bool  stereoThisFrame;

    void StartTopSegment(void);
    void FinishTopSegment(void);
    float eyeShift;
    bool  c3dStarted;
    bool  inFrame;
    bool  reset;

    float beginTime;
};
#endif
