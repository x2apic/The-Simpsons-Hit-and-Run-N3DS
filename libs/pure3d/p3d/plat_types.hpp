//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _BASE_PLAT_TYPES_HPP
#define _BASE_PLAT_TYPES_HPP

#ifdef RAD_PS2
    #include <p3d/platform/ps2/plat_types.hpp>
#endif

#ifdef RAD_LINUX
    #include <p3d/platform/linux/plat_types.hpp>
#endif

#ifdef RAD_XBOX
    #include <p3d/platform/xbox/plat_types.hpp>
#endif

#ifdef RAD_GAMECUBE
    #include <p3d/platform/GameCube/plat_types.hpp>
#endif

#ifdef RAD_WIN32
    #include <p3d/platform/win32/plat_types.hpp>
#endif


#endif

