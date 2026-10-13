//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADMOIVE_PCH_HPP
#define RADMOIVE_PCH_HPP

//
// only win32 and xbox take advantage of the pch at this moment.
//
#if defined RAD_WIN32 || defined RAD_XBOX
//
// Microsoft header files
//
#ifdef WIN32
#define WINVER 0x0501
#define _WIN32_WINNT 0x400
#include <windows.h>
#include <windows.h>

#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <evcode.h>
#include <vfwmsgs.h>
#endif

#ifdef RAD_XBOX
#include <xtl.h>
#endif

//
// Standard C header files
//

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

//
// FTech header files
//
#include "radoptions.hpp"

#endif // defined RAD_WIN32 || defined RAD_XBOX

#endif // RADMOIVE_PCH_HPP
