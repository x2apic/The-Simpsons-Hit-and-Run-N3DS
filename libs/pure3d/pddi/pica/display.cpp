#include <pddi/pica/display.hpp>
#include <pddi/pica/context.hpp>
#include <pddi/base/debug.hpp>
#include <stdio.h>
#include "pddi_shbin.h"

extern "C" void C3Di_UpdateContext(void);
extern "C" bool C3Di_SplitFrame(u32** pBuf, u32* pSize);

static const float PICA_STEREO_MAX_SHIFT   = 0.045f;
static const float PICA_STEREO_CONVERGENCE = 7.0f;

static void picaWriteEye(float shift)
{
    C3D_FVec v;
    v.x = shift;
    v.y = PICA_STEREO_CONVERGENCE;
    v.z = 0.0f;
    v.w = 0.0f;
    GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG, 0x80000000u | VSH_FVEC_u_eye);
    GPUCMD_AddWrites(GPUREG_VSH_FLOATUNIFORM_DATA, (u32*)v.c, 4);
}

extern "C" int      g_picaInFrame        = 0;
static int s_c3dRefCount = 0;

extern "C" void (*g_picaYieldHook)(void) = 0;
extern "C" void (*g_picaGpuWaitHook)(int lend) = 0;

#include <SDL.h>

picaDisplay::picaDisplay(pddiDisplayInfo* info)
{
    displayInfo = info;
    mode        = PDDI_DISPLAY_FULLSCREEN;

    winWidth    = PICA_SCREEN_WIDTH;
    winHeight   = PICA_SCREEN_HEIGHT;
    winBitDepth = 32;

    context     = NULL;
    target      = NULL;
    targetRight = NULL;
    targetBottom = NULL;
    stereoThisFrame = false;
    eyeShift    = 0.0f;
    c3dStarted  = false;
    inFrame     = false;
    reset       = true;
    beginTime   = 0.0f;
}

picaDisplay::~picaDisplay()
{
    if(inFrame)
    {
        C3D_FrameEnd(0);
        inFrame = false;
        g_picaInFrame = 0;
    }

    if(target != NULL)
    {
        C3D_RenderTargetDelete(target);
        target = NULL;
    }
    if(targetRight != NULL)
    {
        C3D_RenderTargetDelete(targetRight);
        targetRight = NULL;
    }
    if(targetBottom != NULL)
    {
        C3D_RenderTargetDelete(targetBottom);
        targetBottom = NULL;
    }

    if(c3dStarted)
    {
        c3dStarted = false;
        if(--s_c3dRefCount == 0)
        {
            C3D_Fini();
        }
    }
}

long picaDisplay::ProcessWindowMessage(SDL_Window* wnd, const SDL_WindowEvent* event)
{
    ( void ) wnd;
    ( void ) event;
    return 0;
}

void picaDisplay::SetWindow(SDL_Window* wnd)
{
    ( void ) wnd;
}

bool picaDisplay::InitDisplay(int x, int y, int bpp)
{
    displayInit.xsize = x;
    displayInit.ysize = y;
    displayInit.bpp   = bpp;
    return InitDisplay(&displayInit);
}

bool picaDisplay::InitDisplay(const pddiDisplayInit* init)
{
    displayInit = *init;

    winWidth    = PICA_SCREEN_WIDTH;
    winHeight   = PICA_SCREEN_HEIGHT;
    winBitDepth = 32;

    if(!c3dStarted)
    {
        const size_t cmdBufBytes = 640 * 1024;
        const bool   c3dOk       = ( s_c3dRefCount > 0 )
                                       ? true
                                       : C3D_Init(cmdBufBytes);

        if(!c3dOk)
        {
            return false;
        }

        s_c3dRefCount++;
        c3dStarted = true;
    }

    if(target == NULL)
    {
        target = C3D_RenderTargetCreate(winHeight, winWidth,
                                        GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
        if(target == NULL)
        {
            return false;
        }

        const u32 transferFlags =
            GX_TRANSFER_FLIP_VERT(0) |
            GX_TRANSFER_OUT_TILED(0) |
            GX_TRANSFER_RAW_COPY(0)  |
            GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
            GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
            GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO);
        C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, transferFlags);

        targetRight = C3D_RenderTargetCreate(winHeight, winWidth,
                                             GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
        if(targetRight != NULL)
        {
            C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT, transferFlags);
            gfxSet3D(true);
        }

        targetBottom = C3D_RenderTargetCreate(PICA_BOTTOM_HEIGHT, PICA_BOTTOM_WIDTH,
                                              GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
        if(targetBottom != NULL)
        {
            C3D_RenderTargetSetOutput(targetBottom, GFX_BOTTOM, GFX_LEFT, transferFlags);
        }
    }

    reset = true;
    return true;
}

pddiDisplayInfo* picaDisplay::GetDisplayInfo(void)
{
    return displayInfo;
}

int picaDisplay::GetHeight(void) { return winHeight; }
int picaDisplay::GetWidth(void)  { return winWidth; }
int picaDisplay::GetDepth(void)  { return winBitDepth; }

pddiDisplayMode picaDisplay::GetDisplayMode(void) { return mode; }

int picaDisplay::GetNumColourBuffer(void) { return 2; }

unsigned picaDisplay::GetBufferMask(void) { return unsigned(-1); }

unsigned picaDisplay::GetFreeTextureMem(void)
{
    return (unsigned)linearSpaceFree();
}

void picaDisplay::BeginFrame(void)
{
    if(!inFrame && target != NULL)
    {
        if(g_picaGpuWaitHook) g_picaGpuWaitHook(1);

        const bool frameOk = C3D_FrameBegin(0);

        if(g_picaGpuWaitHook) g_picaGpuWaitHook(0);

        if(!frameOk)
        {
            return;
        }

        if(!C3D_FrameDrawOn(target))
        {
            C3D_FrameEnd(0);
            return;
        }

        const float slider = osGet3DSliderState();
        stereoThisFrame = (targetRight != NULL) && (slider > 0.01f);
        eyeShift = stereoThisFrame ? slider * PICA_STEREO_MAX_SHIFT : 0.0f;

        StartTopSegment();

        inFrame = true;
        g_picaInFrame = 1;
    }
}

void picaDisplay::StartTopSegment(void)
{
    if(stereoThisFrame)
    {
        C3Di_UpdateContext();
        picaWriteEye(-eyeShift);
        C3D_FrameSplit(0);
    }
    else
    {
        picaWriteEye(0.0f);
    }
}

void picaDisplay::FinishTopSegment(void)
{
    if(!stereoThisFrame)
        return;

    u32* sceneBuf = NULL;
    u32  sceneSize = 0;
    if(C3Di_SplitFrame(&sceneBuf, &sceneSize))
    {
        GX_ProcessCommandList(sceneBuf, sceneSize * 4, 0);

        C3D_FrameDrawOn(targetRight);
        C3Di_UpdateContext();
        picaWriteEye(eyeShift);
        C3D_FrameSplit(0);

        GX_ProcessCommandList(sceneBuf, sceneSize * 4, 0);
    }
}

bool picaDisplay::BeginBottomTarget(void)
{
    if(!inFrame || targetBottom == NULL)
        return false;

    FinishTopSegment();

    C3D_RenderTargetClear(targetBottom, C3D_CLEAR_ALL, 0x000000FF, 0x00FFFFFF);
    C3D_FrameDrawOn(targetBottom);
    picaWriteEye(0.0f);
    return true;
}

void picaDisplay::EndBottomTarget(void)
{
    C3D_FrameDrawOn(target);
    StartTopSegment();
}

void picaDisplay::EndFrame(void)
{
    if(inFrame)
    {
        FinishTopSegment();
        stereoThisFrame = false;

        C3D_FrameEnd(0);
        inFrame = false;
    }
}

void picaDisplay::SwapBuffers(void)
{
    EndFrame();
    reset = false;
}

unsigned picaDisplay::Screenshot(pddiColour* buffer, int nBytes)
{
    ( void ) buffer;
    ( void ) nBytes;
    return 0;
}

void picaDisplay::BeginTiming(void)
{
    beginTime = (float)SDL_GetTicks();
}

float picaDisplay::EndTiming(void)
{
    return (float)SDL_GetTicks() - beginTime;
}
