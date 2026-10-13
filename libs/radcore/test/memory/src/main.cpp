//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:   Radical memory system test
//
// Description:	This file contains code to test various memory operations using
//                radmemory.
//
// Revisions:	V1.00	Dec 03, 2001 Christopher Vienneau
//
//=============================================================================


//=============================================================================
// Include Files
//=============================================================================

#include <stdlib.h>
#include <string.h>
#include <raddebug.hpp>
#include <radfile.hpp>
#include <radtime.hpp>
#include <raddebugcommunication.hpp>
#include <radmemory.hpp>
#include <radplatform.hpp>
#include <radstats.hpp>

#include <radstring.hpp>


//============================================================================
// Local Functions
//=============================================================================


//=============================================================================
// Function:    radStartup
//=============================================================================
// Description: Startup all systems needed by this sample.
//
// Parameters:  banner - a banner to display
//
//------------------------------------------------------------------------------

#if defined( RAD_WIN32 ) && !defined( RAD_XBOX )
void radStartup( const char* banner, HINSTANCE hInstance )
#else
void radStartup( const char* banner )
#endif
{
    //
    // Display a banner message.
    //
    rDebugString( banner );
    rDebugString( "\n" );
    
    //
    // First initialized the platform. Each platform has specific initialization,
    // PS2 requires path to where IRXs reside and where to load them form. We
    // also intialize the debug communication system. This is required if the
    // Radical File Server will be used.
    //
		radMemoryInitialize();

    #ifdef RAD_PS2EE
    radPlatformInitialize( "\\usr\\local\\sce\\iop\\modules\\", IOPMediaHost );
//    radDbgComTargetInitialize( Deci );
    #endif

    #ifdef RAD_WIN32
    #ifndef RAD_XBOX
    //
    // Windows initialization requires instance and main window. This program does
    // not use a main window, so just set it to NULL,
    //
    radPlatformInitialize( NULL, hInstance );
//    radDbgComTargetInitialize( WinSocket );
    #else
    //
    // XBOX does not need any info.
    //
    radPlatformInitialize( );
//    radDbgComTargetInitialize( WinSocket );
    #endif
    #endif

    //
    // Game Cube
    //
    #ifdef RAD_GAMECUBE
    radPlatformInitialize( );
//    radDbgComTargetInitialize( HostIO );
    #endif
}


//=============================================================================
// Function:    radShutdown
//=============================================================================
// Description: Shutdown all systems used by this sample.
//
// Parameters:  banner - a banner to display
//
//------------------------------------------------------------------------------

void radShutdown( const char* banner )
{
    //
    // Finally clean up.
    //
//    radDbgComTargetTerminate( );
    radPlatformTerminate( );
	radMemoryTerminate();

    //
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
#ifdef RAD_DEBUG
    radObject::DumpObjects( );    
#endif
    
    rDebugString( banner );
    rDebugString( "\n" );
}

//=============================================================================
// Public Functions
//=============================================================================

//
// Gamecube currently requires that operators new and delete be implemented.
// We do it for all plaforms just for consistency.
//
void* operator new( size_t size )
{
    return( radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, size ) );
}

void* operator new[ ]( size_t size )
{
    return( radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, size ) );
}

void operator delete( void* ptr )
{
    radMemoryFree( ptr );
}

inline void operator delete[ ]( void* ptr )
{
    radMemoryFree( ptr );
}

//=============================================================================
// Function:    main
//=============================================================================
// Description: Main entry point. Platform specific entry.
//
// Parameters:  see Windows.
//
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_WIN32
int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
#endif

#ifdef RAD_XBOX
VOID _cdecl main (void)
#endif

#ifdef RAD_PS2
int main( int argc, char* argv[ ] )
#endif

#ifdef RAD_GAMECUBE
void main( void )
#endif
{
    //
    // Startup all necessary systems (this is a locally defined function)
    //
    #if defined( RAD_WIN32 ) && !defined( RAD_XBOX )
    ::radStartup( "Foundation Tech - Radical File Timer V1.00", hInstance );
    #else
    ::radStartup( "Foundation Tech - Radical File Timer V1.00" );
    #endif

    //
    // Print standard benchmark disclaimer.
    //
    rReleaseString( "\n==================================================\n" );
    rReleaseString( "|This program will test the radmemory system     |.\n" );
    rReleaseString( "==================================================\n" );

    //
    // Get memory alignment,
    //
    unsigned int memAlign = radFileOptimalMemoryAlignment;
    rReleasePrintf( "Optimal memory alignment for this platform is %u.\n", memAlign );

////////////////////////////////////////////
//TEST START
////////////////////////////////////////////

   rReleaseString( "**radMemoryAlloc test\n" );
   int i = 0; //for all loops couters
   radMemoryAllocator myAllocator = RADMEMORY_ALLOC_DEFAULT;
//   radMemoryRegisterAllocator(myAllocator,RADMEMORY_ALLOC_DEFAULT);
 //INCRIMENTAL ALLOCATIONS
    
   int * testBuffer = NULL;            //pointer to allocated memory
   int BufferSize = 1024 * 1024 * 5;   //original buffer size for first allocation
   int numIterations = 20;
 
   for( i = 1; i < numIterations; i ++){
      //testBuffer = ( int * )radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, BufferSize );
      testBuffer = (int * )radMemoryAllocAligned( myAllocator, BufferSize, memAlign);
      testBuffer[i] = i;

      if((testBuffer == NULL) || (testBuffer[i] != i)){
         rReleasePrintf("%d Allocation of testBuffer FAILED (%dK) \n",i, BufferSize/1024);
      }
      else{
         rReleasePrintf("%d Allocation of testBuffer Succeeded (%dK) \n",i, BufferSize/1024);
      }
      //radMemoryFree(RADMEMORY_ALLOC_DEFAULT, testBuffer);
      radMemoryFreeAligned(myAllocator, testBuffer);
      testBuffer = NULL;
      BufferSize += 1024 * 1024 * 5; //incriment bytes to allocate per iteration

   }
/*
   //LARGE ALLOCATION
  rReleaseString( "\nWe will now attempt to allocate 1GB of memory.\n\n" );
   BufferSize = 1024 * 1024 * 1024;
      testBuffer = ( int * )radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, BufferSize );
      if(testBuffer == NULL){
         rReleasePrintf("%d Allocation of testBuffer FAILED (%dK) \n",i, BufferSize/1024);
      }
      else{
         rReleasePrintf("%d Allocation of testBuffer Succeeded (%dK) \n",i, BufferSize/1024);
      }
         radMemoryFree(RADMEMORY_ALLOC_DEFAULT, testBuffer);
         testBuffer = NULL;*/

   rReleaseString( "**radMemoryAlloc test complete\n" );
//////////////////////////////////////////////////////
   rReleaseString( "\n**IRadMemoryPool test\n" );
 
   
IRadMemoryPool* pool = NULL;
int *elementPointer[20] = {NULL}; //20 pointers for elements  
int numElements = 15;
unsigned int eleSize;
unsigned int numFree;
unsigned int numAlloc;

   radMemoryCreatePool( &pool, sizeof(int), 15 , 0, false, None, RADMEMORY_ALLOC_DEFAULT );
   pool->AddRef();   
   pool->GetStatus(&eleSize, &numFree, &numAlloc);
   if(pool == NULL){
      rReleaseString("pool allocation FAILED\n" );
   }else{
      rReleasePrintf("A pool has been created with; size of element: %d - number Of Elements: %d - Allocated: %d\n\n",eleSize, numFree, numAlloc);

   
//allocate elements
      for(i = 0; i < 20; i++){
         elementPointer[i] = (int *)pool->GetMemory();
         //elementPointer[0][0] = 10;
         //rReleasePrintf("Created element contains %d \n", elementPointer[0][0]);
         
         //check status for update
         if(elementPointer[i] == NULL){
            rReleasePrintf( "%d Creation of pool element FAILED\n", i );
            pool->GetStatus(&eleSize, &numFree, &numAlloc);
            rReleasePrintf("Pool now has; size of element: %d - number Of Elements: %d - Allocated: %d\n",eleSize, numFree, numAlloc);
         }else{
            rReleasePrintf( "%d Created pool element\n", i );
            pool->GetStatus(&eleSize, &numFree, &numAlloc);
            rReleasePrintf("Pool now has; size of element: %d - number Of Elements: %d - Allocated: %d\n",eleSize, numFree, numAlloc);
         }
      }
      rReleaseString( "\n\n\n" );

//release Elements
      for(i = 0; i < 15; i++){      //*******ASSERTS IF YOU ATTEMPT TO FREE ALL 20
         rReleasePrintf( "%d Attempting to release element \n", i );
         pool->FreeMemory(elementPointer[i]);
         pool->GetStatus(&eleSize, &numFree, &numAlloc);
         rReleasePrintf("Pool now has; size of element: %d - number Of Elements: %d - Allocated: %d\n",eleSize, numFree, numAlloc);
      }
   }
   pool->Release();

     rReleaseString( "**IRadMemoryPool test complete\n\n" );
/////////////////////////////////////////////////////////////////////
   rReleaseString( "**IRadMemoryHeap test\n" );


   IRadMemoryHeap* heap = NULL;
   void * elementPointer2[20] = {NULL};   //20 (1K objects)
   int objSize = 1024;
   int heapSize = 1024*15;      //15K    
   i=0;
   unsigned int totalFree, largestBlock, numObjects, highMark;

   //create heap
   heap = radMemoryCreateDougLeaHeap(heapSize,RADMEMORY_ALLOC_DEFAULT);

   if(heap == NULL){
         rReleaseString( "heap allocation FAILED\n" );
   }else{

      heap->AddRef();
      heap->GetStatus(&totalFree, &largestBlock, &numObjects, &highMark);
      rReleasePrintf("Heap Created; Free space: %d - Largest Block: %d - Num Objects: %d - High water mark: %d\n",totalFree, largestBlock, numObjects, highMark);

//Fill Heap  (attempting to allocate and de-allocate too many objects
      for( i = 0; i < 20; i++){
         elementPointer2[i] = heap->GetMemory(objSize);
         if(elementPointer2[i] == NULL){
            rReleasePrintf( "%d Creation of 1K element FAILED\n", i );
            heap->GetStatus(&totalFree, &largestBlock, &numObjects, &highMark);
            rReleasePrintf("Heap now is; Free space: %d - Largest Block: %d - Num Objects: %d - High water mark: %d\n",totalFree, largestBlock, numObjects, highMark);

         }else{
            rReleasePrintf( "%d Created 1K element from heap\n", i );
            heap->GetStatus(&totalFree, &largestBlock, &numObjects, &highMark);
            rReleasePrintf("Heap now is; Free space: %d - Largest Block: %d - Num Objects: %d - High water mark: %d\n",totalFree, largestBlock, numObjects, highMark);
         }
      }
      rReleaseString( "\n\n\n" );

//Release Heap Objects   
      for( i = 0; i < 20; i++){

         heap->FreeMemory(elementPointer2[i]);
         heap->GetStatus(&totalFree, &largestBlock, &numObjects, &highMark);
         rReleaseString( "Attempted release of 1K element from heap\n" );
         rReleasePrintf("Heap now is; Free space: %d - Largest Block: %d - Num Objects: %d - High water mark: %d\n",totalFree, largestBlock, numObjects, highMark);
         elementPointer2[i]= NULL;
      }

      heap->Release();
   }
   
     rReleaseString( "**IRadMemoryHeap test complete\n\n" );

////////////////////////////////////////**

     rReleaseString( "**radMemoryExternalHeap test\n" );


     IRadMemoryHeap* eHeap = NULL;
     void * externalPointer = NULL;
     int allocSize = 1024 * 15;
     externalPointer = radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, allocSize );
     radMemoryExternalHeapCreate(&eHeap, externalPointer, allocSize, radMemorySpace_Local, 50, 50, RADMEMORY_ALLOC_DEFAULT);

////
   if(eHeap == NULL){
         rReleaseString( "External heap allocation FAILED\n" );
   }else{

      eHeap->AddRef();
      eHeap->GetStatus(&totalFree, &largestBlock, &numObjects, &highMark);
      rReleasePrintf("Heap Created; Free space: %d - Largest Block: %d - Num Objects: %d - High water mark: %d\n",totalFree, largestBlock, numObjects, highMark);

//Fill Heap  (attempting to allocate and de-allocate too many objects
      for( i = 0; i < 20; i++){
         elementPointer2[i] = eHeap->GetMemory(objSize);
         if(elementPointer2[i] == NULL){
            rReleasePrintf( "%d Creation of 1K element FAILED\n", i );
            eHeap->GetStatus(&totalFree, &largestBlock, &numObjects, &highMark);
            rReleasePrintf("eHeap now is; Free space: %d - Largest Block: %d - Num Objects: %d - High water mark: %d\n",totalFree, largestBlock, numObjects, highMark);

         }else{
            rReleasePrintf( "%d Created 1K element from eHeap\n", i );
            eHeap->GetStatus(&totalFree, &largestBlock, &numObjects, &highMark);
            rReleasePrintf("Heap now is; Free space: %d - Largest Block: %d - Num Objects: %d - High water mark: %d\n",totalFree, largestBlock, numObjects, highMark);
         }
      }
//Release Heap Objects   
         rReleaseString( "\n\n\n" );

      for( i = 0; i < 14; i++){  //***freeing more than 14 CRASHES the program (unhandled exception)

         eHeap->FreeMemory(elementPointer2[i]);
         eHeap->GetStatus(&totalFree, &largestBlock, &numObjects, &highMark);
         rReleaseString( "Attempted release of 1K element from eHeap\n" );
         rReleasePrintf("eHeap now is; Free space: %d - Largest Block: %d - Num Objects: %d - High water mark: %d\n",totalFree, largestBlock, numObjects, highMark);
     //    elementPointer2[i]= NULL;
      }
      eHeap->Release();      
      radMemoryFree(externalPointer);

   }
   
     rReleaseString( "**radMemoryExternalHeap test complete\n" );


////////////////////////////////////////////
//TEST COMPLETE
////////////////////////////////////////////
     //NOW STARTING FUN WITH STRINGS

/*     IRadString *myString;

     radStringCreate(&myString,RADMEMORY_ALLOC_DEFAULT);

     myString->Append("");
     rReleasePrintf(" %s \n", myString);

     myString->Release();*/


    //
    // Shutdown (this is a local function)
    //
    ::radShutdown( "Tests Complete" );

#if !defined( RAD_GAMECUBE ) && !defined( RAD_XBOX )
    return( 0 );
#endif
}

