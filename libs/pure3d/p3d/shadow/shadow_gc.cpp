//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

/*


#include <p3d/shadow.hpp>

#include <radmath/radmath.hpp>
#include <p3d/shader.hpp>
#include <p3d/utility.hpp>
#include <p3d/vertexlist.hpp>
#include <p3d/primgroup.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/light.hpp>
#include <p3d/anim/polyskin.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/view.hpp>
#include <p3d/camera.hpp>
#include <p3d/chunkfile.hpp>
#include <constants/chunkids.hpp>

#include <pddi/pddiext.hpp>
#include <pddi/gamecube/gctex.hpp>

#include <pddi/gamecube/gcbufcopy.hpp>

//#define CARMACK_REVERSE // use inverted z-buffer mode 
//#define DRAW_CAPS // draw caps on volumes

tShadowGeneratorImpl::tShadowGeneratorImpl()
{
    // create shader for the volumes
    volumeShader = new tShader();
    volumeShader->AddRef();
    volumeShader->SetInt(PDDI_SP_SHADEMODE, PDDI_SHADE_FLAT);
    volumeShader->SetInt(PDDI_SP_ALPHATEST , false);
    volumeShader->SetInt(PDDI_SP_BLENDMODE , PDDI_BLEND_NONE);

    // create the GC wash shader
    bufferCopier       = (gcExtBufferCopy *)p3d::pddi->GetExtension(PDDI_EXT_BUFCOPY);
    shadowColourBuffer = bufferCopier->CreateAlphaTexture(p3d::display->GetWidth(), p3d::display->GetHeight(), 8);

    bufferCopier->CopyAlphaBuf(shadowColourBuffer, false, 8, 8);

    washShader = new tShader("shadow");
    washShader->AddRef();
    washShader->GetShader()->SetTexture(PDDI_SP_SHADOWTEXTURE, shadowColourBuffer);
    washShader->SetColour(PDDI_SP_SHADOWCOLOUR, tColour(0, 0, 0, 64));
    washColour.Set(0, 0, 0, 127); 
}

tShadowGeneratorImpl::~tShadowGeneratorImpl()
{
}

void tShadowGeneratorImpl::SetWashColour(tColour colour)
{
    washColour = colour;
}

void tShadowGeneratorImpl::Begin()
{
    pddiRenderContext* pddi = p3d::pddi;

    pddi->SetZWrite(false);

    //Save some rendering states
    oldCull = p3d::pddi->GetCullMode();

    pddi->Clear(PDDI_BUFFER_STENCIL);
    
    // disable writes to colour buffer
    pddi->SetColourWrite(false, false, false, true); 

    #ifdef CARMACK_REVERSE
        pddi->SetZCompare(PDDI_COMPARE_GREATEREQUAL);
    #endif

    s_volumeShader = volumeShader;
}


void tShadowGeneratorImpl::End()
{
    s_volumeShader = NULL;

    pddiRenderContext* pddi = p3d::pddi;

    // restore the old cull mode
    p3d::pddi->SetCullMode(oldCull);

    washShader->SetColour(PDDI_SP_SHADOWCOLOUR, washColour);

    float width, height;
    width = (float)p3d::display->GetWidth();
    height = (float)p3d::display->GetHeight();

    //Simpsons HAHAHAHAHAHAHACK
    ((gcTexture*)shadowColourBuffer)->SetHeight( height );
    ((gcTexture*)shadowColourBuffer)->SetWidth( width );

    bufferCopier->CopyAlphaBuf(shadowColourBuffer, false, 8, 8);
    pddi->SetColourWrite(true, true, true, true); // enable writes to colour buffer

    pddi->SetProjectionMode(PDDI_PROJECTION_DEVICE);

    p3d::stack->PushIdentity();
    pddiPrimStream *stream = pddi->BeginPrims(washShader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_T, 6);

    float near, far, fov, aspect;
    pddi->GetCamera(&near, &far, &fov, &aspect);
    near += 0.01F;
        
    // tri 1
    stream->UV(0.0F, 1.0F);    stream->Coord(0.0F,  height, near);
    stream->UV(0.0F, 0.0F);    stream->Coord(0.0F,  0.0F,   near);
    stream->UV(1.0F, 0.0F);    stream->Coord(width, 0.0F,   near);

    // tri 2
    stream->UV(0.0F, 1.0F);    stream->Coord(0.0F,  height, near);
    stream->UV(1.0F, 0.0F);    stream->Coord(width, 0.0F,   near);
    stream->UV(1.0F, 1.0F);    stream->Coord(width, height, near);

    pddi->EndPrims(stream);

    p3d::stack->Pop();

    // Restore the render state
     pddi->SetProjectionMode(PDDI_PROJECTION_PERSPECTIVE);
//    pddi->EnableZBuffer(true);
    pddi->SetZWrite(true);
    pddi->SetZCompare(PDDI_COMPARE_LESSEQUAL);

}

void tShadowGeneratorImpl::PreRender()
{
    // disable alpha writes
   p3d::pddi->SetColourWrite(true, true, true, false);
}
*/