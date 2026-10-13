//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef PLATALLOC_HPP
#define PLATALLOC_HPP

#ifdef RAD_GAMECUBE
void   radMemoryPlatInitialize( unsigned int sizeVMMainMemory, unsigned int sizeVMARAM );
#else
void   radMemoryPlatInitialize( void );
#endif
void   radMemoryPlatTerminate( void );

void * radMemoryPlatAlloc( unsigned int size );
void * radMemoryPlatAllocAligned( unsigned int size, unsigned int alignment );
void   radMemoryPlatFree( void * pMemory );
void   radMemoryPlatFreeAligned( void * pMemory );

#endif // PLATALLOC_HPP