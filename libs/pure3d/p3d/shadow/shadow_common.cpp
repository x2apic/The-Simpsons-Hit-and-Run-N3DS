//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================
/*

#include <p3d/shadow.hpp>

//-----------------------------------------------------------------------------
// shadow.cpp
//
// Stencil bufffer shadow rendering
//
//-----------------------------------------------------------------------------

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

#ifndef RAD_PS2
#ifdef RAD_GAMECUBE
    static const tColour s_VolumeColour(0, 0, 0, 4);
#else
    static const tColour s_VolumeColour(255,0,0,0);
#endif
#else
    static const tColour s_VolumeColour(0x8,0x8,0x8,0 );
#endif

#define BUFFER_RENDER_SHADOW

static const unsigned MAX_EDGES     = 16384;
static const unsigned MAX_CAP_POLYS = 16384;
static const unsigned MAX_EDGE_IN_BUFFER = 200;
static const unsigned MAX_SHADOW_BUFFER = MAX_EDGE_IN_BUFFER*6;      //a shadow buffer for displaying maximum 100 shadow volume edges

tPrimGroupOptimised * tShadowMeshImpl::staticShadowBuffer = NULL;

using namespace RadicalMathLibrary;

struct DrawEdge
{
    short a;
    short b;
} s_DrawEdges[MAX_EDGES];

unsigned s_NumDrawEdges;

struct DrawCapPolys
{
    short a;
    short b;
    short c;
} s_DrawCapPolys[MAX_CAP_POLYS];
unsigned s_NumDrawCapPolys;

tShader* tShadowGeneratorImplCommon::s_volumeShader = NULL;

static int VertsToPrims(pddiPrimType type, int verts)
{
    switch(type)
    {
        case PDDI_PRIM_TRIANGLES:
            return verts/3;
        case PDDI_PRIM_TRISTRIP:
            return verts - 2;
        case PDDI_PRIM_LINES:
            return verts / 2;
        case PDDI_PRIM_LINESTRIP:
            return verts - 1;
        case PDDI_PRIM_POINTS:
            return verts;
        default:
            P3DASSERT(0);
            return 0;
    }
    return 0;
}


enum ShadowVolPass
{
    PASS_FRONT,
    PASS_BACK
};

static inline void SetupPass(ShadowVolPass pass, tShader* shader)
{
#ifdef RAD_PS2
    p3d::pddi->SetCullMode( (pass == PASS_FRONT) ? PDDI_CULL_SHADOW_BACKFACE : PDDI_CULL_SHADOW_FRONTFACE );
    shader->SetInt( PDDI_SP_BLENDMODE, (pass == PASS_FRONT) ? PDDI_BLEND_ADD : PDDI_BLEND_SUBTRACT);

#elif defined RAD_GAMECUBE
    
    p3d::pddi->SetCullMode( (pass == PASS_FRONT) ? PDDI_CULL_NORMAL : PDDI_CULL_INVERTED );
    shader->SetInt( PDDI_SP_BLENDMODE, (pass == PASS_FRONT) ? PDDI_BLEND_ADD : PDDI_BLEND_SUBTRACT);

#else
    #ifdef CARMACK_REVERSE
    p3d::pddi->SetCullMode((pass == PASS_FRONT) ? PDDI_CULL_INVERTED :  PDDI_CULL_NORMAL);
    #else
    p3d::pddi->SetCullMode((pass == PASS_FRONT) ? PDDI_CULL_NORMAL : PDDI_CULL_INVERTED );
    #endif

    p3d::pddi->SetStencilOp(PDDI_STENCIL_KEEP, PDDI_STENCIL_KEEP, (pass == PASS_FRONT) ? PDDI_STENCIL_INCR : PDDI_STENCIL_DECR);
#endif

}

tShadowGeneratorImplCommon::tShadowGeneratorImplCommon() :
    washShader(NULL),
    volumeShader(NULL)
{
}

tShadowGeneratorImplCommon::~tShadowGeneratorImplCommon()
{
    tRefCounted::Release(washShader);
    tRefCounted::Release(volumeShader);
}


//=============================================================================
// tShadowMeshImpl
//=============================================================================

tShadowMeshImpl::tShadowMeshImpl() :
    shadowBuffer(NULL),
    topology(NULL),
    vertices(NULL),
    normals(NULL),
    triCulled(NULL),
    volumeLength(0.0f),
    setVolumeLength(0.0f)
{
    if( !staticShadowBuffer )
    {
        CreateShadowBuffer( );
    }
    tRefCounted::Assign(shadowBuffer,staticShadowBuffer);
}

tShadowMeshImpl::~tShadowMeshImpl()
{
    delete[] topology;
    delete[] vertices;
    delete[] normals;
    delete[] triCulled;

    tRefCounted::Release(shadowBuffer);
}

void tShadowMeshImpl::CreateShadowBuffer()
{
    if( !staticShadowBuffer )
    {
        staticShadowBuffer = new tPrimGroupOptimised( MAX_SHADOW_BUFFER );
        staticShadowBuffer->AddRef( );

        pddiPrimBufferDesc desc( PDDI_PRIM_TRIANGLES, PDDI_V_C, MAX_SHADOW_BUFFER, 0 );
        desc.SetMatrixCount( 0 );
        pddiPrimBuffer *primBuffer  = p3d::device->NewPrimBuffer(&desc);

        staticShadowBuffer->SetBuffer( primBuffer );
    }
}

void tShadowMeshImpl::ReleaseShadowBuffer( )
{
    tRefCounted::Release(staticShadowBuffer);
}

void tShadowMeshImpl::Display()
{
    P3DASSERT(tShadowGeneratorImplCommon::GetCurVolumeShader());

//    timeA = radTimeGetMicroseconds();
    // transform light to object space
    Matrix w2o = *p3d::context->GetWorldMatrix();
    w2o.Invert();
    if(isPointLight)
    {
        w2o.Transform(worldLight, &light);
    }
    else
    {
        w2o.RotateVector(worldLight, &light);
    }

    GuessVolumeLength();

    //Update the shadow edge
    UpdateEdges();
#ifdef BUFFER_RENDER_SHADOW

    DrawShadowVolume();

    // draw the front facing pass
    SetupPass(PASS_FRONT, tShadowGeneratorImplCommon::GetCurVolumeShader());
    DrawCaps();

    // draw the inverted pass
    SetupPass(PASS_BACK, tShadowGeneratorImplCommon::GetCurVolumeShader());
    DrawCaps();


#else
    // draw the front facing pass
    SetupPass(PASS_FRONT, tShadowGeneratorImplCommon::GetCurVolumeShader());
    DrawShadowVolume();
    DrawCaps();

    // draw the inverted pass
    SetupPass(PASS_BACK, tShadowGeneratorImplCommon::GetCurVolumeShader());
    DrawShadowVolume();
    DrawCaps();
#endif
    // Reset the volume length to be recalculated next
    // frame.
    volumeLength = 0.0f;
}


void tShadowMeshImpl::UpdateEdges()
{
    P3DASSERT(vertices);
    P3DASSERT(normals);
    unsigned short i;

    //Update back face culling information (from pov of light source)
    if(!isPointLight)
    {
        for(i=0;i<numTriangles;i++)
        {
            triCulled[i] = (normals[i].DotProduct(light) < 0);
        }
    }
    else
    {
        Vector pos = light;
        for(i=0;i<numTriangles;i++)
        {
            Vector cull;
            cull.Sub(pos, vertices[topology[i].v0]);
            triCulled[i] = (normals[i].DotProduct(cull) > 0);
        }
    }

    //Compute edge list
    s_NumDrawCapPolys = 0;
    s_NumDrawEdges = 0;
    for(i=0;i<numTriangles;i++)
    {
        if(!triCulled[i])
        {
            P3DASSERT(s_NumDrawCapPolys < MAX_CAP_POLYS);
            s_DrawCapPolys[s_NumDrawCapPolys].a = topology[i].v0;
            s_DrawCapPolys[s_NumDrawCapPolys].b = topology[i].v1;
            s_DrawCapPolys[s_NumDrawCapPolys].c = topology[i].v2;

            s_NumDrawCapPolys++;

            if((topology[i].n0 == (unsigned short)-1) ||
               (triCulled[topology[i].n0]))
            {
                P3DASSERT(s_NumDrawEdges < MAX_EDGES);
                s_DrawEdges[s_NumDrawEdges].b = topology[i].v0;
                s_DrawEdges[s_NumDrawEdges].a = topology[i].v1;
                s_NumDrawEdges++;
            }
            if((topology[i].n1 == (unsigned short)-1) ||
               (triCulled[topology[i].n1]))
            {
                P3DASSERT(s_NumDrawEdges < MAX_EDGES);
                s_DrawEdges[s_NumDrawEdges].b = topology[i].v1;
                s_DrawEdges[s_NumDrawEdges].a = topology[i].v2;
                s_NumDrawEdges++;
            }
            if((topology[i].n2 == (unsigned short)-1) ||
               (triCulled[topology[i].n2]))
            {
                P3DASSERT(s_NumDrawEdges < MAX_EDGES);
                s_DrawEdges[s_NumDrawEdges].b = topology[i].v2;
                s_DrawEdges[s_NumDrawEdges].a = topology[i].v0;
                s_NumDrawEdges++;
            }
        }
    }

}

// render the caps on the front and rear of the shadow volume
void tShadowMeshImpl::DrawCaps()
{
#ifdef DRAW_CAPS
    if(!s_NumDrawCapPolys)
    {
        return;
    }
    
    P3DASSERT(tShadowGeneratorImplCommon::GetCurVolumeShader());

    unsigned short i;
    Vector shadow = light;
    shadow.Scale(volumeLength);

    pddiPrimStream* stream = p3d::pddi->BeginPrims(tShadowGeneratorImplCommon::GetCurVolumeShader()->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_C, 6 * s_NumDrawCapPolys);

    if(!isPointLight)
    {
        for(i=0; i<s_NumDrawCapPolys;i++)
        {
            Vector a, b, c, d, e, f;
            a = vertices[s_DrawCapPolys[i].a];
            b = vertices[s_DrawCapPolys[i].b];
            c = vertices[s_DrawCapPolys[i].c];

            // draw front cap poly (winding order is reversed)
            stream->Colour(s_VolumeColour); stream->Coord(c.x, c.y, c.z);
            stream->Colour(s_VolumeColour); stream->Coord(b.x, b.y, b.z);
            stream->Colour(s_VolumeColour); stream->Coord(a.x, a.y, a.z);

            // calculate rear cap position
            a.Add(shadow);
            b.Add(shadow);
            c.Add(shadow);

            // draw rear cap
            stream->Colour(s_VolumeColour); stream->Coord(a.x, a.y, a.z);
            stream->Colour(s_VolumeColour); stream->Coord(b.x, b.y, b.z);
            stream->Colour(s_VolumeColour); stream->Coord(c.x, c.y, c.z);
        }
    }
    else
    {
        for(i=0; i<s_NumDrawCapPolys;i++)
        {
            Vector a, b, c, d, e, f;
            a = vertices[s_DrawCapPolys[i].a];
            b = vertices[s_DrawCapPolys[i].b];
            c = vertices[s_DrawCapPolys[i].c];

            // draw front cap poly (winding order is reversed)
            stream->Colour(s_VolumeColour); stream->Coord(c.x, c.y, c.z);
            stream->Colour(s_VolumeColour); stream->Coord(b.x, b.y, b.z);
            stream->Colour(s_VolumeColour); stream->Coord(a.x, a.y, a.z);

            // calculate rear cap position
            d.Sub(a, light);
            e.Sub(b, light);
            f.Sub(c, light);
            d.Normalize();
            e.Normalize();
            f.Normalize();
            d.Scale(volumeLength);
            e.Scale(volumeLength);
            f.Scale(volumeLength);
            a.Add(d);
            b.Add(e);
            c.Add(f);

            // draw rear cap
            stream->Colour(s_VolumeColour); stream->Coord(a.x, a.y, a.z);
            stream->Colour(s_VolumeColour); stream->Coord(b.x, b.y, b.z);
            stream->Colour(s_VolumeColour); stream->Coord(c.x, c.y, c.z);
        }
    }
    p3d::pddi->EndPrims(stream);
#endif
}


//Render the shadow volume 
void tShadowMeshImpl::DrawShadowVolume()
{
#ifdef BUFFER_RENDER_SHADOW
    if(!s_NumDrawEdges)
    {
        return;
    }
    
    unsigned short i, j, start;

    pddiPrimBufferStream *stream;

    P3DASSERT(tShadowGeneratorImplCommon::GetCurVolumeShader());

      //Set the shadow volume colour. It would be better to have this as a shader constant,
    //so we wouldn't have to send this data over the bus many times.
    tShadowGeneratorImplCommon::GetCurVolumeShader()->SetColour(PDDI_SP_DIFFUSE, s_VolumeColour);

    shadowBuffer->SetShader( tShadowGeneratorImplCommon::GetCurVolumeShader( ) );
  
    Vector s;
    if(!isPointLight)
    {
        s = light;
        s.Scale(volumeLength);

        // Draw the shadow
        int passes = s_NumDrawEdges/MAX_EDGE_IN_BUFFER;
        int remains = s_NumDrawEdges%MAX_EDGE_IN_BUFFER;

        for( j = 0; j < passes; ++j ){

            int start = j * MAX_EDGE_IN_BUFFER;
            stream = shadowBuffer->GetBuffer( )->Lock( );

            for( i = start; i < start + MAX_EDGE_IN_BUFFER; ++i ){

                Vector& va = vertices[s_DrawEdges[i].a];
                Vector& vb = vertices[s_DrawEdges[i].b];

                pddiVector a,b,c,d;
                a.Set(va.x, va.y, va.z);
                b.Set(vb.x, vb.y, vb.z);
                c.Set(va.x+s.x, va.y+s.y, va.z+s.z);
                d.Set(vb.x+s.x, vb.y+s.y, vb.z+s.z);

                //Would be better as a quad. Also could consider sending multiple 
                //sets of tri's to try and get better fill rate(since the triangles would be
                //less ponty). Or could pre create something indexed.
                stream->Vertex(&a, s_VolumeColour);
                stream->Vertex(&c, s_VolumeColour);
                stream->Vertex(&b, s_VolumeColour);

                stream->Vertex(&b, s_VolumeColour);
                stream->Vertex(&c, s_VolumeColour);
                stream->Vertex(&d, s_VolumeColour);
            }
            shadowBuffer->GetBuffer( )->Unlock(  stream );

            SetupPass(PASS_FRONT, tShadowGeneratorImplCommon::GetCurVolumeShader());
            shadowBuffer->Display( );
            SetupPass(PASS_BACK, tShadowGeneratorImplCommon::GetCurVolumeShader());
            shadowBuffer->Display( );
        }

        if( remains > 0 ){
        //no draw the remaining edges
            start = passes * MAX_EDGE_IN_BUFFER;

            stream = shadowBuffer->GetBuffer( )->Lock( );
            for( i = start; i < start + remains; ++i ){
                Vector& va = vertices[s_DrawEdges[i].a];
                Vector& vb = vertices[s_DrawEdges[i].b];

                pddiVector a,b,c,d;
                a.Set(va.x, va.y, va.z);
                b.Set(vb.x, vb.y, vb.z);
                c.Set(va.x+s.x, va.y+s.y, va.z+s.z);
                d.Set(vb.x+s.x, vb.y+s.y, vb.z+s.z);

                //Would be better as a quad. Also could consider sending multiple 
                //sets of tri's to try and get better fill rate(since the triangles would be
                //less ponty). Or could pre create something indexed.
                stream->Vertex(&a, s_VolumeColour);
                stream->Vertex(&c, s_VolumeColour);
                stream->Vertex(&b, s_VolumeColour);

                stream->Vertex(&b, s_VolumeColour);
                stream->Vertex(&c, s_VolumeColour);
                stream->Vertex(&d, s_VolumeColour);
            }
            shadowBuffer->GetBuffer( )->Unlock( stream );

            shadowBuffer->GetBuffer( )->SetUsedSize( remains * 6 );

            SetupPass(PASS_FRONT, tShadowGeneratorImplCommon::GetCurVolumeShader());
            shadowBuffer->Display( );
            SetupPass(PASS_BACK, tShadowGeneratorImplCommon::GetCurVolumeShader());
            shadowBuffer->Display( );

            //reset the used size to the maximum
            shadowBuffer->GetBuffer( )->SetUsedSize( 0 );
        }
    }
    else
    {
        s = light;

       // Draw the shadow
        int passes = s_NumDrawEdges/MAX_EDGE_IN_BUFFER;
        int remains = s_NumDrawEdges%MAX_EDGE_IN_BUFFER;

        for( j = 0; j < passes; ++j ){

            int start = j * MAX_EDGE_IN_BUFFER;
            stream = shadowBuffer->GetBuffer( )->Lock( );

            for( i = start; i < start + MAX_EDGE_IN_BUFFER; ++i ){
                Vector& va = vertices[s_DrawEdges[i].a];
                Vector& vb = vertices[s_DrawEdges[i].b];

                pddiVector a,b,c,d;
                a.Set(va.x, va.y, va.z);
                b.Set(vb.x, vb.y, vb.z);
                Vector sa, sb;
                sa.Sub(va, s);sb.Sub(vb, s);
                sa.Normalize();sb.Normalize();
                sa.Scale(volumeLength);sb.Scale(volumeLength);
        
                c.Set(va.x+sa.x, va.y+sa.y, va.z+sa.z);
                d.Set(vb.x+sb.x, vb.y+sb.y, vb.z+sb.z);

                //Would be better as a quad. Also could consider sending multiple 
                //sets of tri's to try and get better fill rate(since the triangles would be
                //less ponty). Or could pre create something indexed.
                stream->Vertex(&a, s_VolumeColour);
                stream->Vertex(&c, s_VolumeColour);
                stream->Vertex(&b, s_VolumeColour);

                stream->Vertex(&b, s_VolumeColour);
                stream->Vertex(&c, s_VolumeColour);
                stream->Vertex(&d, s_VolumeColour);
            }
            shadowBuffer->GetBuffer( )->Unlock( stream );
            

            SetupPass(PASS_FRONT, tShadowGeneratorImplCommon::GetCurVolumeShader());
            shadowBuffer->Display( );
            SetupPass(PASS_BACK, tShadowGeneratorImplCommon::GetCurVolumeShader());
            shadowBuffer->Display( );
        }

        if( remains > 0 ){
        //no draw the remaining edges
            start = passes * MAX_EDGE_IN_BUFFER;

            stream = shadowBuffer->GetBuffer( )->Lock( );
            for( i = start; i < start + remains; ++i ){
                Vector& va = vertices[s_DrawEdges[i].a];
                Vector& vb = vertices[s_DrawEdges[i].b];

                pddiVector a,b,c,d;
                a.Set(va.x, va.y, va.z);
                b.Set(vb.x, vb.y, vb.z);
                Vector sa, sb;
                sa.Sub(va, s);sb.Sub(vb, s);
                sa.Normalize();sb.Normalize();
                sa.Scale(volumeLength);sb.Scale(volumeLength);
        
                c.Set(va.x+sa.x, va.y+sa.y, va.z+sa.z);
                d.Set(vb.x+sb.x, vb.y+sb.y, vb.z+sb.z);

                //Would be better as a quad. Also could consider sending multiple 
                //sets of tri's to try and get better fill rate(since the triangles would be
                //less ponty). Or could pre create something indexed.
                stream->Vertex(&a, s_VolumeColour);
                stream->Vertex(&c, s_VolumeColour);
                stream->Vertex(&b, s_VolumeColour);

                stream->Vertex(&b, s_VolumeColour);
                stream->Vertex(&c, s_VolumeColour);
                stream->Vertex(&d, s_VolumeColour);
            }
            shadowBuffer->GetBuffer( )->Unlock( stream );

            shadowBuffer->GetBuffer( )->SetUsedSize( remains * 6 );
           //draw current buffer
            SetupPass(PASS_FRONT, tShadowGeneratorImplCommon::GetCurVolumeShader());
            shadowBuffer->Display( );
            SetupPass(PASS_BACK, tShadowGeneratorImplCommon::GetCurVolumeShader());
            shadowBuffer->Display( );

            //reset the used size to the maximum
            shadowBuffer->GetBuffer( )->SetUsedSize( 0 );
        }
 
    }
#else
    unsigned short i;
    pddiPrimStream* stream;
    stream = p3d::pddi->BeginPrims(tShadowGeneratorImplCommon::GetCurVolumeShader()->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_C, 6 * s_NumDrawEdges);

    //Set the shadow volume colour. It would be better to have this as a shader constant,
    //so we wouldn't have to send this data over the bus many times.
    tShadowGeneratorImplCommon::GetCurVolumeShader()->SetColour(PDDI_SP_DIFFUSE, s_VolumeColour);

    Vector s;
    if(!isPointLight)
    {
        s = light;
        s.Scale(volumeLength);

        // Draw the shadow
        for(i=0;i<s_NumDrawEdges;i++)
        {
            Vector& va = vertices[s_DrawEdges[i].a];
            Vector& vb = vertices[s_DrawEdges[i].b];

            pddiVector a,b,c,d;
            a.Set(va.x, va.y, va.z);
            b.Set(vb.x, vb.y, vb.z);
            c.Set(va.x+s.x, va.y+s.y, va.z+s.z);
            d.Set(vb.x+s.x, vb.y+s.y, vb.z+s.z);

            //Would be better as a quad. Also could consider sending multiple 
            //sets of tri's to try and get better fill rate(since the triangles would be
            //less ponty). Or could pre create something indexed.
            stream->Vertex(&a, s_VolumeColour);
            stream->Vertex(&c, s_VolumeColour);
            stream->Vertex(&b, s_VolumeColour);

            stream->Vertex(&b, s_VolumeColour);
            stream->Vertex(&c, s_VolumeColour);
            stream->Vertex(&d, s_VolumeColour);
        }
    }
    else
    {
        s = light;

        //Draw the shadow
        for(i=0;i<s_NumDrawEdges;i++)
        {
            Vector& va = vertices[s_DrawEdges[i].a];
            Vector& vb = vertices[s_DrawEdges[i].b];

            pddiVector a,b,c,d;
            a.Set(va.x, va.y, va.z);
            b.Set(vb.x, vb.y, vb.z);
            Vector sa, sb;
            sa.Sub(va, s);sb.Sub(vb, s);
            sa.Normalize();sb.Normalize();
            sa.Scale(volumeLength);sb.Scale(volumeLength);
        
            c.Set(va.x+sa.x, va.y+sa.y, va.z+sa.z);
            d.Set(vb.x+sb.x, vb.y+sb.y, vb.z+sb.z);

            //Would be better as a quad. Also could consider sending multiple 
            //sets of tri's to try and get better fill rate(since the triangles would be
            //less ponty). Or could pre create something indexed.
            stream->Vertex(&a, s_VolumeColour);
            stream->Vertex(&c, s_VolumeColour);
            stream->Vertex(&b, s_VolumeColour);

            stream->Vertex(&b, s_VolumeColour);
            stream->Vertex(&c, s_VolumeColour);
            stream->Vertex(&d, s_VolumeColour);
        }
    }

    p3d::pddi->EndPrims(stream);
#endif
}

void tShadowMeshImpl::DrawNormals(void)
{
    pddiPrimStream* stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_LINES, PDDI_V_C, 2 * numTriangles);
    tColour white(255,255,255,255);
    
    for(unsigned i = 0; i < numTriangles; i++)
    {
        rmt::Vector centre;
        centre.Interpolate(vertices[topology[i].v0], vertices[topology[i].v1], 0.5f);
        centre.Interpolate(centre, vertices[topology[i].v2], 0.5f);
        
        stream->Colour(white);
        stream->Coord(centre.x, centre.y, centre.z);

        centre.Add(normals[i]);

        stream->Colour(white);
        stream->Coord(centre.x, centre.y, centre.z);
    }

    p3d::pddi->EndPrims(stream);
}


void tShadowMeshImpl::DrawShell(void)
{
    pddiPrimStream* stream = p3d::pddi->BeginPrims(NULL, PDDI_PRIM_TRIANGLES, PDDI_V_C, 3 * numTriangles);
    tColour blue(0,0,255,255);
    
    for(unsigned i = 0; i < numTriangles; i++)
    {
        stream->Colour(blue);
        stream->Coord(vertices[topology[i].v0].x, vertices[topology[i].v0].y, vertices[topology[i].v0].z);

        stream->Colour(blue);
        stream->Coord(vertices[topology[i].v1].x, vertices[topology[i].v1].y, vertices[topology[i].v1].z);

        stream->Colour(blue);
        stream->Coord(vertices[topology[i].v2].x, vertices[topology[i].v2].y, vertices[topology[i].v2].z);
    }

    p3d::pddi->EndPrims(stream);
}

void tShadowMeshImpl::GuessVolumeLength(void)
{
    // if someone has set a volume length, use it
    if(setVolumeLength != 0.0f)
    {
        volumeLength = setVolumeLength;
        return;
    }
    // If the volume lenght has been precalculated
    //earlier in the display process we can leave.
    if(volumeLength != 0.0f)
    {
        return;
    }
    
    // otherwise try to guess
    // note : this code is not even remotley correct, it works sometimes, 
    // but needs dramatic improvment
    tCamera* camera = p3d::context->GetView()->GetCamera();
    Vector cameraLight;
    Sphere tmpSphere = boundingSphere;
    p3d::context->ObjectToWorld(boundingSphere.centre, &tmpSphere.centre);
    camera->WorldToCamera(tmpSphere.centre, &tmpSphere.centre);

    if(!isPointLight)
    {
        camera->GetWorldToCameraMatrix().RotateVector(worldLight, &cameraLight);
    }
    else
    {
        Vector tmp;
        camera->WorldToCamera(worldLight, &tmp);
        cameraLight = tmpSphere.centre;
        cameraLight.Sub(tmp);
        cameraLight.Normalize();
    }

    unsigned i;

    for(i = 1; i < 100; ++i)
    {
        tmpSphere.centre.Add(cameraLight);
        if(!camera->SphereVisibleCamera(tmpSphere.centre, tmpSphere.radius))
        {
            break;
        }
    }

    volumeLength = (float)i;
}

//
// tShadowSkinImpl
// 
tShadowSkinImpl::tShadowSkinImpl() :
    skeleton(NULL),
    boneMatrix(NULL),
    sourceVertexOne(NULL),
    sourceVertexTwo(NULL),
    sourceVertexThree(NULL),
    numVertexOne(0),
    numVertexTwo(0),
    numVertexThree(0),
    pose(NULL)
{
}

tShadowSkinImpl::~tShadowSkinImpl()
{
    tRefCounted::Release(skeleton);
    tRefCounted::Release(pose);
    delete[] boneMatrix;
    delete[] sourceVertexOne;
    delete[] sourceVertexTwo;
    delete[] sourceVertexThree;
}

void tShadowSkinImpl::SetPose(tPose* p) 
{ 
    tRefCounted::Assign(pose, p); 
}

void tShadowSkinImpl::Display() 
{ 
    if(!pose) 
    {
        tPose* p = skeleton->NewPose();
        tRefCounted::Assign(pose, p);
    }

    Display(pose);
}

void tShadowSkinImpl::Display(tPose* p)
{
    if(!p) 
    {
        return;
    }

    unsigned int i;

//    timeF = radTimeGetMicroseconds();
    // transform the pose joints into world space
    p->Evaluate();

    bool poseChanged = false;

    // The skin vertices are stored in rest-pose relative world space.
    // The below matrix will transform from this space, into bone-local
    // space where the animation is applied, and then back into camera space.
    unsigned numJoints = rmt::Min( skeleton->GetNumJoint(), p->GetNumJoint() );
    for(i = 0; i < numJoints; ++i)
    {
        Matrix tmp;

#ifndef RAD_PS2
        tmp.Mult(skeleton->GetJoint(i)->inverseWorldMatrix, p->GetJoint(i)->worldMatrix);
#else
        tmp.MultAligned(skeleton->GetJoint(i)->inverseWorldMatrix, p->GetJoint(i)->worldMatrix);
#endif
        if(!poseChanged)
        {
            for(unsigned j = 0; j < 4; j++)
            {
                for(unsigned k = 0; k < 4; k++)
                {
                    if(boneMatrix[i].m[j][k] != tmp.m[j][k])
                    {
                        poseChanged = true;
                        goto poseHasChanged;
                    }
                }
            }
poseHasChanged: ;
        }

        boneMatrix[i] = tmp;
    }

    if(poseChanged)
    {
        P3DASSERT(numVertices == numVertexOne + numVertexTwo + numVertexThree);
        rmt::Vector* pOut = vertices;
        Vector vertex;    //Vertex accumulator
        Vector* sv;
        float weight;
        Matrix* m;
        for(i=0; i<numVertexThree;i++)
        {
            //Fetch source vertex information
            VertexThree* pVertex = &(sourceVertexThree[i]);
            sv = &pVertex->position;

            // compute weighted skin co-ordinate
            weight = pVertex->weights[0];
            m = &boneMatrix[pVertex->matrixIndex[0]];
            vertex.x = (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y = (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z = (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            weight = pVertex->weights[1];
            m = &boneMatrix[pVertex->matrixIndex[1]];
            vertex.x += (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y += (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z += (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            weight = pVertex->weights[2];
            m = &boneMatrix[pVertex->matrixIndex[2]];
            vertex.x += (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y += (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z += (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            // output transformed vertex
            pOut[pVertex->vertexIndex] = vertex;
        }
        for(i=0; i<numVertexTwo;i++)
        {
            //Fetch source vertex information
            VertexTwo* pVertex = &(sourceVertexTwo[i]);
            sv = &pVertex->position;

            // compute weighted skin co-ordinate
            weight = pVertex->weights[0];
            m = &boneMatrix[pVertex->matrixIndex[0]];
            vertex.x = (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y = (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z = (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            weight = pVertex->weights[1];
            m = &boneMatrix[pVertex->matrixIndex[1]];
            vertex.x += (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]) * weight;
            vertex.y += (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]) * weight;
            vertex.z += (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]) * weight;

            // output transformed vertex
            pOut[pVertex->vertexIndex] = vertex;
        }
        for(i=0; i<numVertexOne;i++)
        {
            //Fetch source vertex information
            VertexOne* pVertex = &(sourceVertexOne[i]);
            m = &boneMatrix[pVertex->matrixIndex[0]];

            // compute weighted skin co-ordinate
            sv = &pVertex->position;
            vertex.x = (m->m[0][0]*sv->x + m->m[1][0]*sv->y + m->m[2][0]*sv->z + m->m[3][0]);
            vertex.y = (m->m[0][1]*sv->x + m->m[1][1]*sv->y + m->m[2][1]*sv->z + m->m[3][1]);
            vertex.z = (m->m[0][2]*sv->x + m->m[1][2]*sv->y + m->m[2][2]*sv->z + m->m[3][2]);

            // output transformed vertex
            pOut[pVertex->vertexIndex] = vertex;
        }

       //Update per triangle normal
        Vector vAB, vBC;
        for(i=0; i<numTriangles;i++)
        {
            Vector &a = vertices[topology[i].v0];
            Vector &b = vertices[topology[i].v1];
            Vector &c = vertices[topology[i].v2];

            //Optimization fodder
            vAB.Sub(b, a);    
            vBC.Sub(c, b);
            normals[i].CrossProduct(vAB, vBC);
        }
    }

    GuessVolumeLength( p );
    tShadowMeshImpl::Display();
}

void tShadowSkinImpl::GuessVolumeLength( tPose* Pose )
{
    // if someone has set a volume length, use it
    if( setVolumeLength != 0.0f )
    {
        volumeLength = setVolumeLength;
        return;
    }
    // The length has been precalculated somewhere else.
    if( ( volumeLength != 0.0f ) || ( Pose == 0 ) )
    {
        return;
    }

    // otherwise try to guess
    // note : this code is perfect... :)
    tCamera* camera = p3d::context->GetView()->GetCamera();
    Vector cameraLight;
    Sphere tmpSphere = boundingSphere;
    const rmt::Vector& worldPos = Pose->GetJoint( 0 )->worldMatrix.Row( 3 );
    tmpSphere.centre.Add( worldPos );
    camera->WorldToCamera(tmpSphere.centre, &tmpSphere.centre);

    if(!isPointLight)
    {
        camera->GetWorldToCameraMatrix().RotateVector(worldLight, &cameraLight);
    }
    else
    {
        Vector tmp;
        camera->WorldToCamera(worldLight, &tmp);
        cameraLight = tmpSphere.centre;
        cameraLight.Sub(tmp);
        cameraLight.Normalize();
    }

    unsigned i;

    for(i = 1; i < 100; ++i)
    {
        tmpSphere.centre.Add(cameraLight);
        if(!camera->SphereVisibleCamera(tmpSphere.centre, tmpSphere.radius))
        {
            break;
        }
    }

    volumeLength = (float)i;
}

//-------------------------------------------------------------------
static const int SHADOWMESH_VERSION = 0;

tShadowMeshLoader::tShadowMeshLoader() : tSimpleChunkHandler(Pure3D::Mesh::SHADOWMESH) {};

tEntity* tShadowMeshLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int i;

    char name[128];
    int version;
    unsigned long numVertices;
    unsigned long numTriangles;

    //Read the chunk header information
    f->GetPString(name);
        version = f->GetLong();
    numVertices = f->GetLong();
    numTriangles = f->GetLong();

    //Some basic sanity checking
    P3DASSERT(version == SHADOWMESH_VERSION);

    //Create the tShadowSkinImpl
    tShadowMesh *s = Allocate();
    s->SetName(name);

    tShadowMeshImpl* shadow = &s->impl;
    
    shadow->numTriangles = numTriangles;
    shadow->numVertices = numVertices;

    //Preallocate the static shadow data
    shadow->topology = new tShadowMeshImpl::Topology[numTriangles];

    //Allocate space for the dynamic data
    shadow->vertices = new Vector[numVertices];
    shadow->normals = new Vector[numTriangles];
    shadow->triCulled = new bool[numTriangles];

    int index = 0;

    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        switch(f->GetCurrentID())
        {
            case Pure3D::Mesh::TOPOLOGY:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == shadow->numTriangles);
                P3DASSERT(shadow->topology);
                for(i=0; i<count; i++)
                {
                   f->GetData(&shadow->topology[i], 6, tFile::WORD);
                }
                break;
            }
            case Pure3D::Mesh::POSITIONLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == numVertices);
                P3DASSERT(shadow->vertices);
                for(i=0; i<count; i++)
                {
                   f->GetData(&shadow->vertices[i], 3, tFile::DWORD);
                }
                break;
            }
            case Pure3D::Mesh::SPHERE:
            {
                shadow->boundingSphere.centre.x = f->GetFloat();
                shadow->boundingSphere.centre.y = f->GetFloat();
                shadow->boundingSphere.centre.z = f->GetFloat();
                shadow->boundingSphere.radius = f->GetFloat();
                break;
            }
            case Pure3D::Mesh::BOX:
            {
                shadow->boundingBox.low.x = f->GetFloat();
                shadow->boundingBox.low.y = f->GetFloat();
                shadow->boundingBox.low.z = f->GetFloat();
                shadow->boundingBox.high.x = f->GetFloat();
                shadow->boundingBox.high.y = f->GetFloat();
                shadow->boundingBox.high.z = f->GetFloat();
                break;
            }
            default:
                break;
        }
        f->EndChunk();
    }

    //calculate normals
    Vector vAB, vBC;
    for(i=0; i<shadow->numTriangles;i++)
    {
        Vector &a = shadow->vertices[shadow->topology[i].v0];
        Vector &b = shadow->vertices[shadow->topology[i].v1];
        Vector &c = shadow->vertices[shadow->topology[i].v2];

        vAB.Sub(b, a);    
        vBC.Sub(c, b);
        shadow->normals[i].CrossProduct(vAB, vBC);
    }

    return s;
}

tShadowMesh* tShadowMeshLoader::Allocate()
{
    return new tShadowMesh();
}

//-------------------------------------------------------------------
static const int SHADOWSKIN_VERSION = 0;

tShadowSkinLoader::tShadowSkinLoader() : tSimpleChunkHandler(Pure3D::Mesh::SHADOWSKIN) {};

tEntity* tShadowSkinLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    unsigned int i;

    char name[128];
    int version;
    char skelName[128];
    unsigned long numVertices;
    unsigned long numTriangles;

    //Read the chunk header information
    f->GetPString(name);
        version = f->GetLong();
    f->GetPString(skelName);
    numVertices = f->GetLong();
    numTriangles = f->GetLong();

    //Some basic sanity checking
    P3DASSERT(version == SHADOWSKIN_VERSION);

    //Create the tShadowSkinImpl
    tShadowSkin *s = Allocate();
    s->SetName(name);

    tShadowSkinImpl* shadow = &s->impl;

    shadow->numTriangles = numTriangles;
    shadow->numVertices = numVertices;

    // Temp buffer for holding vertex information.
    tShadowSkinImpl::VertexThree* sourceVertexThree = (tShadowSkinImpl::VertexThree*)p3d::MallocTemp(sizeof(tShadowSkinImpl::VertexThree) * numVertices );
    P3DASSERT( sourceVertexThree );

    //Preallocate the static shadow data
    shadow->topology = new tShadowSkinImpl::Topology[numTriangles];

    //Allocate space for the dynamic data
    shadow->vertices= new Vector[numVertices];
    shadow->normals = new Vector[numTriangles];
    shadow->triCulled = new bool[numTriangles];

    //Setup the skeleton
    shadow->skeleton = p3d::find<tSkeleton>(store, skelName);
    if(shadow->skeleton)
    {
        shadow->skeleton->AddRef();
        shadow->pose = shadow->skeleton->NewPose();
        shadow->pose ->AddRef();
        // allocate one extra bone for the special identity matrix
        shadow->boneMatrix = new rmt::Matrix[shadow->pose->GetNumJoint()+1];
        shadow->boneMatrix[shadow->pose->GetNumJoint()].Identity();
    }
    int index = 0;

    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        switch(f->GetCurrentID())
        {
            case Pure3D::Mesh::TOPOLOGY:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == shadow->numTriangles);
                P3DASSERT(shadow->topology);
                for(i=0; i<count; i++)
                {
                   f->GetData(&shadow->topology[i], 6, tFile::WORD);
                }
                break;
            }
            case Pure3D::Mesh::POSITIONLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == numVertices);

                for(i=0; i<count; i++)
                {
                    f->GetData(&sourceVertexThree[i].position, 3, tFile::DWORD);
                }
                break;
            }
            case Pure3D::Mesh::WEIGHTLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                P3DASSERT(count == numVertices);
                for(i=0; i<count; i++)
                {
                   f->GetData(&sourceVertexThree[i].weights, 3, tFile::DWORD);
                }
                break;
            }
            case Pure3D::Mesh::MATRIXLIST:
            {
                unsigned int count = (unsigned int) f->GetLong();
                unsigned char tempIndex[4];
                P3DASSERT(count == numVertices);
                for(i=0; i<count; i++)
                {
                    //For some reason these seem to be in reverse order.
                    f->GetData(&tempIndex, 4, tFile::BYTE);
                    sourceVertexThree[i].matrixIndex[0] = tempIndex[3];
                    sourceVertexThree[i].matrixIndex[1] = tempIndex[2];
                    sourceVertexThree[i].matrixIndex[2] = tempIndex[1];
                    sourceVertexThree[i].matrixIndex[3] = tempIndex[0];
                }
                break;
            }
            case Pure3D::Mesh::SPHERE:
                shadow->boundingSphere.centre.x = f->GetFloat();
                shadow->boundingSphere.centre.y = f->GetFloat();
                shadow->boundingSphere.centre.z = f->GetFloat();
                shadow->boundingSphere.radius = f->GetFloat();
                break;

            case Pure3D::Mesh::BOX:
                shadow->boundingBox.low.x = f->GetFloat();
                shadow->boundingBox.low.y = f->GetFloat();
                shadow->boundingBox.low.z = f->GetFloat();
                shadow->boundingBox.high.x = f->GetFloat();
                shadow->boundingBox.high.y = f->GetFloat();
                shadow->boundingBox.high.z = f->GetFloat();
                break;

            default:
            break;
        }
        f->EndChunk();
    }

    for( i = 0; i < numVertices; ++i )
    {
        tShadowSkinImpl::VertexThree* pSource = &( sourceVertexThree[ i ] );
        if( 0 == pSource->weights[ 2 ] && 0 == pSource->weights[ 1 ] )
        {
            // One bone.
            ++(shadow->numVertexOne);
        }
        else if( 0 == pSource->weights[ 2 ] )
        {
            ++(shadow->numVertexTwo);
        }
        else
        {
            ++(shadow->numVertexThree);
        }
    }

    shadow->sourceVertexOne = new tShadowSkinImpl::VertexOne[ shadow->numVertexOne ];
    shadow->sourceVertexTwo = new tShadowSkinImpl::VertexTwo[ shadow->numVertexTwo ];
    shadow->sourceVertexThree = new tShadowSkinImpl::VertexThree[ shadow->numVertexThree ];

    int vertexOneIndex = 0;
    int vertexTwoIndex = 0;
    int vertexThreeIndex = 0;

    //Sort the vertices by number of weights
    for(i=0;i<numVertices;i++)
    {
        //Source vertex
        tShadowSkinImpl::VertexThree* source = &(sourceVertexThree[i]);

        //Bucket according to number of bones.
        if(0==source->weights[2] && 0==source->weights[1])
        {
            //one bone
            tShadowSkinImpl::VertexOne* pDest = &(shadow->sourceVertexOne[ vertexOneIndex ]);
            pDest->position = source->position;
            pDest->vertexIndex = i;
            pDest->matrixIndex[0] = source->matrixIndex[0];
            ++vertexOneIndex;
        }
        else if(0==source->weights[2])
        {
            //two bones
            tShadowSkinImpl::VertexTwo* pDest = &(shadow->sourceVertexTwo[ vertexTwoIndex]);
            pDest->position = source->position;
            pDest->vertexIndex = i;
            pDest->matrixIndex[0] = source->matrixIndex[0];
            pDest->matrixIndex[1] = source->matrixIndex[1];
            pDest->weights[0] = source->weights[0];
            pDest->weights[1] = source->weights[1];
            ++vertexTwoIndex;
        }
        else
        {
            //three bones
            tShadowSkinImpl::VertexThree* pDest = &( shadow->sourceVertexThree[ vertexThreeIndex ] );
            pDest->position = source->position;
            pDest->vertexIndex = i;
            pDest->matrixIndex[ 0 ] = source->matrixIndex[ 0 ];
            pDest->matrixIndex[ 1 ] = source->matrixIndex[ 1 ];
            pDest->matrixIndex[ 2 ] = source->matrixIndex[ 2 ];
            pDest->weights[ 0 ] = source->weights[ 0 ];
            pDest->weights[ 1 ] = source->weights[ 1 ];
            pDest->weights[ 2 ] = source->weights[ 2 ];
            ++vertexThreeIndex;
        }
    }

    P3DASSERT( (unsigned)vertexOneIndex == shadow->numVertexOne );
    P3DASSERT( (unsigned)vertexTwoIndex == shadow->numVertexTwo );
    P3DASSERT( (unsigned)vertexThreeIndex == shadow->numVertexThree );

    p3d::FreeTemp(sourceVertexThree);

    return s;
}

tShadowSkin* tShadowSkinLoader::Allocate()
{
    return new tShadowSkin();
}
*/