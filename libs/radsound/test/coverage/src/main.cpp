//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#ifdef RAD_WIN32
#include <windows.h>
#endif 

#include "sonyreaddma.hpp"
#include "streamthrash.hpp"
#include "clipthrash.hpp"
#include "positional.hpp"
#include "effects.hpp"

#ifdef RAD_XBOX
#include "xboxwma.hpp"
#endif 

#ifdef RAD_WIN32

HINSTANCE g_hInstance;

int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
	int argc = 0;
	char * argv[] = { "" };

	g_hInstance = hInstance;
#else
int main( int argc, char * argv[] )
{
#endif
    // return sonyReadDmaTest( argc, argv );
    // return streamThrashTest( argc, argv );
    // return clipThrashTest( argc, argv );
    return positionalTest( argc, argv );
    // return effectsTest( argc, argv );
    // return xboxWmaTest(argc, argv );
}
