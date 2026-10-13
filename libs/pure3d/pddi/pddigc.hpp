//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PDDIGC_HPP
#define _PDDIGC_HPP

#include <pddi/pddienum.hpp>
#include <pddi/base/debug.hpp>

#include <dolphin/gx.h>
class pddiDevice;
class gcContext;
class gcShader;

// prototype for the initialization function through implicit linking
extern "C" int pddiCreate(int versionMajor, int versionMinor, pddiDevice** dev);

// prototype for the initialization function through LoadLibrary
typedef int (*PDDICREATEPROC)(int, int, pddiDevice**);

// machine dependent types
typedef long long          PDDI_S64;
typedef unsigned long long PDDI_U64;
typedef int                PDDI_S32;
typedef unsigned           PDDI_U32;
typedef char               PDDI_S8;
typedef unsigned char      PDDI_U8;
typedef short              PDDI_S16;
typedef unsigned short     PDDI_U16;

#define PDDI_INTERFACE virtual
#define PDDI_PURE  =0
#define PDDI_VIRTUAL

#define GC_MAX_UV_CHANNELS 4

// display initialization structure
class pddiDisplayInit
{
public:
    int      xsize;         // x resolution (fullscreen only) MAX 640, multiple of 16 pixels
    int      ysize;         // y resolution (fullscreen only) MAX NTSC 480, MAX PAL 576, must be even
    unsigned bufferMask;    // auxilliary buffer mask
    bool     pal;           // true for pal
    bool     fieldmode;     // true for interlaced display ysize <= MAXY / 2
    bool     aa;            // true for anti-alising, only works if fieldmode && ysize <= 264
    bool     progressive;   // true for non-interlaced display
    bool     lockToVsync;   // true if framebuffer swap waits for vsync
};

// alpha blending modes
// these map to the values of the ALPHA_1,ALPHA_2 registers
enum pddiColourFactor
{
    PDDI_CF_SRC,
    PDDI_CF_DEST,
    PDDI_CF_ZERO,
    PDDI_CF_RESERVED
};

enum pddiAlphaFactor
{
    PDDI_AF_SRC,
    PDDI_AF_DEST,
    PDDI_AF_FIX,
    PDDI_AF_RESERVED
};

#include <pddi/GameCube/gcPrimStream.hpp>

#endif /* _PDDIGCPC_HPP */

