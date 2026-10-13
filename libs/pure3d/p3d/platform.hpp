//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _BASE_PLATFORM_HPP
#define _BASE_PLATFORM_HPP

#ifdef RAD_PS2
    #include <p3d/platform/ps2/platform.hpp>
#endif

#ifdef RAD_LINUX
    #include <p3d/platform/linux/platform.hpp>
#endif

#ifdef RAD_XBOX
    #include <p3d/platform/xbox/platform.hpp>
#endif

#ifdef RAD_GAMECUBE
    #include <p3d/platform/GameCube/platform.hpp>
#endif

#ifdef RAD_WIN32
    #include <p3d/platform/win32/platform.hpp>
#endif


#endif

