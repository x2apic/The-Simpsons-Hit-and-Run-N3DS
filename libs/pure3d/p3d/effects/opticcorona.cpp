/*===========================================================================
    opticcorona.cpp

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
/*
#include <p3d/utility.hpp>
#include <p3d/shader.hpp>
#include <p3d/view.hpp>
#include <p3d/camera.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/effects/opticconstants.hpp>
#include <p3d/effects/opticcorona.hpp>

//*****************************************************************************
//
// tCorona::tCorona
//
//*****************************************************************************
tCorona::tCorona():
    shader(NULL),
    colour(255, 255, 255, 255),
    textureFrameRate(1),
    numTextureFrames(1),
    width(1.0f),
    height(1.0f),
    perspectiveScale(true),
    billboardMode(p3dBillboardConstants::BillboardMode::ALL_AXIS_AAX)
{
}

//*****************************************************************************
//
// tCorona::~tCorona
//
//*****************************************************************************
tCorona::~tCorona()
{
    tRefCounted::Release(shader);
}

//*****************************************************************************
//
// tCorona::SetColour
//
//*****************************************************************************
void tCorona::SetColour(tColour c)
{
#ifdef RAD_PS2
    colour.Set(c.Red() / 2, c.Green() / 2, c.Blue() / 2, c.Alpha());
#else
    colour = c;
#endif
}

//*****************************************************************************
//
// tCorona::GetColour
//
//*****************************************************************************
tColour tCorona::GetColour() const
{
#ifdef RAD_PS2
    return tColour(colour.Red() * 2, colour.Green() * 2, colour.Blue() * 2, colour.Alpha());
#else
    return colour;
#endif
}

//*****************************************************************************
//
// tCorona::SetShader
//
//*****************************************************************************
void tCorona::SetShader(tShader* mat)
{
    tRefCounted::Assign(shader, mat);
}

void tCorona::ProcessShaders(ShaderCallback& callback)
{
    tRefCounted::Assign(shader, callback.Process(shader));
}

//*****************************************************************************
//
// tCorona::DisplayEffect
//
//*****************************************************************************
void tCorona::DisplayEffect(const rmt::Vector& worldPos, float displayIntensity)
{
    // calculate vertex colours
#if defined(PS2MW) || defined(RAD_GAMECUBE)
    pddiColour modulatedColour(0,0,0,0); // jan10/2002 amb - stupid warning from MW
#else
    pddiColour modulatedColour;
#endif
    modulatedColour.SetRed((int)(displayIntensity * colour.Red()));
    modulatedColour.SetGreen((int)(displayIntensity * colour.Green()));
    modulatedColour.SetBlue((int)(displayIntensity * colour.Blue()));
    modulatedColour.SetAlpha(colour.Alpha());

    int texFrame = (rmt::FtoL(frame) / textureFrameRate) % numTextureFrames;
    float uInc = 1.0f/rmt::LtoF(numTextureFrames);
    float uL = uInc * rmt::LtoF(texFrame);
    float uR = uL + uInc;
  
    float x = width;
    float y = height;
      
    if (billboardMode == p3dBillboardConstants::BillboardMode::ALL_AXIS_AAX)
    {
        rmt::Matrix transform = p3d::context->GetView()->GetCamera()->GetWorldToCameraMatrix();
        rmt::Vector currPos;
        currPos.x = transform.m[0][0]*worldPos.x + transform.m[1][0]*worldPos.y + transform.m[2][0]*worldPos.z + transform.m[3][0];
        currPos.y = transform.m[0][1]*worldPos.x + transform.m[1][1]*worldPos.y + transform.m[2][1]*worldPos.z + transform.m[3][1];
        currPos.z = transform.m[0][2]*worldPos.x + transform.m[1][2]*worldPos.y + transform.m[2][2]*worldPos.z + transform.m[3][2];

        if (!perspectiveScale)
        {
            x *= currPos.z;
            y *= currPos.z;
        }

        // draw the quad
        p3d::stack->Push();
        p3d::stack->LoadIdentity();

        pddiPrimStream* stream = p3d::pddi->BeginPrims(shader->GetShader(), PDDI_PRIM_TRISTRIP, PDDI_V_CT, 4);

        stream->Colour(modulatedColour);
        stream->UV(uL, 1.0f);
        stream->Coord(currPos.x-x, currPos.y+y, currPos.z);

        stream->Colour(modulatedColour);
        stream->UV(uR, 1.0f);
        stream->Coord(currPos.x+x, currPos.y+y, currPos.z);

        stream->Colour(modulatedColour);
        stream->UV(uL, 0.0f);
        stream->Coord(currPos.x-x, currPos.y-y, currPos.z);

        stream->Colour(modulatedColour);
        stream->UV(uR, 0.0f);
        stream->Coord(currPos.x+x, currPos.y-y, currPos.z);

        p3d::pddi->EndPrims(stream);

        p3d::stack->Pop();
    }
    else
    {
        rmt::Vector cameraPos;
        rmt::Matrix transform;

        p3d::context->GetView()->GetCamera()->GetWorldPosition(&cameraPos);
        cameraPos.x = -cameraPos.x;
        cameraPos.y = -cameraPos.y;
        cameraPos.z = -cameraPos.z;
        transform.Identity();

        switch (billboardMode)
        {
        case p3dBillboardConstants::BillboardMode::X_AXIS_XAX:
            {
                transform.FillHeadingYZ(cameraPos);
                break;
            }
        case p3dBillboardConstants::BillboardMode::Y_AXIS_YAX:
            {
                transform.FillHeadingXZ(cameraPos);
                break;
            }
        case p3dBillboardConstants::BillboardMode::Z_AXIS_ZAX:
            {
                transform.FillHeadingXY(cameraPos);
                break;
            }
        default:
            {
                break;
            }
        }

        if (!perspectiveScale)
        {
            rmt::Matrix transform = p3d::context->GetView()->GetCamera()->GetWorldToCameraMatrix();
            float z = transform.m[0][2]*worldPos.x + transform.m[1][2]*worldPos.y + transform.m[2][2]*worldPos.z + transform.m[3][2];
            x *= z;
            y *= z;
        }

        // draw the quad
        p3d::stack->Push();
        p3d::stack->Multiply(transform);

        pddiPrimStream* stream = p3d::pddi->BeginPrims(shader->GetShader(), PDDI_PRIM_TRISTRIP, PDDI_V_CT, 4);

        stream->Colour(modulatedColour);
        stream->UV(uL, 1.0f);
        stream->Coord(-x, y, 0.0f);

        stream->Colour(modulatedColour);
        stream->UV(uR, 1.0f);
        stream->Coord(x, y, 0.0f);

        stream->Colour(modulatedColour);
        stream->UV(uL, 0.0f);
        stream->Coord(-x, -y, 0.0f);

        stream->Colour(modulatedColour);
        stream->UV(uR, 0.0f);
        stream->Coord(x, -y, 0.0f);

        p3d::pddi->EndPrims(stream);   

        p3d::stack->Pop();   
    }
}

*/