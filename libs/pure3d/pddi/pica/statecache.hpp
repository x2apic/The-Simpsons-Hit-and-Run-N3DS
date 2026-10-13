#ifndef PDDI_PICA_STATECACHE_HPP
#define PDDI_PICA_STATECACHE_HPP

#include <3ds.h>
#include <citro3d.h>

struct picaStateCache
{
    bool         alphaTestValid;
    bool         alphaTestEnable;
    GPU_TESTFUNC alphaTestFunc;
    int          alphaTestRef;

    bool              blendValid;
    GPU_BLENDEQUATION blendColorEq, blendAlphaEq;
    GPU_BLENDFACTOR   blendSrcClr, blendDstClr, blendSrcAlpha, blendDstAlpha;

    bool         cullValid;
    GPU_CULLMODE cull;

    bool          depthValid;
    bool          depthEnable;
    GPU_TESTFUNC  depthFunc;
    GPU_WRITEMASK depthMask;

    bool        texValid;
    const C3D_Tex* texPtr;
    const void* texData;
    u32         texParam;
    u16         texWidth, texHeight;
    GPU_TEXCOLOR texFmt;

    int  env0Mode;
    bool passStagesSet;

    bool bufInfoSet;
};

extern picaStateCache g_picaState;

static inline void picaStateInvalidate(void)
{
    g_picaState.alphaTestValid = false;
    g_picaState.blendValid     = false;
    g_picaState.cullValid      = false;
    g_picaState.depthValid     = false;
    g_picaState.texValid       = false;
    g_picaState.env0Mode       = 0;
    g_picaState.passStagesSet  = false;
    g_picaState.bufInfoSet     = false;
}

static inline void picaAlphaTest(bool enable, GPU_TESTFUNC func, int ref)
{
    picaStateCache& s = g_picaState;
    if(s.alphaTestValid && s.alphaTestEnable == enable &&
       s.alphaTestFunc == func && s.alphaTestRef == ref)
        return;

    C3D_AlphaTest(enable, func, ref);
    s.alphaTestValid  = true;
    s.alphaTestEnable = enable;
    s.alphaTestFunc   = func;
    s.alphaTestRef    = ref;
}

static inline void picaAlphaBlend(GPU_BLENDEQUATION colorEq, GPU_BLENDEQUATION alphaEq,
                                  GPU_BLENDFACTOR srcClr, GPU_BLENDFACTOR dstClr,
                                  GPU_BLENDFACTOR srcAlpha, GPU_BLENDFACTOR dstAlpha)
{
    picaStateCache& s = g_picaState;
    if(s.blendValid && s.blendColorEq == colorEq && s.blendAlphaEq == alphaEq &&
       s.blendSrcClr == srcClr && s.blendDstClr == dstClr &&
       s.blendSrcAlpha == srcAlpha && s.blendDstAlpha == dstAlpha)
        return;

    C3D_AlphaBlend(colorEq, alphaEq, srcClr, dstClr, srcAlpha, dstAlpha);
    s.blendValid    = true;
    s.blendColorEq  = colorEq;  s.blendAlphaEq  = alphaEq;
    s.blendSrcClr   = srcClr;   s.blendDstClr   = dstClr;
    s.blendSrcAlpha = srcAlpha; s.blendDstAlpha = dstAlpha;
}

static inline void picaCullFace(GPU_CULLMODE mode)
{
    picaStateCache& s = g_picaState;
    if(s.cullValid && s.cull == mode)
        return;

    C3D_CullFace(mode);
    s.cullValid = true;
    s.cull      = mode;
}

static inline void picaDepthTest(bool enable, GPU_TESTFUNC func, GPU_WRITEMASK mask)
{
    picaStateCache& s = g_picaState;
    if(s.depthValid && s.depthEnable == enable && s.depthFunc == func && s.depthMask == mask)
        return;

    C3D_DepthTest(enable, func, mask);
    s.depthValid  = true;
    s.depthEnable = enable;
    s.depthFunc   = func;
    s.depthMask   = mask;
}

static inline void picaTexBind0(C3D_Tex* tex)
{
    picaStateCache& s = g_picaState;
    if(s.texValid && s.texPtr == tex && s.texData == tex->data && s.texParam == tex->param &&
       s.texWidth == tex->width && s.texHeight == tex->height && s.texFmt == tex->fmt)
        return;

    C3D_TexBind(0, tex);
    s.texValid  = true;
    s.texPtr    = tex;
    s.texData   = tex->data;
    s.texParam  = tex->param;
    s.texWidth  = tex->width;
    s.texHeight = tex->height;
    s.texFmt    = tex->fmt;
}
#endif
