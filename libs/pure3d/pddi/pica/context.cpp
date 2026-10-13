#include <pddi/pica/pica.hpp>
#include <pddi/pica/context.hpp>
#include <pddi/pica/statecache.hpp>
extern "C" void C3Di_UpdateContext(void);
static const u32 PICA_BUFINFO_BASE = 0x18000000;
#include <pddi/pica/device.hpp>
#include <pddi/pica/display.hpp>
#include <pddi/pica/texture.hpp>
#include <pddi/pica/material.hpp>

#include <pddi/base/debug.hpp>
#include <raddebug.hpp>

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "pddi_shbin.h"

extern "C" const unsigned char pddi_pica_shbin[];
extern "C" const unsigned int  pddi_pica_shbin_size;

static const GPU_Primitive_t primTypeTable[5] =
{
    GPU_TRIANGLES,
    GPU_TRIANGLE_STRIP,
    GPU_TRIANGLES,
    GPU_TRIANGLES,
    GPU_TRIANGLES
};

static inline bool PrimIsDrawable(pddiPrimType type)
{
    return (type == PDDI_PRIM_TRIANGLES) || (type == PDDI_PRIM_TRISTRIP);
}

static const GPU_TESTFUNC compTable[8] =
{
    GPU_NEVER,
    GPU_ALWAYS,
    GPU_LESS,
    GPU_LEQUAL,
    GPU_GREATER,
    GPU_GEQUAL,
    GPU_EQUAL,
    GPU_NOTEQUAL
};

static const GPU_CULLMODE cullModeTable[3] =
{
    GPU_CULL_NONE,
    GPU_CULL_FRONT_CCW,
    GPU_CULL_BACK_CCW
};

static const GPU_STENCILOP stencilOpTable[6] =
{
    GPU_STENCIL_KEEP,
    GPU_STENCIL_ZERO,
    GPU_STENCIL_REPLACE,
    GPU_STENCIL_INCR,
    GPU_STENCIL_DECR,
    GPU_STENCIL_INVERT
};

class picaExtGamma : public pddiExtGammaControl
{
public:
    picaExtGamma() { r = g = b = 1.0f; }

    void SetGamma(float ir, float ig, float ib) { r = ir; g = ig; b = ib; }
    void GetGamma(float* outR, float* outG, float* outB) { *outR = r; *outG = g; *outB = b; }

protected:
    float r, g, b;
};

static volatile bool s_picaResumed = false;
static aptHookCookie s_picaAptCookie;

static void picaAptHook(APT_HookType hook, void*)
{
    if(hook == APTHOOK_ONRESTORE || hook == APTHOOK_ONWAKEUP)
    {
        s_picaResumed = true;
    }
}

static picaContext* s_picaContext = NULL;

picaContext::picaContext(picaDevice* dev, picaDisplay* disp)
    : pddiBaseContext((pddiDisplay*)disp, (pddiDevice*)dev)
{
    device  = dev;
    stereoOn = false;
    bottomPass = false;
    s_picaContext = this;
    display = disp;

    device->AddRef();
    display->AddRef();
    disp->SetContext(this);

    contextID = 0;

    defaultShader = NULL;
    clearShader   = NULL;
    matWriteMask  = PDDI_WRITE_ALL;

    extGamma = new picaExtGamma;

    shaderBinary = DVLB_ParseFile((u32*)pddi_pica_shbin, pddi_pica_shbin_size);
    PDDIASSERT(shaderBinary != NULL);
    if(shaderBinary == NULL)
    {
        lastError = PDDI_ERR;
        return;
    }
    shaderProgramInit(&shaderProgram);
    shaderProgramSetVsh(&shaderProgram, &shaderBinary->DVLE[0]);
    C3D_BindProgram(&shaderProgram);

    {
        static bool s_hooked = false;
        if(!s_hooked)
        {
            aptHook(&s_picaAptCookie, picaAptHook, NULL);
            s_hooked = true;
        }
    }

    AttrInfo_Init(&attrInfo);
    AttrInfo_AddLoader(&attrInfo, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(&attrInfo, 1, GPU_FLOAT, 3);
    AttrInfo_AddLoader(&attrInfo, 2, GPU_FLOAT, 2);
    AttrInfo_AddLoader(&attrInfo, 3, GPU_UNSIGNED_BYTE, 4);
    C3D_SetAttrInfo(&attrInfo);

    C3D_DepthMap(true, -1.0f, 0.0f);
    C3D_FragOpMode(GPU_FRAGOPMODE_GL);

    for(unsigned i = 0; i < PICA_IMM_BUFFERS; i++)
    {
        immVertexRing[i] = (unsigned char*)linearAlloc(PICA_IMM_VERTEX_BYTES);
        immIndexRing[i]  = (unsigned short*)linearAlloc(PICA_IMM_INDEX_BYTES);
        PDDIASSERT(immVertexRing[i] && immIndexRing[i]);
    }
    immRing       = 0;
    immVertices   = immVertexRing[0];
    immIndices    = immIndexRing[0];
    immRing       = ( immRing + 1 ) % PICA_IMM_BUFFERS;
    immVertices   = immVertexRing[immRing];
    immIndices    = immIndexRing[immRing];
    immVertexUsed = 0;
    immIndexUsed  = 0;

    memset(lightPos, 0, sizeof(lightPos));
    memset(lightColour, 0, sizeof(lightColour));
    memset(lightAtten, 0, sizeof(lightAtten));
    memset(eyeLight, 0, sizeof(eyeLight));
    for(int i = 0; i < PDDI_MAX_LIGHTS; i++)
        eyeLight[i].z = 1.0f;
    nActiveLights = 0;
    lightsDirty   = true;

    lastMatAmbient.c   = 0;
    lastMatDiffuse.c   = 0;
    lastMatEmissive.c  = 0;
    lastSceneAmbient.c = 0;
    lastLit            = false;
    InvalidateUniformCache();

    fogLutStart = 0.0f;
    fogLutEnd   = 0.0f;
    fogLutValid = false;
    C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);

    Mtx_Identity(&hwProjection);

    wantViewport[0] = hwViewport[0] = 0;
    wantViewport[1] = hwViewport[1] = 0;
    wantViewport[2] = hwViewport[2] = PICA_SCREEN_HEIGHT;
    wantViewport[3] = hwViewport[3] = PICA_SCREEN_WIDTH;

    DefaultState();

    defaultShader = new picaMat(this);
    defaultShader->AddRef();
    clearShader = new picaMat(this);
    clearShader->AddRef();
    clearShader->SetInt(PDDI_SP_COLOURWRITE, PDDI_WRITE_NONE);

    lastError = PDDI_OK;
}

picaContext::~picaContext()
{
    if(defaultShader)
        defaultShader->Release();
    if(clearShader)
        clearShader->Release();

    delete extGamma;

    for(unsigned i = 0; i < PICA_IMM_BUFFERS; i++)
    {
        if(immVertexRing[i]) linearFree(immVertexRing[i]);
        if(immIndexRing[i])  linearFree(immIndexRing[i]);
    }
    immVertices = NULL;
    immIndices  = NULL;

    if(shaderBinary)
    {
        shaderProgramFree(&shaderProgram);
        DVLB_Free(shaderBinary);
        shaderBinary = NULL;
    }

    display->SetContext(NULL);
    display->Release();
    device->Release();
}

picaStateCache g_picaState;

void picaContext::BeginFrame()
{
    pddiBaseContext::BeginFrame();

    picaStateInvalidate();

    display->BeginFrame();

    InvalidateUniformCache();

    immVertexUsed = 0;
    immIndexUsed  = 0;

    hwViewport[0] = 0;
    hwViewport[1] = 0;
    hwViewport[2] = PICA_SCREEN_HEIGHT;
    hwViewport[3] = PICA_SCREEN_WIDTH;

    if(s_picaResumed && !display->HasReset())
    {
        s_picaResumed = false;
        C3D_BindProgram(&shaderProgram);
        C3D_SetAttrInfo(&attrInfo);
        C3D_DepthMap(true, -1.0f, 0.0f);
        C3D_FragOpMode(GPU_FRAGOPMODE_GL);
        pddiBaseShader::ClearCurrentShader();
        SyncState(0xffffffff);
        SetupHardwareProjection();
        ApplyFogState();
    }

    if(display->HasReset())
    {
        s_picaResumed = false;
        contextID++;

        C3D_BindProgram(&shaderProgram);
        C3D_SetAttrInfo(&attrInfo);
        C3D_DepthMap(true, -1.0f, 0.0f);
        C3D_FragOpMode(GPU_FRAGOPMODE_GL);
        pddiBaseShader::ClearCurrentShader();
        SyncState(0xffffffff);
        SetupHardwareProjection();
        ApplyFogState();
    }
}

void picaContext::EndFrame()
{
    pddiBaseContext::EndFrame();

    if(immVertexUsed)
        GSPGPU_FlushDataCache(immVertices, immVertexUsed);
    if(immIndexUsed)
        GSPGPU_FlushDataCache(immIndices, immIndexUsed);

    display->EndFrame();
}

void picaContext::Clear(unsigned bufferMask)
{
    pddiBaseContext::Clear(bufferMask);

    PushState((pddiStateMask)(PDDI_STATE_RENDER | PDDI_STATE_VIEW));
    SetCamera(0.0f, 1.0f, 1.0f, 1.0f);
    SetProjectionMode(PDDI_PROJECTION_DEVICE);
    EnableZBuffer((bufferMask & PDDI_BUFFER_DEPTH) != 0);
    SetZCompare(PDDI_COMPARE_ALWAYS);
    SetZWrite((bufferMask & PDDI_BUFFER_DEPTH) != 0);
    SetCullMode(PDDI_CULL_NONE);

    PushIdentityMatrix(PDDI_MATRIX_MODELVIEW);

    pddiVector a, b, c, d;
    const float w = (float)display->GetWidth();
    const float h = (float)display->GetHeight();
    const float z = state.viewState->clearDepth;
    a.Set(0.0f, 0.0f, z);
    b.Set(w,    0.0f, z);
    c.Set(0.0f, h,    z);
    d.Set(w,    h,    z);

    pddiColour clearColour = state.viewState->clearColour;

    pddiPrimStream* stream = BeginPrims(
        (bufferMask & PDDI_BUFFER_COLOUR) ? defaultShader : clearShader,
        PDDI_PRIM_TRISTRIP, PDDI_V_C, 4);
    if(stream)
    {
        stream->Vertex(&a, clearColour);
        stream->Vertex(&b, clearColour);
        stream->Vertex(&c, clearColour);
        stream->Vertex(&d, clearColour);
        EndPrims(stream);
    }

    PopMatrix(PDDI_MATRIX_MODELVIEW);
    PopState((pddiStateMask)(PDDI_STATE_RENDER | PDDI_STATE_VIEW));
}

void picaContext::ApplyProjection(pddiMatrix* m)
{
    float t[4][4];
    for(int i = 0; i < 4; i++)
    {
        t[i][0] = m->Row4(i).x;
        t[i][1] = m->Row4(i).y;
        t[i][2] = m->Row4(i).z;
        t[i][3] = m->Row4(i).w;
    }

    if(state.viewState->projectionMode == PDDI_PROJECTION_PERSPECTIVE)
    {
        for(int i = 0; i < 4; i++)
            t[i][2] = -t[i][2];
    }
    else
    {
        for(int i = 0; i < 4; i++)
            t[i][2] *= -0.5f;
        t[3][2] -= 0.5f;
    }

    if(bottomPass)
    {
        float wl = 0.0f, wt = 0.0f, wr = 1.0f, wb = 1.0f;
        if(state.viewState->projectionMode != PDDI_PROJECTION_DEVICE)
        {
            wl = state.viewState->viewWindow.left;
            wt = state.viewState->viewWindow.top;
            wr = state.viewState->viewWindow.right;
            wb = state.viewState->viewWindow.bottom;
        }

        const float su = (bottomDst[2] - bottomDst[0]) / (bottomSrc[2] - bottomSrc[0]);
        const float sv = (bottomDst[3] - bottomDst[1]) / (bottomSrc[3] - bottomSrc[1]);

        const float A = (wr - wl) * su;
        const float B = 2.0f * (bottomDst[0] + (wl + 0.5f * (wr - wl) - bottomSrc[0]) * su) - 1.0f;
        const float C = (wb - wt) * sv;
        const float E = 1.0f - 2.0f * (bottomDst[1] + (wt + 0.5f * (wb - wt) - bottomSrc[1]) * sv);

        for(int i = 0; i < 4; i++)
        {
            t[i][0] = A * t[i][0] + B * t[i][3];
            t[i][1] = C * t[i][1] + E * t[i][3];
        }
    }

    for(int i = 0; i < 4; i++)
    {
        const float x = t[i][0];
        t[i][0] = t[i][1];
        t[i][1] = -x;
    }

    for(int row = 0; row < 4; row++)
    {
        hwProjection.r[row].x = t[0][row];
        hwProjection.r[row].y = t[1][row];
        hwProjection.r[row].z = t[2][row];
        hwProjection.r[row].w = t[3][row];
    }

    projUploaded = false;
}

void picaContext::SetupViewport(void)
{
    if(bottomPass)
    {
        wantViewport[0] = 0;
        wantViewport[1] = 0;
        wantViewport[2] = PICA_BOTTOM_HEIGHT;
        wantViewport[3] = PICA_BOTTOM_WIDTH;
        ApplyViewport();
        return;
    }

    const float fw = (float)display->GetWidth();
    const float fh = (float)display->GetHeight();

    int left, top, width, height;

    if(state.viewState->projectionMode == PDDI_PROJECTION_DEVICE)
    {
        left   = 0;
        top    = 0;
        width  = (int)fw;
        height = (int)fh;
    }
    else
    {
        left   = (int)(state.viewState->viewWindow.left * fw);
        top    = (int)(state.viewState->viewWindow.top * fh);
        width  = (int)((state.viewState->viewWindow.right - state.viewState->viewWindow.left) * fw);
        height = (int)((state.viewState->viewWindow.bottom - state.viewState->viewWindow.top) * fh);
    }

    if(width <= 0)  width = 1;
    if(height <= 0) height = 1;

    const int hwX = (int)fh - (top + height);
    const int hwY = (int)fw - (left + width);

    wantViewport[0] = hwX < 0 ? 0 : hwX;
    wantViewport[1] = hwY < 0 ? 0 : hwY;
    wantViewport[2] = height;
    wantViewport[3] = width;

    ApplyViewport();
}

void picaContext::ApplyViewport(void)
{
    if((hwViewport[0] == wantViewport[0]) && (hwViewport[1] == wantViewport[1]) &&
       (hwViewport[2] == wantViewport[2]) && (hwViewport[3] == wantViewport[3]))
    {
        return;
    }

    hwViewport[0] = wantViewport[0];
    hwViewport[1] = wantViewport[1];
    hwViewport[2] = wantViewport[2];
    hwViewport[3] = wantViewport[3];

    C3D_SetViewport((u32)hwViewport[0], (u32)hwViewport[1],
                    (u32)hwViewport[2], (u32)hwViewport[3]);
}

void picaContext::SetupHardwareProjection(void)
{
    pddiMatrix projection;

    switch(state.viewState->projectionMode)
    {
        case PDDI_PROJECTION_DEVICE:
            projection.Identity();
            projection.SetOrthographic(0.0f, (float)display->GetWidth(),
                                       (float)display->GetHeight(), 0.0f,
                                       state.viewState->camera.nearPlane,
                                       state.viewState->camera.farPlane);
            break;

        case PDDI_PROJECTION_ORTHOGRAPHIC:
            projection.Identity();
            projection.SetOrthographic(-0.5f, 0.5f,
                                       -((1.0f / state.viewState->camera.aspect) / 2.0f),
                                        ((1.0f / state.viewState->camera.aspect) / 2.0f),
                                       state.viewState->camera.nearPlane,
                                       state.viewState->camera.farPlane);
            break;

        case PDDI_PROJECTION_PERSPECTIVE:
            projection.Identity();
            projection.SetPerspective(state.viewState->camera.fov,
                                      state.viewState->camera.aspect,
                                      state.viewState->camera.nearPlane,
                                      state.viewState->camera.farPlane);
            break;

        default:
            PDDIASSERTMSG(0, "Bad projection mode", "");
            projection.Identity();
            break;
    }

    stereoOn = (state.viewState->projectionMode == PDDI_PROJECTION_PERSPECTIVE) &&
               (state.viewState->camera.fov < 1.5f);

    ApplyProjection(&projection);
    SetupViewport();
}

void picaContext::LoadHardwareMatrix(pddiMatrixType id)
{
}

void picaContext::SetScissor(pddiRect* rect)
{
    pddiBaseContext::SetScissor(rect);

    int w = display->GetWidth();
    int h = display->GetHeight();

    pddiRect* r = &state.viewState->scissor;

    pddiRect bottomRect;
    if(bottomPass)
    {
        float u0, v0, u1, v1;
        BottomMapPoint((float)r->left / w,  (float)r->top / h,    &u0, &v0);
        BottomMapPoint((float)r->right / w, (float)r->bottom / h, &u1, &v1);
        w = PICA_BOTTOM_WIDTH;
        h = PICA_BOTTOM_HEIGHT;
        bottomRect.left   = (int)(u0 * w);
        bottomRect.top    = (int)(v0 * h);
        bottomRect.right  = (int)(u1 * w + 0.5f);
        bottomRect.bottom = (int)(v1 * h + 0.5f);
        if(bottomRect.left < 0) bottomRect.left = 0;
        if(bottomRect.top < 0) bottomRect.top = 0;
        if(bottomRect.right > w) bottomRect.right = w;
        if(bottomRect.bottom > h) bottomRect.bottom = h;
        r = &bottomRect;
    }

    if((r->left <= 0) && (r->top <= 0) && (r->right >= w) && (r->bottom >= h))
    {
        C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
        return;
    }

    C3D_SetScissor(GPU_SCISSOR_NORMAL,
                   (u32)(h - r->bottom), (u32)(w - r->right),
                   (u32)(h - r->top),    (u32)(w - r->left));
}

unsigned char* picaContext::AllocImmVertices(unsigned count)
{
    const unsigned bytes = count * PICA_VERTEX_STRIDE;
    if((immVertexUsed + bytes) > PICA_IMM_VERTEX_BYTES)
    {
        return NULL;
    }

    unsigned char* p = immVertices + immVertexUsed;
    immVertexUsed += bytes;
    return p;
}

unsigned short* picaContext::AllocImmIndices(unsigned count)
{
    const unsigned bytes = count * sizeof(unsigned short);
    if((immIndexUsed + bytes) > PICA_IMM_INDEX_BYTES)
    {
        return NULL;
    }

    unsigned short* p = immIndices + (immIndexUsed / sizeof(unsigned short));
    immIndexUsed += bytes;
    return p;
}

class picaPrimStream : public pddiPrimStream
{
public:
    pddiPrimBufferStream* stream;

    void Coord(float x, float y, float z)            { stream->Coord(x, y, z); }
    void Normal(float x, float y, float z)           { stream->Normal(x, y, z); }
    void Colour(pddiColour colour, int channel = 0)  { stream->Colour(colour, channel); }
    void UV(float u, float v, int channel = 0)       { stream->UV(u, v, channel); }
    void Specular(pddiColour colour)                 { stream->Specular(colour); }

    void Vertex(pddiVector* v, pddiColour c)                      { stream->Vertex(v, c); }
    void Vertex(pddiVector* v, pddiVector* n)                     { stream->Vertex(v, n); }
    void Vertex(pddiVector* v, pddiVector2* uv)                   { stream->Vertex(v, uv); }
    void Vertex(pddiVector* v, pddiColour c, pddiVector2* uv)     { stream->Vertex(v, c, uv); }
    void Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv)    { stream->Vertex(v, n, uv); }
} thePrimStream;

static picaPrimBuffer* theImmBuffer = NULL;

pddiPrimStream* picaContext::BeginPrims(pddiShader* mat, pddiPrimType primType, unsigned vertexType, int vertexCount, unsigned pass)
{
    if(!mat)
        mat = defaultShader;

    if(vertexCount <= 0)
        vertexCount = PICA_IMM_DEFAULT_VERTICES;

    pddiBaseContext::BeginPrims(mat, primType, vertexType, vertexCount);

    if(theImmBuffer == NULL)
    {
        theImmBuffer = new picaPrimBuffer(this, primType, vertexType, 0, 0);
        theImmBuffer->AddRef();
    }

    unsigned char* verts = AllocImmVertices((unsigned)vertexCount);
    if(verts == NULL)
    {
        return NULL;
    }

    theImmBuffer->SetFormat(primType, vertexType);
    theImmBuffer->SetTransient(verts, NULL, (unsigned)vertexCount);

    thePrimStream.stream = theImmBuffer->Lock();

    pddiBaseShader* material = (pddiBaseShader*)mat;
    ADD_STAT(PDDI_STAT_MATERIAL_OPS, !material->IsCurrent());
    material->SetMaterial();

    return &thePrimStream;
}

void picaContext::EndPrims(pddiPrimStream* stream)
{
    pddiBaseContext::EndPrims(stream);

    if(theImmBuffer == NULL)
        return;

    theImmBuffer->Unlock(thePrimStream.stream);
    theImmBuffer->Display();
}

void picaContext::DrawPrimBuffer(pddiShader* mat, pddiPrimBuffer* buffer)
{
    if(!mat)
        mat = defaultShader;

    pddiBaseShader* material = (pddiBaseShader*)mat;
    ADD_STAT(PDDI_STAT_MATERIAL_OPS, !material->IsCurrent());
    material->SetMaterial();

    ((picaPrimBuffer*)buffer)->Display();
}

int picaContext::GetMaxLights(void)
{
    return PDDI_MAX_LIGHTS;
}

void picaContext::SetAmbientLight(pddiColour col)
{
    pddiBaseContext::SetAmbientLight(col);
}

void picaContext::InvalidateUniformCache(void)
{
    projUploaded   = false;
    lightsUploaded = false;
    matUploaded    = false;
    miscUploaded   = false;
}

void picaContext::SetupHardwareLight(int handle)
{
    if(handle < 0 || handle >= PDDI_MAX_LIGHTS)
        return;

    CaptureEyeLight(handle);
    lightsDirty = true;
}

void picaContext::CaptureEyeLight(int handle)
{
    pddiMatrix* mv = state.matrixStack[PDDI_MATRIX_MODELVIEW]->Top();
    pddiLight*  light = &state.lightingState->light[handle];

    float vx, vy, vz, type;

    if(light->type == PDDI_LIGHT_DIRECTIONAL)
    {
        const float dx = -light->worldDirection.x;
        const float dy = -light->worldDirection.y;
        const float dz = -light->worldDirection.z;

        vx = dx * mv->Row4(0).x + dy * mv->Row4(1).x + dz * mv->Row4(2).x;
        vy = dx * mv->Row4(0).y + dy * mv->Row4(1).y + dz * mv->Row4(2).y;
        vz = dx * mv->Row4(0).z + dy * mv->Row4(1).z + dz * mv->Row4(2).z;

        const float len = sqrtf(vx * vx + vy * vy + vz * vz);
        if(len > 1.0e-6f)
        {
            vx /= len;
            vy /= len;
            vz /= len;
        }
        type = 0.0f;
    }
    else
    {
        const float px = light->worldPosition.x;
        const float py = light->worldPosition.y;
        const float pz = light->worldPosition.z;

        vx = px * mv->Row4(0).x + py * mv->Row4(1).x + pz * mv->Row4(2).x + mv->Row4(3).x;
        vy = px * mv->Row4(0).y + py * mv->Row4(1).y + pz * mv->Row4(2).y + mv->Row4(3).y;
        vz = px * mv->Row4(0).z + py * mv->Row4(1).z + pz * mv->Row4(2).z + mv->Row4(3).z;

        type = 1.0f;
    }

    eyeLight[handle].x = vx;
    eyeLight[handle].y = vy;
    eyeLight[handle].z = vz;
    eyeLight[handle].w = type;
}

void picaContext::RebuildLightArray(void)
{
    nActiveLights = 0;

    for(int i = 0; i < PDDI_MAX_LIGHTS; i++)
    {
        pddiLight* light = &state.lightingState->light[i];
        if(!light->enabled)
            continue;

        const int slot = nActiveLights;

        const float type = eyeLight[i].w;

        lightPos[slot].x = eyeLight[i].x;
        lightPos[slot].y = eyeLight[i].y;
        lightPos[slot].z = eyeLight[i].z;
        lightPos[slot].w = type;

        lightColour[slot].x = (float)light->colour.Red()   / 255.0f;
        lightColour[slot].y = (float)light->colour.Green() / 255.0f;
        lightColour[slot].z = (float)light->colour.Blue()  / 255.0f;
        lightColour[slot].w = 1.0f;

        if(type == 0.0f)
        {
            lightAtten[slot].x = 1.0f;
            lightAtten[slot].y = 0.0f;
            lightAtten[slot].z = 0.0f;
        }
        else
        {
            lightAtten[slot].x = light->attenA;
            lightAtten[slot].y = light->attenB;
            lightAtten[slot].z = light->attenC;

            if((light->attenA == 0.0f) && (light->attenB == 0.0f) && (light->attenC == 0.0f))
                lightAtten[slot].x = 1.0f;
        }
        lightAtten[slot].w = 0.0f;

        nActiveLights++;
    }

    for(int i = nActiveLights; i < PDDI_MAX_LIGHTS; i++)
    {
        lightPos[i].x = 0.0f;
        lightPos[i].y = 0.0f;
        lightPos[i].z = 1.0f;
        lightPos[i].w = 0.0f;

        lightColour[i].x = 0.0f;
        lightColour[i].y = 0.0f;
        lightColour[i].z = 0.0f;
        lightColour[i].w = 0.0f;

        lightAtten[i].x = 1.0f;
        lightAtten[i].y = 0.0f;
        lightAtten[i].z = 0.0f;
        lightAtten[i].w = 0.0f;
    }

    lightsDirty = false;
}

void picaContext::SetVertexUniforms(bool lit, pddiColour ambient, pddiColour diffuse, pddiColour emissive)
{
    if(!g_picaInFrame)
    {
        InvalidateUniformCache();
        return;
    }

    pddiMatrix* mv = state.matrixStack[PDDI_MATRIX_MODELVIEW]->Top();
    C3D_Mtx hwModelView;
    for(int row = 0; row < 4; row++)
    {
        hwModelView.r[row].x = mv->Row4(0)[row];
        hwModelView.r[row].y = mv->Row4(1)[row];
        hwModelView.r[row].z = mv->Row4(2)[row];
        hwModelView.r[row].w = mv->Row4(3)[row];
    }

    if(!projUploaded)
    {
        C3D_FVUnifSet(GPU_VERTEX_SHADER, VSH_FVEC_u_stereoOn,
                      stereoOn ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, VSH_FVEC_u_proj, &hwProjection);
        projUploaded = true;
    }

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, VSH_FVEC_u_mv, &hwModelView);

    if(!miscUploaded || lit != lastLit)
    {
        C3D_FVUnifSet(GPU_VERTEX_SHADER, VSH_FVEC_u_misc,
                      lit ? 1.0f : 0.0f,
                      0.0f,
                      1.0f,
                      1.0f / 255.0f);
        lastLit = lit;
        miscUploaded = true;
    }

    if(!lit)
        return;

    if(lightsDirty)
    {
        RebuildLightArray();
        lightsUploaded = false;
    }

    if(!lightsUploaded)
    {
        C3D_FVec* pos = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, VSH_FVEC_u_lightPos, PDDI_MAX_LIGHTS);
        C3D_FVec* col = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, VSH_FVEC_u_lightColour, PDDI_MAX_LIGHTS);
        C3D_FVec* att = C3D_FVUnifWritePtr(GPU_VERTEX_SHADER, VSH_FVEC_u_lightAtten, PDDI_MAX_LIGHTS);
        for(int i = 0; i < PDDI_MAX_LIGHTS; i++)
        {
            pos[i] = lightPos[i];
            col[i] = lightColour[i];
            att[i] = lightAtten[i];
        }
        lightsUploaded = true;
    }

    if(matUploaded &&
       ambient.c  == lastMatAmbient.c  &&
       diffuse.c  == lastMatDiffuse.c  &&
       emissive.c == lastMatEmissive.c &&
       state.lightingState->ambient.c == lastSceneAmbient.c)
    {
        return;
    }
    lastMatAmbient   = ambient;
    lastMatDiffuse   = diffuse;
    lastMatEmissive  = emissive;
    lastSceneAmbient = state.lightingState->ambient;
    matUploaded      = true;

    C3D_FVUnifSet(GPU_VERTEX_SHADER, VSH_FVEC_u_matAmbient,
                  (float)ambient.Red() / 255.0f,
                  (float)ambient.Green() / 255.0f,
                  (float)ambient.Blue() / 255.0f,
                  1.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, VSH_FVEC_u_matDiffuse,
                  (float)diffuse.Red() / 255.0f,
                  (float)diffuse.Green() / 255.0f,
                  (float)diffuse.Blue() / 255.0f,
                  (float)diffuse.Alpha() / 255.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, VSH_FVEC_u_matEmissive,
                  (float)emissive.Red() / 255.0f,
                  (float)emissive.Green() / 255.0f,
                  (float)emissive.Blue() / 255.0f,
                  0.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, VSH_FVEC_u_sceneAmbient,
                  (float)state.lightingState->ambient.Red() / 255.0f,
                  (float)state.lightingState->ambient.Green() / 255.0f,
                  (float)state.lightingState->ambient.Blue() / 255.0f,
                  1.0f);
}

void picaContext::EnableCulling(bool enable)
{
    picaCullFace(enable ? cullModeTable[state.renderState->cullMode] : GPU_CULL_NONE);
}

void picaContext::SetCullMode(pddiCullMode mode)
{
    pddiBaseContext::SetCullMode(mode);
    picaCullFace(cullModeTable[mode]);
}

void picaContext::SetColourWrite(bool red, bool green, bool blue, bool alpha)
{
    pddiBaseContext::SetColourWrite(red, green, blue, alpha);
    EnableZBuffer(state.renderState->zEnabled);
}

void picaContext::EnableZBuffer(bool enable)
{
    pddiBaseContext::EnableZBuffer(enable);

    unsigned mask = 0;
    if(state.renderState->redWrite   && (matWriteMask & PDDI_WRITE_RED))   mask |= GPU_WRITE_RED;
    if(state.renderState->greenWrite && (matWriteMask & PDDI_WRITE_GREEN)) mask |= GPU_WRITE_GREEN;
    if(state.renderState->blueWrite  && (matWriteMask & PDDI_WRITE_BLUE))  mask |= GPU_WRITE_BLUE;
    if(state.renderState->alphaWrite && (matWriteMask & PDDI_WRITE_ALPHA)) mask |= GPU_WRITE_ALPHA;
    if(enable && state.renderState->zWrite) mask |= GPU_WRITE_DEPTH;

    picaDepthTest(enable, enable ? compTable[state.renderState->zCompare] : GPU_ALWAYS,
                  (GPU_WRITEMASK)mask);
}

void picaContext::SetMaterialWriteMask(int mask)
{
    if(matWriteMask == mask)
        return;
    matWriteMask = mask;
    EnableZBuffer(state.renderState->zEnabled);
}

void picaContext::SetZCompare(pddiCompareMode compareMode)
{
    pddiBaseContext::SetZCompare(compareMode);
    EnableZBuffer(state.renderState->zEnabled);
}

void picaContext::SetZWrite(bool b)
{
    pddiBaseContext::SetZWrite(b);
    EnableZBuffer(state.renderState->zEnabled);
}

void picaContext::SetZBias(float bias)
{
    pddiBaseContext::SetZBias(bias);
}

void picaContext::SetZRange(float n, float f)
{
    pddiBaseContext::SetZRange(n, f);

    C3D_DepthMap(true, -(f - n), n);
}

void picaContext::EnableStencilBuffer(bool enable)
{
    pddiBaseContext::EnableStencilBuffer(enable);
    SetupHardwareStencil();
}

void picaContext::SetupHardwareStencil(void)
{
    if(state.stencilState->enabled)
    {
        C3D_StencilTest(true,
                        compTable[state.stencilState->compare],
                        state.stencilState->ref,
                        (int)state.stencilState->mask,
                        (int)state.stencilState->writeMask);
        C3D_StencilOp(stencilOpTable[state.stencilState->failOp],
                      stencilOpTable[state.stencilState->zFailOp],
                      stencilOpTable[state.stencilState->zPassOp]);
    }
    else
    {
        C3D_StencilTest(false, GPU_ALWAYS, 0, 0xff, 0x00);
    }
}

void picaContext::SetStencilCompare(pddiCompareMode compare)
{
    pddiBaseContext::SetStencilCompare(compare);
    SetupHardwareStencil();
}

void picaContext::SetStencilRef(int ref)
{
    pddiBaseContext::SetStencilRef(ref);
    SetupHardwareStencil();
}

void picaContext::SetStencilMask(unsigned mask)
{
    pddiBaseContext::SetStencilMask(mask);
    SetupHardwareStencil();
}

void picaContext::SetStencilWriteMask(unsigned mask)
{
    pddiBaseContext::SetStencilWriteMask(mask);
    SetupHardwareStencil();
}

void picaContext::SetStencilOp(pddiStencilOp failOp, pddiStencilOp zFailOp, pddiStencilOp zPassOp)
{
    pddiBaseContext::SetStencilOp(failOp, zFailOp, zPassOp);
    SetupHardwareStencil();
}

void picaContext::SetFillMode(pddiFillMode mode)
{
    pddiBaseContext::SetFillMode(mode);
}

void picaContext::ApplyFogState(void)
{
    if(!state.fogState->enabled)
    {
        C3D_FogGasMode(GPU_NO_FOG, GPU_PLAIN_DENSITY, false);
        return;
    }

    const float start = state.fogState->start;
    const float end   = state.fogState->end;

    if(!fogLutValid || (start != fogLutStart) || (end != fogLutEnd))
    {
        const float n = state.viewState->camera.nearPlane;
        const float f = state.viewState->camera.farPlane;

        float data[256];
        const float range = (end > start) ? (end - start) : 1.0f;

        for(int i = 0; i < 256; i++)
        {
            const float depth = (float)i / 255.0f;
            const float z = FogLut_CalcZ(depth, n, f);

            float density = (z - start) / range;
            if(density < 0.0f) density = 0.0f;
            if(density > 1.0f) density = 1.0f;

            data[i] = density;
        }

        FogLut_FromArray(&fogLut, data);
        fogLutStart = start;
        fogLutEnd   = end;
        fogLutValid = true;
    }

    C3D_FogColor(((unsigned)state.fogState->colour.Blue() << 16) |
                 ((unsigned)state.fogState->colour.Green() << 8) |
                  (unsigned)state.fogState->colour.Red());
    C3D_FogLutBind(&fogLut);
    C3D_FogGasMode(GPU_FOG, GPU_PLAIN_DENSITY, false);
}

void picaContext::EnableFog(bool enable)
{
    pddiBaseContext::EnableFog(enable);
    ApplyFogState();
}

void picaContext::SetFog(pddiColour colour, float start, float end)
{
    pddiBaseContext::SetFog(colour, start, end);
    ApplyFogState();
}

int picaContext::GetMaxTextureDimension(void)
{
    return PICA_MAX_TEXTURE_DIMENSION;
}

pddiExtension* picaContext::GetExtension(unsigned extID)
{
    switch(extID)
    {
        case PDDI_EXT_GAMMACONTROL:
            return extGamma;
    }

    return pddiBaseContext::GetExtension(extID);
}

bool picaContext::VerifyExtension(unsigned extID)
{
    switch(extID)
    {
        case PDDI_EXT_GAMMACONTROL:
            return true;
    }

    return pddiBaseContext::VerifyExtension(extID);
}

void picaContext::BeginTiming(void)
{
    display->BeginTiming();
}

float picaContext::EndTiming(void)
{
    return display->EndTiming();
}

class picaPrimBufferStream : public pddiPrimBufferStream
{
public:
    picaPrimBuffer* buffer;

    picaPrimBufferStream(picaPrimBuffer* b) { buffer = b; }

    void Next(void)
    {
        if(buffer->coord)
            buffer->coord = (float*)((char*)buffer->coord + PICA_VERTEX_STRIDE);
        if(buffer->normal)
            buffer->normal = (float*)((char*)buffer->normal + PICA_VERTEX_STRIDE);
        if(buffer->uv)
            buffer->uv = (float*)((char*)buffer->uv + PICA_VERTEX_STRIDE);
        if(buffer->colour)
            buffer->colour += PICA_VERTEX_STRIDE;

        buffer->total++;
        PDDIASSERT(buffer->total <= buffer->allocated);
    }

    void Position(float x, float y, float z)
    {
        if(buffer->coord && (buffer->total < buffer->allocated))
        {
            buffer->coord[0] = x;
            buffer->coord[1] = y;
            buffer->coord[2] = z;
        }
        Next();
    }

    void Normal(float x, float y, float z)
    {
        if(!buffer->normal || (buffer->total >= buffer->allocated)) return;
        buffer->normal[0] = x;
        buffer->normal[1] = y;
        buffer->normal[2] = z;
    }

    void Colour(pddiColour colour, int channel = 0)
    {
        if(!buffer->colour || (buffer->total >= buffer->allocated)) return;
        buffer->colour[0] = colour.Red();
        buffer->colour[1] = colour.Green();
        buffer->colour[2] = colour.Blue();
        buffer->colour[3] = colour.Alpha();
    }

    void TexCoord1(float u, int channel = 0) {}

    void TexCoord2(float u, float v, int channel = 0)
    {
        if(!buffer->uv || (channel != 0) || (buffer->total >= buffer->allocated)) return;
        buffer->uv[0] = u;
        buffer->uv[1] = v;
    }

    void TexCoord3(float u, float v, float s, int channel = 0) {}
    void TexCoord4(float u, float v, float s, float t, int channel = 0) {}

    void Specular(pddiColour colour) {}
    void SkinIndices(unsigned, unsigned, unsigned, unsigned) {}
    void SkinWeights(float, float, float) {}

    void Vertex(pddiVector* v, pddiColour c)
    {
        Colour(c);
        Position(v->x, v->y, v->z);
    }

    void Vertex(pddiVector* v, pddiVector* n)
    {
        Normal(n->x, n->y, n->z);
        Position(v->x, v->y, v->z);
    }

    void Vertex(pddiVector* v, pddiVector2* uv)
    {
        TexCoord2(uv->x, uv->y);
        Position(v->x, v->y, v->z);
    }

    void Vertex(pddiVector* v, pddiColour c, pddiVector2* uv)
    {
        Colour(c);
        TexCoord2(uv->x, uv->y);
        Position(v->x, v->y, v->z);
    }

    void Vertex(pddiVector* v, pddiVector* n, pddiVector2* uv)
    {
        Normal(n->x, n->y, n->z);
        TexCoord2(uv->x, uv->y);
        Position(v->x, v->y, v->z);
    }
};

picaPrimBuffer::picaPrimBuffer(picaContext* c, pddiPrimType type, unsigned vertexFormat, int nVertex, int nIndex)
    : context(c)
{
    stream = new picaPrimBufferStream(this);

    total = allocated = 0;
    coord = normal = uv = NULL;
    colour = NULL;
    buffer = NULL;
    indices = NULL;
    indexCount = 0;
    expanded = NULL;
    expandedBytes = 0;
    expandDirty = true; elemState = -1;
    mem = 0;
    transient = false;

    primType = type;
    vertexType = vertexFormat;

    if(nVertex <= 0)
    {
        transient = true;
        return;
    }

    allocated = (unsigned)nVertex;

    indexCount = (nIndex > 0) ? (unsigned)nIndex : allocated;

    const unsigned vertexBytes = PICA_VERTEX_STRIDE * allocated;
    const unsigned indexBytes  = ((indexCount * (unsigned)sizeof(unsigned short)) + 3u) & ~3u;

    mem = vertexBytes + indexBytes;
    buffer = (unsigned char*)linearAlloc(mem);
    if(buffer == NULL)
    {
        lastError = PDDI_ERR;
        allocated = 0;
        indexCount = 0;
        return;
    }

    indices = (unsigned short*)(buffer + vertexBytes);

    if(nIndex == 0)
    {
        for(unsigned i = 0; i < indexCount; i++)
            indices[i] = (unsigned short)i;
    }

    context->ADD_STAT(PDDI_STAT_BUFFERED_COUNT, 1);
    context->ADD_STAT(PDDI_STAT_BUFFERED_ALLOC, mem / 1024.0f);
}

picaPrimBuffer::~picaPrimBuffer()
{
    delete stream;

    if(!transient)
    {
        if(buffer)
            linearFree(buffer);
        if(expanded)
        {
            linearFree(expanded);
        }

        context->ADD_STAT(PDDI_STAT_BUFFERED_COUNT, -1);
        context->ADD_STAT(PDDI_STAT_BUFFERED_ALLOC, -(float)mem / 1024.0f);
    }
}

void picaPrimBuffer::SetFormat(pddiPrimType type, unsigned vertexFormat)
{
    primType   = type;
    vertexType = vertexFormat;
}

void picaPrimBuffer::SetTransient(unsigned char* verts, unsigned short* idx, unsigned count)
{
    PDDIASSERT(transient);
    buffer     = verts;
    indices    = idx;
    allocated  = count;
    indexCount = idx ? count : 0;
    total      = 0;
}

pddiPrimBufferStream* picaPrimBuffer::Lock()
{
    total = 0;

    if(buffer == NULL)
        return stream;

    unsigned char* ptr = buffer;
    coord  = (float*)(ptr + PICA_VERTEX_OFF_POS);
    normal = (vertexType & PDDI_V_NORMAL) ? (float*)(ptr + PICA_VERTEX_OFF_NRM) : NULL;
    uv     = (vertexType & 0xf)           ? (float*)(ptr + PICA_VERTEX_OFF_UV)  : NULL;
    colour = (vertexType & PDDI_V_COLOUR) ? (ptr + PICA_VERTEX_OFF_COL)         : NULL;

    for(unsigned i = 0; i < allocated; i++)
    {
        unsigned char* v = buffer + (i * PICA_VERTEX_STRIDE);
        if(!normal)
        {
            float* n = (float*)(v + PICA_VERTEX_OFF_NRM);
            n[0] = 0.0f; n[1] = 0.0f; n[2] = 1.0f;
        }
        if(!uv)
        {
            float* t = (float*)(v + PICA_VERTEX_OFF_UV);
            t[0] = 0.0f; t[1] = 0.0f;
        }
        if(!colour)
        {
            unsigned char* c = v + PICA_VERTEX_OFF_COL;
            c[0] = 255; c[1] = 255; c[2] = 255; c[3] = 255;
        }
    }

    return stream;
}

static inline void picaFlushForGpu(const void* p, unsigned bytes)
{
    if(p != NULL && bytes != 0)
        GSPGPU_FlushDataCache(p, bytes);
}

void picaPrimBuffer::Unlock(pddiPrimBufferStream*)
{
    if(coord)
        coord = (float*)((char*)coord - (total * PICA_VERTEX_STRIDE));
    if(normal)
        normal = (float*)((char*)normal - (total * PICA_VERTEX_STRIDE));
    if(uv)
        uv = (float*)((char*)uv - (total * PICA_VERTEX_STRIDE));
    if(colour)
        colour -= (total * PICA_VERTEX_STRIDE);

    if(!transient)
    {
        picaFlushForGpu(buffer, mem);
        expandDirty = true; elemState = -1;
    }
}

unsigned char* picaPrimBuffer::LockIndexBuffer()
{
    return (unsigned char*)indices;
}

void picaPrimBuffer::UnlockIndexBuffer(int count)
{
    if(count > 0)
        indexCount = (unsigned)count;

    if(!transient)
    {
        picaFlushForGpu(indices, indexCount * (unsigned)sizeof(unsigned short));
        expandDirty = true; elemState = -1;
    }
}

void picaPrimBuffer::SetIndices(unsigned short* i, int count)
{
    if(!indices)
        return;
    PDDIASSERT(count <= (int)indexCount);
    if(count > (int)indexCount)
        count = (int)indexCount;
    memcpy(indices, i, count * sizeof(unsigned short));

    if(!transient)
    {
        picaFlushForGpu(indices, count * (unsigned)sizeof(unsigned short));
        expandDirty = true; elemState = -1;
    }
}

bool picaPrimBuffer::IndicesInRange(void) const
{
    if(indices == NULL || indexCount == 0 || allocated == 0)
        return false;
    for(unsigned i = 0; i < indexCount; i++)
    {
        if(indices[i] >= allocated)
            return false;
    }
    return true;
}

bool picaPrimBuffer::BuildExpanded(void)
{
    if(!expandDirty && expanded != NULL)
        return true;

    const unsigned needed = indexCount * PICA_VERTEX_STRIDE;
    if(needed == 0)
        return false;

    if(expanded != NULL && expandedBytes < needed)
    {
        linearFree(expanded);
        expanded = NULL;
        expandedBytes = 0;
    }

    if(expanded == NULL)
    {
        expanded = (unsigned char*)linearAlloc(needed);
        if(expanded == NULL)
            return false;
        expandedBytes = needed;
    }

    for(unsigned i = 0; i < indexCount; i++)
    {
        unsigned v = indices[i];
        if(v >= allocated)
            v = 0;
        memcpy(expanded + (i * PICA_VERTEX_STRIDE),
               buffer + (v * PICA_VERTEX_STRIDE),
               PICA_VERTEX_STRIDE);
    }

    picaFlushForGpu(expanded, needed);
    expandDirty = false;
    return true;
}

void picaPrimBuffer::Display(void)
{
    if(!g_picaInFrame)
    {
        return;
    }

    if(!buffer || !PrimIsDrawable(primType))
    {
        return;
    }

    const unsigned char* vertexData = buffer;
    unsigned count;

    bool useElements = false;
    if(indices != NULL && !transient)
    {
        if(elemState < 0)
            elemState = IndicesInRange() ? 1 : 0;
        useElements = (elemState == 1);
    }

    if(useElements)
    {
        count = indexCount;
    }
    else if(indices != NULL)
    {
        if(!BuildExpanded())
        {
            return;
        }
        vertexData = expanded;
        count = indexCount;
    }
    else
    {
        count = total ? total : allocated;
    }

    if(count == 0)
        return;

    context->ApplyViewport();

    {
        u32* cmdBuf    = NULL;
        u32  cmdSize   = 0;
        u32  cmdOffset = 0;
        GPUCMD_GetBuffer(&cmdBuf, &cmdSize, &cmdOffset);

        const u32 headroom = 2048;

        if(cmdBuf == NULL || (cmdOffset + headroom) > cmdSize)
        {
            return;
        }
    }

    if(!g_picaState.bufInfoSet)
    {
        C3D_BufInfo bufInfo;
        BufInfo_Init(&bufInfo);
        BufInfo_Add(&bufInfo, vertexData, PICA_VERTEX_STRIDE, 4, 0x3210);
        C3D_SetBufInfo(&bufInfo);
        C3Di_UpdateContext();
        g_picaState.bufInfoSet = true;
    }
    GPUCMD_AddWrite(GPUREG_ATTRIBBUFFER0_OFFSET,
                    osConvertVirtToPhys(vertexData) - PICA_BUFINFO_BASE);

    if(useElements)
        C3D_DrawElements(primTypeTable[primType], (int)count, C3D_UNSIGNED_SHORT, indices);
    else
        C3D_DrawArrays(primTypeTable[primType], 0, (int)count);
}

void picaContext::BottomMapPoint(float u, float v, float* bu, float* bv) const
{
    *bu = bottomDst[0] + (u - bottomSrc[0]) * (bottomDst[2] - bottomDst[0]) / (bottomSrc[2] - bottomSrc[0]);
    *bv = bottomDst[1] + (v - bottomSrc[1]) * (bottomDst[3] - bottomDst[1]) / (bottomSrc[3] - bottomSrc[1]);
}

bool picaContext::BeginBottomScreen(float u0, float v0, float u1, float v1)
{
    if(bottomPass || (u1 <= u0) || (v1 <= v0) || !display->BeginBottomTarget())
        return false;

    bottomSrc[0] = u0; bottomSrc[1] = v0; bottomSrc[2] = u1; bottomSrc[3] = v1;

    const float srcW = (u1 - u0) * 640.0f;
    const float srcH = (v1 - v0) * 480.0f;
    float k = (float)PICA_BOTTOM_WIDTH / srcW;
    if((float)PICA_BOTTOM_HEIGHT / srcH < k)
        k = (float)PICA_BOTTOM_HEIGHT / srcH;
    const float dw = srcW * k / (float)PICA_BOTTOM_WIDTH;
    const float dh = srcH * k / (float)PICA_BOTTOM_HEIGHT;
    bottomDst[0] = 0.5f - 0.5f * dw;
    bottomDst[1] = 0.5f - 0.5f * dh;
    bottomDst[2] = 0.5f + 0.5f * dw;
    bottomDst[3] = 0.5f + 0.5f * dh;

    bottomPass = true;

    picaStateInvalidate();
    InvalidateUniformCache();
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
    hwViewport[0] = hwViewport[1] = hwViewport[2] = hwViewport[3] = -1;
    SetupHardwareProjection();
    return true;
}

void picaContext::EndBottomScreen(void)
{
    if(!bottomPass)
        return;

    bottomPass = false;
    display->EndBottomTarget();

    picaStateInvalidate();
    InvalidateUniformCache();
    hwViewport[0] = 0;
    hwViewport[1] = 0;
    hwViewport[2] = PICA_SCREEN_HEIGHT;
    hwViewport[3] = PICA_SCREEN_WIDTH;
    SetupHardwareProjection();
    SetScissor(&state.viewState->scissor);
    ApplyFogState();
}

extern "C" bool picaBottomScreenBegin(float u0, float v0, float u1, float v1)
{
    return (s_picaContext != NULL) && s_picaContext->BeginBottomScreen(u0, v0, u1, v1);
}

extern "C" void picaBottomScreenEnd(void)
{
    if(s_picaContext != NULL)
        s_picaContext->EndBottomScreen();
}
