/*===========================================================================
    opticcorona.hpp

    Copyright (c)2000 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#if 0

#ifndef _OPTIC_CORONA_HPP
#define _OPTIC_CORONA_HPP

#include <radmath/radmath.hpp>
#include <p3d/p3dtypes.hpp>
#include <p3d/effects/optic.hpp>
#include <constants/psenum.hpp>

class tShader;

//*****************************************************************************
//
// Class tCorona - halo/flare immediately around object in worldspace
//
//*****************************************************************************
class tCorona: public tOpticEffect
{
public:
    tCorona();

    void SetShader(tShader* mat);
    tShader* GetShader() const             { return shader; }

    void SetColour(tColour c);
    tColour GetColour() const;

    // texture anim of individual flares
    void SetTextureFrameRate(int r)        { if (r>0) textureFrameRate = r; }
    int GetTextureFrameRate() const        { return textureFrameRate; }

    void SetNumTextureFrames(int n)        { if (n>0) numTextureFrames = n; }
    int GetNumTextureFrames() const        { return numTextureFrames; }

    // width of corona,
    void SetWidth(float w)                 { width = (w * 0.5f); }
    float GetWidth() const                 { return (width * 2.0f); }

    // height of corona
    void SetHeight(float h)                { height = (h * 0.5f); }
    float GetHeight() const                { return (height * 2.0f); }

    // set whether to scale size optic effect
    void SetPerspectiveScale(bool scale)         { perspectiveScale = scale; }
    bool GetPerspectiveScale() const             { return perspectiveScale; }

    void SetBillboardMode(unsigned mode)         { billboardMode = mode; }
    unsigned GetBillboardMode() const            { return billboardMode; }

    virtual void ProcessShaders(ShaderCallback&);

protected:
    virtual ~tCorona();

    virtual void DisplayEffect(const rmt::Vector& worldPos, float displayIntensity);

    tShader* shader;
    tColour  colour;
    int      textureFrameRate;
    int      numTextureFrames;
    float    width;
    float    height;
    bool     perspectiveScale;
    unsigned billboardMode;
};

#endif

#endif // #if 0
