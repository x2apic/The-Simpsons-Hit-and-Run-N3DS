//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radmath/radmath.hpp>
#include <stdio.h>


#ifndef RAD_RELEASE
#include <math.h>
#include <assert.h>

#define RMT_MAX_ERROR 0.001

int RadmathSanityCheck()
{
    for(int i=0;i<1024;i++)
    {
        float theta = (i / 128.0f) - 4.0f;
        float s = rmt::Sin(theta);
        float c = rmt::Cos(theta);

        assert( fabs(s-sinf(theta)) < RMT_MAX_ERROR );
        assert( fabs(c-cosf(theta)) < RMT_MAX_ERROR );

        rmt::SinCos(theta,&s,&c);

        assert( fabs(s-sinf(theta)) < RMT_MAX_ERROR );
        assert( fabs(c-cosf(theta)) < RMT_MAX_ERROR );

        float t = rmt::Tan(theta);
        assert( fabs(t-tanf(theta)) < RMT_MAX_ERROR );
    }

    return 0;
}

#endif // RAD_RELEASE

#ifdef RAD_PS2

#include <libdma.h>
#include <libvu0.h>

#ifndef RAD_RELEASE
bool g_rmtInitialized = false;
#endif

int radMathInitialize()
{
    #ifndef RAD_RELEASE
    int ucodeSize = (unsigned)vu0_end - (unsigned)vu0_begin;
    printf("RadMath : VU0 ucode size = %d bytes\n", ucodeSize);
    assert(ucodeSize < 4096 && "VU0 code too big!");    
    #endif

    sceDmaChan* dmaVif0; 

    // upload the VU0 ucode
    dmaVif0 = sceDmaGetChan(SCE_DMA_VIF0);
    dmaVif0->chcr.TTE = 0;	

	sceDmaSend( dmaVif0,vu0_ucode );
	sceDmaSync( dmaVif0,0,0 );

    #ifndef RAD_RELEASE
    g_rmtInitialized = true;
    RadmathSanityCheck();
    printf("RadmathSanityCheck() - passed!\n");
    #endif

    return 0;
}

#else

// todo - add radMathInitiliaze() for other platforms if needed
int radMathInitialize()
{
    #ifndef RAD_RELEASE
    RadmathSanityCheck();
    #endif

    return 0;
}

#endif

