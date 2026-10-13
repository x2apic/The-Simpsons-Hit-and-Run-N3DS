//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#ifndef _GXM_HPP_
#define _GXM_HPP_

// stub GXM header, all pddi gxm code uses this instead of '#include <psp2/gxm.h>

#include <psp2/gxm.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/sysmem.h>
#include <pddi/base/debug.hpp>

#ifdef PDDI_USE_ASSERTS
bool pddiGxmAssert(const char* file, int line, unsigned int err, const char* cond = nullptr);
#define CHK_GXM(c) pddiGxmAssert(__FILE__, __LINE__, (c), #c)
#else
#define CHK_GXM(c) ((c) == SCE_OK)
#endif

#endif
