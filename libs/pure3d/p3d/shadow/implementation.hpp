//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SHADOW_IMPLIMENTATION_HPP_
#define _SHADOW_IMPLIMENTATION_HPP_


#if defined (RAD_WIN32) || defined(RAD_XBOX)
#define CARMACK_REVERSE // use inverted z-buffer mode 
#define DRAW_CAPS // draw caps on volumes
#endif

#if defined (RAD_PS2) || defined (RAD_GAMECUBE)
//#define CARMACK_REVERSE // use inverted z-buffer mode 
//#define DRAW_CAPS // draw caps on volumes
#endif


#ifdef RAD_PS2
#include <p3d/shadow/implementation_ps2.hpp>
#else
#include <p3d/shadow/implementation_common.hpp>
#endif


#endif