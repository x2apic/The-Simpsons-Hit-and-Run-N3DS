//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#include "pch.hpp"
#include <raddebug.hpp>
#include "platalloc.hpp"

#if defined RAD_PS2

    #include <malloc.h>

#elif defined WIN32 || defined RAD_XBOX
    
    #include <stdlib.h>
    #include <malloc.h>
    
    #if defined MALLOC_DEBUG
        #include <crtdbg.h>
    #endif

#elif defined RAD_GAMECUBE

    #include <dolphin.h>
    #include <dolphin/vm.h>

static bool sVMMInitialized = false;
OSHeapHandle gGCHeap = -1;

#elif defined RAD_3DS
    #include <stdlib.h>
    #include <3ds.h>
#endif

//============================================================================
// ::radMemoryPlatInitialize
//============================================================================

#ifdef RAD_GAMECUBE
void radMemoryPlatInitialize( unsigned int sizeVMMainMemory, unsigned int sizeVMARAM)
{
    // Looking for some VMM?
    sVMMInitialized = false;
    if ((sizeVMMainMemory != 0) && (sizeVMARAM != 0))
    {
        unsigned int baseVMARAM = (1024 * 1024 * 16) - sizeVMARAM;

        VMInit(sizeVMMainMemory, baseVMARAM, sizeVMARAM);
        sVMMInitialized = true;
    }

	// Initializes the Dolphin OS memory allocator and ensures that
	// new and delete will work properly.

	void *pArenaLo = OSGetArenaLo( );
	void *pArenaHi = OSGetArenaHi( );

	// Create a heap
	// OSInitAlloc should only ever be invoked once.

	pArenaLo = OSInitAlloc( pArenaLo, pArenaHi, 1); // 1 heap size
	OSSetArenaLo( pArenaLo );

	// Ensure boundaries are 32B aligned

	pArenaLo = (void*) OSRoundUp32B( pArenaLo );
	pArenaHi = (void*) OSRoundDown32B( pArenaHi );

	// The boundaries given to OSCreateHeap should be 32B aligned
    gGCHeap = OSCreateHeap( pArenaLo, pArenaHi );
    rAssert( gGCHeap != -1 );
	OSSetCurrentHeap( gGCHeap );

	//
	// From here on out, OSAlloc and OSFree behave like malloc and free
	// respectively.
	//
	OSSetArenaLo( pArenaLo = pArenaHi );
}
#else
// ---Non-GameCube Memory Plat init --------------------------------------------------
void radMemoryPlatInitialize( void )
{

}
#endif


//============================================================================
// ::radMemoryPlatTerminate
//============================================================================

void radMemoryPlatTerminate( void )
{
#ifdef RAD_GAMECUBE
    if (sVMMInitialized)
    {
        VMQuit();
        sVMMInitialized = false;
    }

#endif
}

//============================================================================
// ::radMemoryPlatAlloc
//============================================================================

#ifdef RAD_3DS
static RecursiveLock s_radPlatLock;
static bool s_radPlatLockReady = false;

static inline void radPlatLock( void )
{
    if( !s_radPlatLockReady )
    {
        RecursiveLock_Init( &s_radPlatLock );
        s_radPlatLockReady = true;
    }
    RecursiveLock_Lock( &s_radPlatLock );
}

static inline void radPlatUnlock( void )
{
    RecursiveLock_Unlock( &s_radPlatLock );
}
#define RAD_PLAT_LOCK()   radPlatLock()
#define RAD_PLAT_UNLOCK() radPlatUnlock()
#else
#define RAD_PLAT_LOCK()
#define RAD_PLAT_UNLOCK()
#endif

void * radMemoryPlatAlloc( unsigned int numberOfBytes )
{
    void * pMemory;
    //
    // C++ standard says you can allocate 0 byte memory object.
    //
    if ( numberOfBytes == 0 )
    {
        numberOfBytes = 1;
    }

    RAD_PLAT_LOCK();
    pMemory = malloc( numberOfBytes );
    RAD_PLAT_UNLOCK();

    rWarningMsg( pMemory != NULL, "radMemory: Platform (malloc) allocator failed to allocate memory\n" );
    return pMemory;
}

//============================================================================
// ::radMemoryPlatFree
//============================================================================

void radMemoryPlatFree( void * pMemory )
{
    RAD_PLAT_LOCK();
    free( pMemory );
    RAD_PLAT_UNLOCK();
}

//============================================================================
// ::radMemoryPlatAllocAligned
//============================================================================

void * radMemoryPlatAllocAligned( unsigned int numberOfBytes, unsigned int alignment )
{
	#if defined RAD_3DS
		if( alignment < sizeof( void * ) )
		{
			alignment = sizeof( void * );
		}
		unsigned int rounded = ( numberOfBytes + alignment - 1 ) & ~( alignment - 1 );
		if( rounded > ( 16u * 1024u * 1024u ) || rounded < numberOfBytes )
		{
			return NULL;
		}
		RAD_PLAT_LOCK();
		void * p = ::aligned_alloc( alignment, rounded );
		RAD_PLAT_UNLOCK();
		return p;
	#elif !defined WIN32

		return ::aligned_alloc( alignment, numberOfBytes );

	#else

        return _aligned_malloc( numberOfBytes, alignment );

	#endif
}

//============================================================================
// ::radMemoryPlatFreeAligned
//============================================================================

void radMemoryPlatFreeAligned( void * pAlignedMemory )
{

	#ifndef WIN32
		
		RAD_PLAT_LOCK();
		free( pAlignedMemory );
		RAD_PLAT_UNLOCK();
	
	#else

        _aligned_free( pAlignedMemory );

	#endif
}
