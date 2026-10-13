//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "context.hpp"
#include "device.hpp"
#include "shaders/shader.hpp"
#include "display.hpp"
#include "util.hpp"
#include "prim.hpp"
#include "primstream.hpp"
#include "vertprog.hpp"
#include "state.hpp"
#include "shaders/simpleshader.hpp"

#include <math.h>

//-------------------------------------------------------------------
// these tables convert pddi enums into the corresponding
// D3D enums

// pddiLightType
static D3DLIGHTTYPE lightTable[] = 
{
    D3DLIGHT_DIRECTIONAL,
    D3DLIGHT_POINT,
    D3DLIGHT_SPOT
};

// pddiPrimType
static D3DPRIMITIVETYPE primTable[] =
{
    D3DPT_TRIANGLELIST,
    D3DPT_TRIANGLESTRIP,
    D3DPT_LINELIST,
    D3DPT_LINESTRIP,
    D3DPT_POINTLIST,
};

// pddiMatrixType
static D3DTRANSFORMSTATETYPE matrixTable[] = 
{

    D3DTS_WORLD,
    D3DTS_TEXTURE0,
    D3DTS_TEXTURE1,
    D3DTS_TEXTURE2,
    D3DTS_TEXTURE3
};

// pddiCullMode
static D3DCULL cullTable[] =
{
    D3DCULL_NONE,
    D3DCULL_CCW,
    D3DCULL_CW
};

// pddiStencilMode
static D3DSTENCILOP stencilTable[] =
{
    D3DSTENCILOP_KEEP,
    D3DSTENCILOP_ZERO,
    D3DSTENCILOP_REPLACE,
    D3DSTENCILOP_INCR,
    D3DSTENCILOP_DECR,
    D3DSTENCILOP_INVERT
};

// pddiCompareMode
static D3DCMPFUNC compareTable[] =
{
    D3DCMP_NEVER,
    D3DCMP_ALWAYS,
    D3DCMP_LESS,
    D3DCMP_LESSEQUAL,
    D3DCMP_GREATER,
    D3DCMP_GREATEREQUAL,
    D3DCMP_EQUAL,
    D3DCMP_NOTEQUAL
};

// pddiFillMode
static D3DFILLMODE fillTable[] = 
{
    D3DFILL_SOLID,
    D3DFILL_WIREFRAME,
    D3DFILL_POINT
};

//----

class d3dExtGammaControl : public pddiExtGammaControl
{
public:
    d3dExtGammaControl(d3dDisplay* d) { display = d;}

    void SetGamma(float r, float g, float b)     {display->SetGamma(r,g,b);}
    void GetGamma(float *r, float *g, float *b)  {display->GetGamma(r,g,b);}

protected:
    d3dDisplay* display;
};

class d3dExtAntialiasControl : public pddiExtAntialiasControl
{
public:
    d3dExtAntialiasControl(d3dContext* c) { context = c; }

    void EnableMultiSampling(bool b)
    {
        context->GetDisplay()->GetD3DDevice()->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,b);
    }
    void EnablePolyClear(bool b)
    {
        context->polyClear = b;
    }

    void SetWriteMask(unsigned mask)
    {
        context->GetDisplay()->GetD3DDevice()->SetRenderState(D3DRS_MULTISAMPLEMASK,mask);
    }
    
    // ziemek - temp fix to get dx8 building.
    int GetAntiAliasMode(void)
    {
        return 0;
    }

protected:
    d3dContext* context;
};

class d3dExtGlobalProjectedTexture: public pddiExtGlobalProjectedTexture
{
public:
    d3dExtGlobalProjectedTexture()
    {
        for(int i = 0; i < PDDI_MAX_GLOBAL_PROJECTED_TEXTURE; i++)
        {
            textures[i] = NULL;
        }
    }

    ~d3dExtGlobalProjectedTexture()
    {
        for(int i = 0; i < PDDI_MAX_GLOBAL_PROJECTED_TEXTURE; i++)
        {
            if(textures[i])
                textures[i]->Release();
        }
    }


    void SetProjection(unsigned id, const pddiMatrix* matrix, pddiTexture* tex)
    {
        PDDIASSERT(id < PDDI_MAX_GLOBAL_PROJECTED_TEXTURE);
        matrices[id] = *matrix;
        if(tex)
            tex->AddRef();

        if(textures[id])
            textures[id]->Release();

        textures[id] = tex;
    }
        
    virtual bool GetProjection(unsigned id, pddiMatrix* matrix, pddiTexture** tex)
    {
        PDDIASSERT(id < PDDI_MAX_GLOBAL_PROJECTED_TEXTURE);
        *matrix = matrices[id];
        *tex = textures[id];
        return (*tex != NULL);
    }

private:
    pddiTexture* textures[PDDI_MAX_GLOBAL_PROJECTED_TEXTURE];
    pddiMatrix matrices[PDDI_MAX_GLOBAL_PROJECTED_TEXTURE];
};

class d3dExtFramebufferEffects : public pddiExtFramebufferEffects 
{
public:
    d3dExtFramebufferEffects(d3dContext* c)
    {
        context = c;

        shader = NULL;
        texture = NULL;

        enable = false;
        alpha = 0.9f;
        zoom = 0.0f; 
        rgb = false;
    }

    ~d3dExtFramebufferEffects()
    {
        if( shader )
            shader->Release( );

        if( texture )
            texture->Release( );
    }

    void EnableMotionBlur(bool e, float a, float z, bool fbm ) 
    {
        enable = e;
        alpha = a;
        zoom = z;
        rgb = fbm;
    }

    void RenderMotionBlur(void)
    {
        if( zoom ){         // feedback
            RenderFeedBack(  );
        }
        else{
            RenderMotionBlurInternal( );
        }

    }
    
    void SetQuality(Quality q) { }

protected:

    void RenderMotionBlurInternal( )
    {
        if(!enable)
            return;

        if(!shader)
        {
            shader = context->GetDevice()->NewShader("simple");
            texture = new d3dTexture(context, NULL);

            shader->AddRef();
            texture->AddRef();

            shader->SetTexture(PDDI_SP_BASETEX, texture);
        }

        pddiColour colour( 255, 255, 255, (unsigned char)(255.0f * alpha) );
        
        float dummy;
        float nearPlane;
        float aspect;

        context->GetCamera(&nearPlane, &dummy, &dummy, &aspect);
        context->SetProjectionMode(PDDI_PROJECTION_DEVICE);
        context->EnableZBuffer(false);
        context->IdentityMatrix(PDDI_MATRIX_MODELVIEW);

        shader->SetInt(PDDI_SP_BLENDMODE, PDDI_BLEND_ALPHA);
               
        texture->ChangeForgedTexture( context->GetDisplay()->GetSnapshotTexture( ) );

        pddiPrimStream* stream = context->BeginPrims(shader, PDDI_PRIM_TRISTRIP, PDDI_V_CT, 4);

        float top = 0;
        float left = 0;
        float bottom = (float)context->GetDisplay()->GetHeight();
        float right = (float)context->GetDisplay()->GetWidth();

        float topv = 0 + (zoom* aspect);
        float leftu = 0 + zoom;
        float bottomv = bottom - (zoom * aspect );
        float rightu = right - zoom;

        stream->Colour(colour);
        stream->UV(leftu,topv);
        stream->Coord(left, top, nearPlane);

        stream->Colour(colour);
        stream->UV(rightu,topv);
        stream->Coord(right, top, nearPlane);

        stream->Colour(colour);
        stream->UV(leftu,bottomv);
        stream->Coord(left, bottom, nearPlane);

        stream->Colour(colour);
        stream->UV(rightu,bottomv);
        stream->Coord(right, bottom, nearPlane);

        context->EndPrims(stream);

        texture->ChangeForgedTexture(NULL);

        context->SetProjectionMode(PDDI_PROJECTION_PERSPECTIVE);
        context->EnableZBuffer(true);

    }
    void RenderFeedBack(  )
    {
        if(!enable)
            return;

        if(!shader)
        {
            shader = context->GetDevice()->NewShader("simple");
            texture = new d3dTexture(context, NULL);

            shader->AddRef();
            texture->AddRef();

            shader->SetTexture(PDDI_SP_BASETEX, texture);
        }

        pddiColour colour[ 3 ];

        int pass;
        unsigned char value = (unsigned char)(254.f*alpha);
        float factor[ 3 ] = { zoom, zoom+34, zoom+15 };
		

        if( rgb ){
            pass = 3;

            colour[ 0 ].Set( value, 0, 0, 0 );
            colour[ 1 ].Set( 0, value, 0, 0 );
            colour[ 2 ].Set( 0, 0, value, 0 );            
        }
        else{
            pass = 1;
            colour[ 0 ].Set( value, value, value, 0 );
        }

        float dummy;
        float nearPlane;
        float aspect;

        context->GetCamera(&nearPlane, &dummy, &dummy, &aspect);
        context->SetProjectionMode(PDDI_PROJECTION_DEVICE);
        context->EnableZBuffer(false);
        context->IdentityMatrix(PDDI_MATRIX_MODELVIEW);

        shader->SetInt(PDDI_SP_BLENDMODE, PDDI_BLEND_ADD);

		context->GetDisplay()->Snapshot( );
        texture->ChangeForgedTexture( context->GetDisplay()->GetSnapshotTexture( ) );

        float top = 0;
        float left = 0;
        float bottom = (float)context->GetDisplay()->GetHeight();
        float right = (float)context->GetDisplay()->GetWidth();
/*
		float top = 200.f;
        float left = 200.f;
        float bottom = 400.f;
        float right = 450.f;
*/
        for( int i = 0; i < pass; ++i ){
            pddiPrimStream* stream = context->BeginPrims(shader, PDDI_PRIM_TRISTRIP, PDDI_V_CT, 4);


            float topv = (factor[ i ] * aspect)/bottom;
            float leftu = factor[ i ]/right;
            float bottomv = ( bottom - (factor[ i ] * aspect ) )/bottom;
            float rightu = (right - factor[ i ])/right;
/*
            float topv = 0.f;
            float leftu = 0.f;
            float bottomv = 1.f;
            float rightu = 1.f;
*/
           stream->Colour(colour[ i ]);
            stream->UV(leftu,topv);
            stream->Coord(left, top, nearPlane);

            stream->Colour(colour[ i ]);
            stream->UV(rightu,topv);
            stream->Coord(right, top, nearPlane);

            stream->Colour(colour[ i ]);
            stream->UV(leftu,bottomv);
            stream->Coord(left, bottom, nearPlane);

            stream->Colour(colour[ i ]);
            stream->UV(rightu,bottomv);
            stream->Coord(right, bottom, nearPlane);

            context->EndPrims(stream);

        }

        texture->ChangeForgedTexture(NULL);

        context->SetProjectionMode(PDDI_PROJECTION_PERSPECTIVE);
        context->EnableZBuffer(true);
    }

    d3dContext* context;
    pddiShader* shader;
    d3dTexture* texture;

    bool enable;
    float alpha;
    float zoom;    
    bool rgb;               //render 3 passes for rgb separated feedback
};

class d3dExtReadPixels : public pddiExtReadPixels
{
    public:
        d3dExtReadPixels( d3dContext* c );
        virtual void ReadPixels( pddiBuffer source, const pddiRect& rect, pddiColour* buffer ) const;
        virtual ~d3dExtReadPixels();

    protected:
        d3dContext* context;

    private:

};


d3dExtReadPixels::d3dExtReadPixels( d3dContext* c )
{
    context = c;
}


d3dExtReadPixels::~d3dExtReadPixels()
{
}


void d3dExtReadPixels::ReadPixels( pddiBuffer source, const pddiRect& rectangle, pddiColour* buffer ) const
{
    context->GetDisplay()->Screenshot(buffer, (rectangle.right - rectangle.left) * (rectangle.bottom - rectangle.top) * 4, rectangle, source);
}
//-------------------------------------------------------------------
//		Hardware skinning
//-------------------------------------------------------------------
class d3dExtHardwareSkinning: public pddiExtHardwareSkinning
{
public:
    d3dExtHardwareSkinning(d3dContext* c) { context = c; }

    pddiPrimBuffer* NewPrimBuffer(unsigned,pddiPrimBufferDesc* desc)
    {
        return context->GetDevice()->NewPrimBuffer(desc);
    }

    int MaxMatrixPaletteSize(unsigned weightcount)
    {
        return 25;
    }

    void SetMatrixCount(unsigned count) { ; }

    void SetMatrix(unsigned index, pddiMatrix* m)
    {
        pddiMatrix tmp, transpose;
        tmp.Mult(*m, *context->GetMatrix(PDDI_MATRIX_MODELVIEW));

        LPDIRECT3DDEVICE8 m_pd3dDevice = context->GetDisplay()->GetD3DDevice();

        transpose.Transpose(tmp);

        PDDIASSERT("A single vertex buffer may not have more than 25 bones. Use 'p3dprimgroup -m 25' one to split up prim groups" && ((17 + (index*3) + 2) < 96));
        m_pd3dDevice->SetVertexShaderConstant(17 + (index*3), &transpose,   3 );
    }

    void Begin(void)
    {
    }

    void End(void)
    {
    }

    void DrawSkin(pddiShader* shader, pddiPrimBuffer* skin)
    {
        context->LoadSkinConstants(shader);

        d3dShader *s = (d3dShader *)( shader );

        s->EnableSkinMode( true );

        pddiBaseShader* material = (pddiBaseShader*)shader;
        d3dPrimBuffer* buffer = (d3dPrimBuffer*)skin;

        pddiVector lVec;
        context->GetLightDirection(0, &lVec); 
        material->SetVector(PDDI_SP_LIGHTV, lVec);
    
        for(int i = 0; i < material->GetPasses(); i++)
        {
            context->ADD_STAT(PDDI_STAT_MATERIAL_OPS, !material->IsCurrent());
            material->SetMaterial(i);
            buffer->Display();
        }

        s->EnableSkinMode( false );
    }

    d3dContext* context;
};

//-------------------------------------------------------------------
d3dContext::d3dContext(d3dDisplay* disp, d3dDevice* dev) : pddiBaseContext(disp, dev)
{
    id = 0;
    display = disp;
    display->AddRef();

    device = dev;
    device->AddRef();


    d3d = display->GetD3DDevice();
    d3dstate = new d3dState(this, d3d);

    viewPort.X = 0;
    viewPort.Y = 0;
    viewPort.Width = 640;
    viewPort.Height = 480;
    viewPort.MinZ = 0.0f;
    viewPort.MaxZ = 1.0f;
    
    zBuffer = NULL;

    stream= NULL;

    beginTime = 0;
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    ticksToMs = 1000.0f / (float)freq.LowPart;

    vertprogs = new d3dVertexProgramManager(this);

    polyClear = false;
    defaultShader = NULL;

    for(int i=0; i < PDDI_MAX_LIGHTS; i++)
        lightStateChanged[i] = false;

    anyLightStateChanged = false;

	extHardwareSkinning = new d3dExtHardwareSkinning(this);
    extGammaControl = new d3dExtGammaControl(display);
    extAntialiasControl = new d3dExtAntialiasControl(this);
    extReadPixels = new d3dExtReadPixels( this );
    defaultShader = new d3dSimpleShader(this);
    defaultShader->AddRef();

    extFramebufferEffects = new d3dExtFramebufferEffects(this);
}

//-------------------------------------------------------------------
d3dContext::~d3dContext()
{
    delete extGammaControl;
    delete extAntialiasControl;
    delete extReadPixels;
	delete extFramebufferEffects;
	delete extHardwareSkinning;

    if(defaultShader)
    {
        defaultShader->Release();
    }

    delete d3dstate;
    delete vertprogs;
    delete stream;

    display->Release();
    device->Release();
}

class d3dTextureValidator
{
public :
    D3DFORMAT* list;
    int position;
    int adapterID;
    D3DDEVTYPE devType;
    D3DFORMAT bufferFormat;
    d3dDisplay* display;

    d3dTextureValidator(D3DFORMAT* l, int a, D3DDEVTYPE t, D3DFORMAT b, d3dDisplay* d)
    {
        list = l;
        position = 0;
        adapterID = a;
        devType = t;
        bufferFormat = b;
        display = d;
    }


    inline void DoValidation(D3DFORMAT format)
    {
        PDDIASSERT(position < MAX_TEXTUREFORMATS);

        if(D3D_OK == display->GetD3D()->CheckDeviceFormat(adapterID, devType, bufferFormat, 0, D3DRTYPE_TEXTURE, format))
        {
            list[position] = format;
            list[(position+1)] = (D3DFORMAT)0xffffffff;
            position++;
        }
    }
};

void d3dContext::DefaultD3DState()
{
    pddiMatrix matrix;
    matrix.Identity();
    d3d->SetTransform(D3DTS_VIEW,       (D3DMATRIX*)&matrix);
    d3d->SetTransform(D3DTS_WORLD,      (D3DMATRIX*)&matrix);
    d3d->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&matrix);

    d3d->SetRenderState(D3DRS_DITHERENABLE,           true);
    d3d->SetRenderState(D3DRS_TEXTUREFACTOR,          0xffffffff);
    d3d->SetRenderState(D3DRS_COLORVERTEX,            true);
    d3d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,  D3DMCS_MATERIAL);
    d3d->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    d3d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,  D3DMCS_MATERIAL);
    d3d->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
}

bool d3dContext::ValidateTexture(D3DFORMAT f)
{
    for(int i = 0; i < MAX_TEXTUREFORMATS; i++)
    {
        if(validTextures[i] == (D3DFORMAT)0xffffffff)
            return false;

        if(validTextures[i] == f)
            return true;
    }
    return false;
}

bool d3dContext::IsHardwareVertexShader( )
{ 
    return display->IsHardwareVertexShader( ); 
}
//-------------------------------------------------------------------
bool d3dContext::Init(void)
{
    D3DDEVICE_CREATION_PARAMETERS params;
    d3d->GetCreationParameters(&params);
    D3DFORMAT bufferFormat = display->GetColourBufferFormat();

    d3dTextureValidator val(validTextures, params.AdapterOrdinal, params.DeviceType, bufferFormat, display);

    val.DoValidation(D3DFMT_A8R8G8B8);
    val.DoValidation(D3DFMT_X8R8G8B8);
    val.DoValidation(D3DFMT_R5G6B5);

    val.DoValidation(D3DFMT_X1R5G5B5);
    val.DoValidation(D3DFMT_A1R5G5B5);
    val.DoValidation(D3DFMT_A4R4G4B4);
    val.DoValidation(D3DFMT_A8);

    val.DoValidation(D3DFMT_P8);

    val.DoValidation(D3DFMT_L8);
    val.DoValidation(D3DFMT_A8L8);

    val.DoValidation(D3DFMT_V8U8);
    val.DoValidation(D3DFMT_L6V5U5);
    val.DoValidation(D3DFMT_X8L8V8U8);
    val.DoValidation(D3DFMT_Q8W8V8U8);
    val.DoValidation(D3DFMT_V16U16);

    val.DoValidation(D3DFMT_UYVY);
    val.DoValidation(D3DFMT_YUY2);
    val.DoValidation(D3DFMT_DXT1);
    val.DoValidation(D3DFMT_DXT2);
    val.DoValidation(D3DFMT_DXT3);
    val.DoValidation(D3DFMT_DXT4);
    val.DoValidation(D3DFMT_DXT5);

    stream = new d3dPrimStream(this);
    DefaultState();
    DefaultD3DState();

    return true;
}

//-------------------------------------------------------------------
void d3dContext::BeginFrame()
{
    if(display->HasReset())
    {
        id++; // tell objects that the D3D context is new
        DefaultD3DState();
        d3dstate->SetDefault();
        SyncState(PDDI_STATE_ALL);
        vertprogs->Reset();
    }

    pddiBaseContext::BeginFrame();
    d3d->BeginScene();
    SetViewPort(0.0f, 0.0f, (float)display->GetWidth(), (float)display->GetHeight());
}

void d3dContext::EndFrame()
{
    pddiBaseShader::ClearCurrentShader();
    pddiBaseContext::EndFrame();
    d3d->EndScene();
}

void d3dContext::DrawSync(void)
{
}

void d3dContext::DetectSkinVertexProcessor(  )
{
    DWORD value;
    d3d->GetRenderState( D3DRS_SOFTWAREVERTEXPROCESSING, &value );
    if( !IsHardwareVertexShader( ) && !value ){
        d3d->SetRenderState( D3DRS_SOFTWAREVERTEXPROCESSING, true );
        d3d->GetRenderState( D3DRS_SOFTWAREVERTEXPROCESSING, &value );
    }
}
void d3dContext::LoadHardwareMatrix(pddiMatrixType id)
{
    ADD_STAT(PDDI_STAT_MATRIX_OPS, 1);
    d3d->SetTransform(matrixTable[id], (D3DMATRIX*)state.matrixStack[id]->Top());
}

void d3dContext::SetupHardwareProjection()
{
    ADD_STAT(PDDI_STAT_MATRIX_OPS, 1);

    pddiCamera& camera = state.viewState->camera;
    pddiMatrix projection;
    projection.Identity();
    
    float width = (float)display->GetWidth();
    float height = (float)display->GetHeight();   
    pddiFloatRect& vw = state.viewState->viewWindow;

    switch(state.viewState->projectionMode)
    {
        case PDDI_PROJECTION_PERSPECTIVE:
        {
            float halfX = (float)tan(double(state.viewState->camera.fov * 0.5)) * state.viewState->camera.nearPlane;
            float halfY = halfX / state.viewState->camera.aspect;
            float nea = camera.nearPlane;
            float fa = camera.farPlane;

            //these numbers represent how much of the viewport is on screen 
            //in a specific direction - they will all be 1.0 unless the viewport 
            //is partially offscreen
            float leftOnscreenPercentage = 1 + 2 * vw.left / ( vw.right - vw.left );
            if( leftOnscreenPercentage > 1 )
            {
                leftOnscreenPercentage = 1;
            }
            float rightOnscreenPercentage = 1 - 2 * ( vw.right - 1 ) / ( vw.right - vw.left );
            if( rightOnscreenPercentage > 1 )
            {
                rightOnscreenPercentage = 1;
            }
            float bottomOnscreenPercentage = 1 + 2 * ( vw.bottom - 1 ) / ( vw.top - vw.bottom );
            if( bottomOnscreenPercentage > 1 )
            {
                bottomOnscreenPercentage = 1;
            }
            float topOnscreenPercentage = 1 - 2 * ( vw.top ) / ( vw.top - vw.bottom );
            if( topOnscreenPercentage > 1 )
            {
                topOnscreenPercentage = 1;
            }

            //this code mimics glFrustum for direct3d
            //the numbers are used to fill the projection matrix
            float r = rightOnscreenPercentage * halfX;
            float l = -leftOnscreenPercentage * halfX;
            float top = topOnscreenPercentage * halfY;
            float bot = -bottomOnscreenPercentage * halfY;
            float a = ( r + l ) / ( r - l );
            float b = ( top + bot ) / ( top - bot );
            float c = fa / (fa-nea);
            float d = -c * nea;
            float e = 2 * nea / ( r - l );
            float f = 2 * nea / ( top - bot );

            projection.Row4(0).Set(e, 0, 0, 0);
            projection.Row4(1).Set(0, f, 0, 0);
            projection.Row4(2).Set(-a, -b, c, 1);
            projection.Row4(3).Set(0, 0, d, 0);

            //clamp the view window because D3D does not support offscreen
            //viewports (even slightly offscreen)
            if( vw.left < 0 )
            {
                vw.left = 0;
            }
            if( vw.right > 1 )
            {
                vw.right = 1;
            }
            if( vw.bottom > 1 )
            {
                vw.bottom = 1;
            }
            if( vw.top < 0 )
            {
                vw.top = 0;
            }

            //set up the viewport
            this->SetViewPort
            ( 
                vw.left * width ,                     
                vw.top * height,
                ( vw.right - vw.left ) * width,
                ( vw.bottom - vw.top ) * height
            );
            break;
        }

        case PDDI_PROJECTION_ORTHOGRAPHIC:
        {
            float l = -0.5f;
            float r = 0.5f;
            float t = -0.5f * (1.0f / camera.aspect);
            float b = 0.5f * (1.0f / camera.aspect);
            float n = camera.nearPlane;
            float f = camera.farPlane;

            projection.m[0][0] = 2.0f / (r - l);
            projection.m[1][1] = 2.0f / (b - t);
            projection.m[2][2] = 1.0f / (f - n);
            projection.m[3][2] = -n / (f-n);

            SetViewPort(vw.left * width,
                            vw.top * height,
                            (vw.right-vw.left) * width,
                            (vw.bottom-vw.top) * height);
            break;
        }

        case PDDI_PROJECTION_DEVICE:
        {
            float l = -0.5f;
            float r = width - 0.5f;
            float t = -0.5f;
            float b = height - 0.5f;
            float n = camera.nearPlane;
            float f = camera.farPlane;

            projection.m[0][0] = 2.0f / (r - l);
            projection.m[1][1] = 2.0f / (t - b);
            projection.m[2][2] = 1.0f / (f - n);
            projection.m[3][2] = -n / (f-n);
            projection.m[3][0] = -1.0f;
            projection.m[3][1] = 1.0f;
            
            SetViewPort(0.0f, 0.0f, width, height);
            break;
        }

        default:
        break;
    }
  
    d3d->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&projection);
}

void d3dContext::SetupHardwareLight(int handle)
{
    anyLightStateChanged = true;
    lightStateChanged[handle] = true;
    lightViewMatrix[handle] = *state.matrixStack[PDDI_MATRIX_MODELVIEW]->Top();
}

void d3dContext::UploadLights(void)
{
    if(!anyLightStateChanged)
    {
        return;
    }

    anyLightStateChanged = false;

    for(int handle = 0; handle < PDDI_MAX_LIGHTS; handle++)
    {
        if(!lightStateChanged[handle])
            continue;

        lightStateChanged[handle] = false;

        pddiLight* l = &state.lightingState->light[handle];
        pddiVector viewPosition, viewDirection;

        // Transform light worldspace vector into viewspace.
        // This lets us use the inverse model-view matrix to go back into object
        // space to do lighting, i.e. the same space as the object's normals.
        viewDirection.Rotate(l->worldDirection, lightViewMatrix[handle]);
        viewPosition.Transform(l->worldPosition, lightViewMatrix[handle]);

        // differences between PDDIs, and D3Ds lighting model:
        // - specular colour == diffuse colour
        // - ambient term is global, not per light
        ADD_STAT(PDDI_STAT_LIGHT_OPS, 1);

        D3DCOLORVALUE black = {0,0,0,0};
        D3DLIGHT8 light;
        light.Type = lightTable[state.lightingState->light[handle].type];
        d3dColourValue(state.lightingState->light[handle].colour, &light.Diffuse);
        light.Specular = light.Diffuse;
        light.Ambient = black;
        light.Position = *(D3DVECTOR*)&viewPosition;
        light.Direction = *(D3DVECTOR*)&viewDirection;
        light.Range = state.lightingState->light[handle].range;
        light.Falloff = state.lightingState->light[handle].falloff;
        light.Attenuation0 = state.lightingState->light[handle].attenA;
        light.Attenuation1 = state.lightingState->light[handle].attenB;
        light.Attenuation2 = state.lightingState->light[handle].attenC;
        light.Theta = state.lightingState->light[handle].theta;
        light.Phi = state.lightingState->light[handle].phi;

        d3d->LightEnable(handle, state.lightingState->light[handle].enabled);
        HRESULT res = d3d->SetLight(handle, &light);
        PDDIASSERT(res == D3D_OK);
    }
}



char buf[255];
int frm = 0;

void d3dContext::BeginTiming()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&beginTime);
}

float d3dContext::EndTiming()
{
    __int64 t;
    QueryPerformanceCounter((LARGE_INTEGER*)&t);
    __int64 diff = t - beginTime;
    return (float)diff * ticksToMs;
}

//-------------------------------------------------------------------
void d3dContext::SetViewPort(float dwX, float dwY, float dwWidth, float dwHeight)
{
    d3d->GetViewport(&viewPort);

    viewPort.X = (DWORD)dwX;
    viewPort.Y = (DWORD)dwY;
    viewPort.Width = (DWORD)dwWidth;
    viewPort.Height = (DWORD)dwHeight;

    d3d->SetViewport(&viewPort);
}

//-------------------------------------------------------------------
void d3dContext::Clear(unsigned bufferMask)
{
    pddiBaseContext::Clear(bufferMask);

    if(polyClear)
    {
        pddiMatrix identity;
        identity.Identity();

        PushMatrix(PDDI_MATRIX_MODELVIEW);
        LoadMatrix(PDDI_MATRIX_MODELVIEW, &identity);
        PushState(PDDI_STATE_VIEW);
        PushState(PDDI_STATE_RENDER);
        PushState(PDDI_STATE_STENCIL);

        SetProjectionMode(PDDI_PROJECTION_DEVICE);
        SetZCompare(PDDI_COMPARE_ALWAYS);

        pddiPrimStream* stream = BeginPrims(NULL, PDDI_PRIM_TRISTRIP, PDDI_V_C, 4);

        stream->Colour(state.viewState->clearColour);
        stream->Coord((float)viewPort.X, (float)viewPort.Y, state.viewState->camera.farPlane);

        stream->Colour(state.viewState->clearColour);
        stream->Coord((float)viewPort.X + (float)viewPort.Width, (float)viewPort.Y, state.viewState->camera.farPlane);

        stream->Colour(state.viewState->clearColour);
        stream->Coord((float)viewPort.X, (float)viewPort.Y + (float)viewPort.Height, state.viewState->camera.farPlane);

        stream->Colour(state.viewState->clearColour);
        stream->Coord((float)viewPort.X + (float)viewPort.Width, (float)viewPort.Y + (float)viewPort.Height, state.viewState->camera.farPlane);

        EndPrims(stream);

        PopState(PDDI_STATE_VIEW);
        PopState(PDDI_STATE_RENDER);
        PopState(PDDI_STATE_STENCIL);
        PopMatrix(PDDI_MATRIX_MODELVIEW);
    }
    else
    {
        DWORD flags = 0;
        bufferMask &= display->GetBufferMask();
        if(bufferMask & PDDI_BUFFER_COLOUR) flags |= D3DCLEAR_TARGET;
        if(bufferMask & PDDI_BUFFER_DEPTH) flags |= D3DCLEAR_ZBUFFER;
        if(bufferMask & PDDI_BUFFER_STENCIL) flags |= D3DCLEAR_STENCIL;
        HRESULT res = d3d->Clear(0, NULL, flags, 
            (D3DCOLOR)state.viewState->clearColour,
            state.viewState->clearDepth,
            state.viewState->clearStencil );
        DDVERIFY(res);
    }
}

pddiPrimStream* d3dContext::BeginPrims(pddiShader* mat, pddiPrimType primType, unsigned vertexType, int vertexCount, unsigned pass)
{
    PDDIASSERT(vertexCount != 0);

    if(!mat)
        mat = defaultShader;

    if(anyLightStateChanged)
        UploadLights();

    stream->Begin(primType, vertexType, (pddiBaseShader*)mat, vertexCount);
    return stream;
}

void d3dContext::EndPrims(pddiPrimStream*)
{
    stream->End();
}

void d3dContext::DrawPrimBuffer(pddiShader* mat, pddiPrimBuffer* buf)
{
    if(!mat)
        mat = defaultShader;

    if(anyLightStateChanged)
        UploadLights();

    pddiBaseShader* material = (pddiBaseShader*)mat;
    d3dPrimBuffer* buffer = (d3dPrimBuffer*)buf;

    pddiVector lVec;
    this->GetLightDirection(0, &lVec); 
    material->SetVector(PDDI_SP_LIGHTV, lVec);
    
    for(int i = 0; i < material->GetPasses(); i++)
    {
        ADD_STAT(PDDI_STAT_MATERIAL_OPS, !material->IsCurrent());
        material->SetMaterial(i);
        buffer->Display();
    }
}

void d3dContext::SetAmbientLight(pddiColour colour)
{
    pddiBaseContext::SetAmbientLight(colour);
    d3d->SetRenderState(D3DRS_AMBIENT, (D3DCOLOR)colour);
}

void d3dContext::ResetCull(void)
{
    d3d->SetRenderState(D3DRS_CULLMODE, cullTable[state.renderState->cullMode]);
}

void d3dContext::SetCullMode(pddiCullMode mode)
{
    pddiBaseContext::SetCullMode(mode);
    if(!d3dstate->IsTwoSided())
    {
        d3d->SetRenderState(D3DRS_CULLMODE, cullTable[mode]);
    }
}

void d3dContext::SetColourWrite(bool red, bool green, bool blue, bool alpha)
{
    pddiBaseContext::SetColourWrite(red, green, blue, alpha);

    unsigned mask = 0;
    if(red) mask |= D3DCOLORWRITEENABLE_RED;
    if(green) mask |= D3DCOLORWRITEENABLE_GREEN;
    if(blue) mask |= D3DCOLORWRITEENABLE_BLUE;
    if(alpha) mask |= D3DCOLORWRITEENABLE_ALPHA;

    d3d->SetRenderState(D3DRS_COLORWRITEENABLE, mask);
}

void d3dContext::EnableZBuffer(bool enabled)
{
    pddiBaseContext::EnableZBuffer(enabled);
    d3d->SetRenderState(D3DRS_ZENABLE, enabled);
}

void d3dContext::SetZCompare(pddiCompareMode compareMode)
{
    pddiBaseContext::SetZCompare(compareMode);
    d3d->SetRenderState(D3DRS_ZFUNC, compareTable[compareMode]);
}

void d3dContext::SetZWrite(bool write)
{
    pddiBaseContext::SetZWrite(write);
    d3d->SetRenderState(D3DRS_ZWRITEENABLE, write);
}

void d3dContext::SetZBias(float bias)
{
    pddiBaseContext::SetZBias(bias);
    d3d->SetRenderState(D3DRS_ZBIAS, (int)bias);
}

void d3dContext::SetZRange(float n, float f)
{
    pddiBaseContext::SetZRange(n, f);
    viewPort.MinZ = n;
    viewPort.MaxZ = f;
    d3d->SetViewport(&viewPort);
}

void d3dContext::EnableStencilBuffer(bool enable)
{
    pddiBaseContext::EnableStencilBuffer(enable);
    d3d->SetRenderState(D3DRS_STENCILENABLE, enable);
}

void d3dContext::SetStencilCompare(pddiCompareMode compare)
{
    pddiBaseContext::SetStencilCompare(compare);
    d3d->SetRenderState(D3DRS_STENCILFUNC, compareTable[compare]);
}

void d3dContext::SetStencilRef(int ref)
{
    pddiBaseContext::SetStencilRef(ref);
    d3d->SetRenderState(D3DRS_STENCILREF, ref);
}

void d3dContext::SetStencilMask(unsigned mask)
{
    pddiBaseContext::SetStencilMask(mask);
    d3d->SetRenderState(D3DRS_STENCILMASK, mask);
}

void d3dContext::SetStencilWriteMask(unsigned mask)
{
    pddiBaseContext::SetStencilWriteMask(mask);
    d3d->SetRenderState(D3DRS_STENCILWRITEMASK, mask);
}

void d3dContext::SetStencilOp(pddiStencilOp failOp, pddiStencilOp zFailOp, pddiStencilOp zPassOp)
{
    pddiBaseContext::SetStencilOp(failOp, zFailOp, zPassOp);
    d3d->SetRenderState(D3DRS_STENCILFAIL, stencilTable[failOp]);
    d3d->SetRenderState(D3DRS_STENCILZFAIL, stencilTable[zFailOp]);
    d3d->SetRenderState(D3DRS_STENCILPASS, stencilTable[zPassOp]);
}

void d3dContext::SetFillMode(pddiFillMode mode)
{
    pddiBaseContext::SetFillMode(mode);
    d3d->SetRenderState(D3DRS_FILLMODE, fillTable[mode]);
}

void d3dContext::EnableFog(bool enable)
{
    pddiBaseContext::EnableFog(enable);
    d3d->SetRenderState(D3DRS_FOGENABLE, enable);
}

void d3dContext::SetFog(pddiColour colour, float start, float end)
{
    pddiBaseContext::SetFog(colour, start, end);
    d3d->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
    d3d->SetRenderState(D3DRS_FOGCOLOR, (D3DCOLOR)colour);
    d3d->SetRenderState(D3DRS_FOGSTART, *((LPDWORD)&start));
    d3d->SetRenderState(D3DRS_FOGEND, *((LPDWORD)&end));
}

unsigned d3dContext::SetRenderTarget(pddiTexture* texture)
{
    IDirect3DSurface8* depth;
    d3d->GetDepthStencilSurface(&depth);

    if (!texture)
    {
        LPDIRECT3DSURFACE8 backBuffer = NULL;
        d3d->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&backBuffer);
        d3d->SetRenderTarget(backBuffer,depth);
        backBuffer->Release();
    }
    else
    {
        d3dTexture* tex = (d3dTexture*)texture;
        PDDIASSERT(tex->IsRenderTarget());
        LPDIRECT3DSURFACE8 surface;
        tex->GetTexture()->GetSurfaceLevel(0, &surface);
        if (surface)
        {
            d3d->SetRenderTarget(surface,depth);
        }
        surface->Release();
    }
    depth->Release();
    return 0;
}

int d3dContext::GetMaxTextureDimension()
{
    return 2048;
}

d3dVertexProgram* d3dContext::GetVertexProgram(const char* name, pddiPrimType primType, unsigned vertexMask, unsigned flags)
{
    return vertprogs->GetVertexProgram(name, primType, vertexMask, flags);
}

void d3dContext::AddVertexProgram( const char* name )
{
    vertprogs->AddVertexProgram( name );
}


void d3dContext::PickVSLights(int* pick)
{
    unsigned dirCount = 0;
    int* use = pick;

    for(int j = 0; j < 4; j++)
    {
        pick[j] = -1;
    }

    for(int i=0; i < PDDI_MAX_LIGHTS; i++)
    {
        pddiLight* l = &state.lightingState->light[i];

        if(l->enabled)
        {
            if(l->type == PDDI_LIGHT_DIRECTIONAL)
            {
                *use = i;
                use++;
                dirCount++;
            }
        }
    }

}

// Set the vertex shader constants. 
void d3dContext::LoadSkinConstants(pddiShader* shader)
{
    UploadLights(); // TODO : use pddi light info, not D3D

    LPDIRECT3DDEVICE8 m_pd3dDevice = GetDisplay()->GetD3DDevice();

    // Some basic constants
    float constants[4] = {0.f, 1.f, 256.0f,0.5f};

    D3DLIGHT8 light0, light1, light2, light3;
    d3dShaderInfo info;

    int pick[4];
    PickVSLights(pick);

    m_pd3dDevice->GetLight((pick[0] == -1) ? 0 : pick[0], &light0);
    m_pd3dDevice->GetLight((pick[1] == -1) ? 0 : pick[1], &light1);
    m_pd3dDevice->GetLight((pick[2] == -1) ? 0 : pick[2], &light2);
    m_pd3dDevice->GetLight((pick[3] == -1) ? 0 : pick[3], &light3);

    ((d3dShader*)shader)->GetShaderInfo(&info);

    D3DCOLORVALUE specular, diffuse, emissive, ambient, ambientLight;
    D3DCOLORVALUE black = {0,0,0,0};

    d3dColourValue(info.specular, &specular);
    d3dColourValue(info.diffuse, &diffuse);
    d3dColourValue(info.emissive, &emissive);
    d3dColourValue(info.ambient, &ambient);
    d3dColourValue(GetAmbientLight(), &ambientLight);

    if(emissive.a == 0.0f)
    {
        emissive.a = diffuse.a;
        diffuse.a = 0.0f;
    }

    emissive.r += (ambient.r * ambientLight.r);
    emissive.g += (ambient.g * ambientLight.g);
    emissive.b += (ambient.b * ambientLight.b);
    emissive.a += (ambient.a * ambientLight.a);

    specular.a = info.shininess;

    // Vertex shader operations use transposed matrices
    pddiMatrix matProj, matProjTranspose;
    m_pd3dDevice->GetTransform( D3DTS_PROJECTION, (D3DMATRIX*)&matProj );
    matProjTranspose.Transpose(matProj);

    // Set the vertex shader constants
    m_pd3dDevice->SetVertexShaderConstant(  0, &constants,    1 );
    m_pd3dDevice->SetVertexShaderConstant(  2, &matProjTranspose, 4 );
    m_pd3dDevice->SetVertexShaderConstant(  6, &diffuse, 1 );
    m_pd3dDevice->SetVertexShaderConstant(  7, &specular, 1 );
    m_pd3dDevice->SetVertexShaderConstant(  8, &emissive,  1 );
    m_pd3dDevice->SetVertexShaderConstant(  9, &light0.Direction,   1 );
    m_pd3dDevice->SetVertexShaderConstant(  10, (pick[0] == -1) ? &black : &light0.Diffuse, 1 );
    m_pd3dDevice->SetVertexShaderConstant(  11, &light1.Direction,   1 );
    m_pd3dDevice->SetVertexShaderConstant(  12, (pick[1] == -1) ? &black : &light1.Diffuse, 1 );
    m_pd3dDevice->SetVertexShaderConstant(  13, &light2.Direction,   1 );
    m_pd3dDevice->SetVertexShaderConstant(  14, (pick[2] == -1) ? &black : &light2.Diffuse, 1 );
    m_pd3dDevice->SetVertexShaderConstant(  15, &light3.Direction,   1 );
    m_pd3dDevice->SetVertexShaderConstant(  16, (pick[3] == -1) ? &black : &light3.Diffuse, 1 );          
}


pddiExtension* d3dContext::GetExtension(unsigned extID)
{
    switch(extID)
    {
		case PDDI_EXT_HARDWARE_SKINNING:
			return extHardwareSkinning;		
        case PDDI_EXT_GAMMACONTROL :
            return extGammaControl;
        case PDDI_EXT_ANTIALIAS_CONTROL :
            return extAntialiasControl;
        case PDDI_EXT_GLOBAL_PROJECTED_TEXTURE :
            return extGlobalProjectedTexture;
        case PDDI_EXT_READ_PIXELS:
            return extReadPixels;
        case PDDI_EXT_FRAMEBUFFER_EFFECTS :
            return extFramebufferEffects;
    }
    return pddiBaseContext::GetExtension(extID);
}

bool d3dContext::VerifyExtension(unsigned extID)
{
    switch(extID)
    {
		case PDDI_EXT_HARDWARE_SKINNING:
        case PDDI_EXT_GAMMACONTROL :
        case PDDI_EXT_ANTIALIAS_CONTROL :
        case PDDI_EXT_GLOBAL_PROJECTED_TEXTURE :
		case PDDI_EXT_FRAMEBUFFER_EFFECTS:
            return true;
    }
    return pddiBaseContext::VerifyExtension(extID);
}