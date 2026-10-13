//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


/* main.cpp */

#include <radplatform.hpp>
#include <radthread.hpp>

#if defined RAD_WIN32
#include "window.hpp"
#endif

bool g_Done = false;

#define RADMEMORY_ALLOC_HEAP1               ( RADMEMORY_ALLOC_TEMP + 1 )
#define RADMEMORY_ALLOC_HEAP2               ( RADMEMORY_ALLOC_TEMP + 2 )
#define RADMEMORY_ALLOC_HEAP1_CHILD1        ( RADMEMORY_ALLOC_TEMP + 3 )
#define RADMEMORY_ALLOC_HEAP1_CHILD2        ( RADMEMORY_ALLOC_TEMP + 4 )
#define RADMEMORY_ALLOC_HEAP1_CHILD1_CHILD1 ( RADMEMORY_ALLOC_TEMP + 5 )

#define ONE_K 1024 
#define NUM_BINS 15

void msInstallHeap( radMemoryAllocator child,
    radMemoryAllocator parent, unsigned int size )
{
    parent = RADMEMORY_ALLOC_DEFAULT;

    /* IRadMemoryHeap * pIRadMemoryHeap
        = radMemoryCreateDougLeaHeap( size, parent ); */

    IRadMemoryBinAllocator::Bin bins[ NUM_BINS ];

    for( unsigned int bin = 0; bin < NUM_BINS; bin++ )
    {
        bins[ bin ].m_ElementSizeInBytes = ( 1 << ( bin + 3 ) ) + ( rand() % 1024 );
        bins[ bin ].m_NumberOfElements = (NUM_BINS - bin ) * 100;
    }

    ref< IRadMemoryBinAllocator > xIRadMemoryBinAllocator;
    ::radMemoryBinAllocatorCreate( & xIRadMemoryBinAllocator,
        bins, NUM_BINS, parent );

    ::radMemoryRegisterAllocator( child, parent, xIRadMemoryBinAllocator );
}

void msInstallHeaps( void )
{
    msInstallHeap( RADMEMORY_ALLOC_HEAP1, RADMEMORY_ALLOC_DEFAULT, ONE_K * ONE_K );
    msInstallHeap( RADMEMORY_ALLOC_HEAP2, RADMEMORY_ALLOC_DEFAULT, ONE_K * ONE_K);
    msInstallHeap( RADMEMORY_ALLOC_HEAP1_CHILD1, RADMEMORY_ALLOC_HEAP1, ONE_K * 128 );
    msInstallHeap( RADMEMORY_ALLOC_HEAP1_CHILD2, RADMEMORY_ALLOC_HEAP2, ONE_K * 128 );
    msInstallHeap( RADMEMORY_ALLOC_HEAP1_CHILD1_CHILD1, RADMEMORY_ALLOC_HEAP1_CHILD1, ONE_K * 8 );
}

void msUnInstallHeaps( void )
{
    ::radMemoryUnregisterAllocator( RADMEMORY_ALLOC_HEAP1_CHILD1_CHILD1 );
    ::radMemoryUnregisterAllocator( RADMEMORY_ALLOC_HEAP1_CHILD2 );
    ::radMemoryUnregisterAllocator( RADMEMORY_ALLOC_HEAP1_CHILD1 );
    ::radMemoryUnregisterAllocator( RADMEMORY_ALLOC_HEAP2 );
    ::radMemoryUnregisterAllocator( RADMEMORY_ALLOC_HEAP1 );
}

#define NUM_ALLOCATIONS ( 1024 )

void OutOfMemoryCallback( void * pUserData, radMemoryAllocator allocator )
{
    rAssert( pUserData == (void*) 0x12345678 );

   /* rReleasePrintf( "Trapped out of memory, allocator: [%d]\n",
        allocator ); */
}

void msMain( void )
{
    ::radMemorySetOutOfMemoryCallback( OutOfMemoryCallback, (void*) 0x12345678 );

    // little test to make sure current allcoator flag is functional

    rAssert( radMemoryGetCurrentAllocator() == RADMEMORY_ALLOC_DEFAULT );
    ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_HEAP1_CHILD1_CHILD1 );
    rAssert( ::radMemoryGetCurrentAllocator( ) == RADMEMORY_ALLOC_HEAP1_CHILD1_CHILD1 );
    rAssert( ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_DEFAULT ) == RADMEMORY_ALLOC_HEAP1_CHILD1_CHILD1 );

    for( unsigned int times = 0; times < 100; times ++ )
    {
        msInstallHeaps( );

        void *             pointers[   NUM_ALLOCATIONS ];
        radMemoryAllocator allocators[ NUM_ALLOCATIONS ];
        unsigned int       bytes[      NUM_ALLOCATIONS ];
        bool               aligned[    NUM_ALLOCATIONS ];

        for( unsigned int i = 0; i < NUM_ALLOCATIONS; i ++ )
        { 
            bytes[ i ]        = bytes[ i ] = rand() % 1000; 
            allocators[ i ]   = RADMEMORY_ALLOC_TEMP +
                ( rand() % ( RADMEMORY_ALLOC_HEAP1_CHILD1_CHILD1 - RADMEMORY_ALLOC_HEAP1 + 1 ) + 1 );
            aligned[ i ]      = ( ( rand() % 2 ) == 0 );

            if ( aligned[ i ] == false )
            {
                pointers[ i ]     = ::radMemoryAlloc( allocators[ i ], bytes[ i ] );
            }
            else
            {
                pointers[ i ]     = ::radMemoryAllocAligned( allocators[ i ], bytes[ i ], ( (rand( ) % 32 ) * 4));
            }
        }

        for ( unsigned int j = 0; j < NUM_ALLOCATIONS; j ++ )
        {
            switch ( rand() % 3 )
            {
                case 0 :
                    if ( aligned[ j ] )
                    {
                        radMemoryFreeAligned( pointers[ j ] );
                    }
                    else
                    {
                        radMemoryFree( pointers[ j ] );
                    }
                    break;
                case 1 :
                    if ( aligned[ j ] )
                    {
                        radMemoryFreeAligned( allocators[ j ], pointers[ j ] );
                    }
                    else
                    {
                        radMemoryFree( allocators[ j ], pointers[ j ] );
                    }
                    break;
                case 2 :

                        IRadMemoryAllocator * pAllocator = radMemoryFindAllocator( pointers[ j ] );
                        IRadMemoryAllocator * pRightAllocator = ::radMemoryGetAllocator( allocators[ j ] );

                        if ( pointers[ j ] != NULL ) // System can't tell who allocated a null pointer!
                        {
                            rAssert( pAllocator == pRightAllocator );
                        }

                        rAssert( pAllocator != NULL );

                        if ( aligned[ j ] )
                        {
                            pAllocator->FreeMemoryAligned( pointers[ j ] );
                        }
                        else
                        {
                            pAllocator->FreeMemory( pointers[ j ] );
                        }
 
                    break;
            }
        }

        msUnInstallHeaps( );
    }
    
    ::radMemorySetOutOfMemoryCallback( NULL, NULL );        
}

#if defined WIN32 && ! defined _XBOX
	int WINAPI WinMain(
	  HINSTANCE hInstance,  // handle to current instance
	  HINSTANCE hPrevInstance,  // handle to previous instance
	  LPSTR lpCmdLine,      // pointer to command line
	  int nCmdShow          // show state of window
	) 
#else
	int main( void )
#endif
{
    srand( 0 );

    ::radThreadInitialize( );
    ::radMemoryInitialize( );

    #if defined WIN32 && ! defined _XBOX
	    ::radPlatformInitialize(
		    ::CreateMainWindow( hInstance, SW_SHOWNORMAL ),
			    hInstance, RADMEMORY_ALLOC_DEFAULT );
    #elif defined RAD_GAMECUBE
        ::radPlatformInitialize( );
    #elif defined _XBOX
        ::radPlatformInitialize( );
    #else
	    ::radPlatformInitialize( "irx/", IOPMediaHost, GameMediaCD, NULL, RADMEMORY_ALLOC_DEFAULT );
    #endif

	msMain( );

    ::radPlatformTerminate( );
    ::radMemoryTerminate( );
    ::radThreadTerminate( );

#ifdef RAD_DEBUG
	::radObject::DumpObjects( );
#endif

	return 0;
}