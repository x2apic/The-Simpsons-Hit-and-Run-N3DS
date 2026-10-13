//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <windows.h>

#include "ods/ods.hpp"

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    hOdsProject proj = odsCreateProject( );
    odsDestroyProject( & proj );
    
    return 0;
    
}

