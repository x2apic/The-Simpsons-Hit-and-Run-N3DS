//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radsound_hal.hpp>

#ifdef RAD_PS2
    #define SOUND_MEMORY 0
#else
    #define SOUND_MEMORY (1024*1024*4)
#endif

#ifdef RAD_WIN32
    #define SAMPLING_RATE 48000
    #define IS_STICKY_FOCUS_ENABLED false;
#else
    #define SAMPLING_RATE 0
#endif

#define MAX_ROOT_ALLOCATIONS 50
#define AUX_SENDS 2

void rstRadSoundInitialize( void )
{
    ::radSoundHalSystemInitialize( RADMEMORY_ALLOC_DEFAULT );

    IRadSoundHalSystem::SystemDescription systemDescription;
    ::memset( ( void * ) & systemDescription, 0, sizeof( systemDescription ) );

    systemDescription.m_MaxRootAllocations = MAX_ROOT_ALLOCATIONS;
    systemDescription.m_NumAuxSends = AUX_SENDS;

    #ifndef RAD_PS2
    systemDescription.m_ReservedSoundMemory = SOUND_MEMORY;
    #endif 

    #ifdef RAD_WIN32
    systemDescription.m_SamplingRate = SAMPLING_RATE;
    systemDescription.m_EnableStickyFocus = IS_STICKY_FOCUS_ENABLED;
    #endif

    ::radSoundHalSystemGet( )->Initialize( systemDescription );
}

void rstRadSoundService( void )
{
    ::radSoundHalSystemGet( )->Service( );
    ::radSoundHalSystemGet( )->ServiceOncePerFrame( );
}

void rstRadSoundTerminate( void )
{
    radSoundHalSystemTerminate( );
}