//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:	Foundation Technologies - Thread Sample
//
// Description:	This file contains the sample program to illustrate the use
//              of the Foundation Technologies Threading services
//
// Date:    	V1.00	January 22, 2002		Creation
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#ifdef RAD_WIN32	
#include <windows.h>
#endif 

#ifdef RAD_XBOX
#include <xtl.h>
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <radobject.hpp>
#include <radtime.hpp>
#include <radmemory.hpp>
#include <radthread.hpp>

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
// Local Defintions
//=============================================================================

static void FiberEntryPoint( void* userData );
static unsigned int ThreadEntryPoint( void* userData );

//=============================================================================
// Global Data Declarations
//=============================================================================

//
// These are used for our threading test.
//
static IRadThreadMutex*     s_pMutex;
static IRadThreadSemaphore* s_pSemaphore;

static unsigned int s_MainThreadCount = 0;
static unsigned int s_ChildThread1Count = 0;
static unsigned int s_ChildThread2Count = 0;
static unsigned int s_ChildThreadSharedCount = 0;

//
// This is used for the thread local storage testing.
//
static IRadThreadLocalStorage* s_ThreadLocalStorage;

//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    main
//=============================================================================
// Description: main entry point
//
// Parameters:  argc		number of string parameters on command line.
//				argv		pointer to array of strings from command line.
//
// Returns:     0 means no errors
//				!0 means errors
//
// Notes:
//------------------------------------------------------------------------------

#ifdef RAD_WIN32	
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#endif

#if defined( RAD_PS2) || defined( RAD_XBOX )
int main( int argc, char* argv[ ] )
#endif

#ifdef RAD_GAMECUBE
void main( void )
#endif
{
    char Outbuf[ 200 ];

    //
    // Display banner message.
    //
    rReleaseString( "Threading Sample Program V1.00\n" );

    //
    // Initialize threading system prior to memory system as memory system may
    // use exclusion objects
    //
    radThreadInitialize( );
    radMemoryInitialize( );
    radTimeInitialize( );

	//
	// Initialte Platform stuff.
	//
    #ifdef RAD_WIN32
        //
        // Windows requires the game provide the main window handle and the module
        // instance.
        //
        radPlatformInitialize( NULL, hInstance );
    #endif

    #ifdef RAD_XBOX
        //
        // XBOX initialization requires no parameters
        //
        radPlatformInitialize(  );
    #endif

    #ifdef RAD_PS2
        //
        // On the PS2, provide the root path for IRXs. 
        // 
        radPlatformInitialize( "\\usr\\local\\sce\\iop\\modules\\",IOPMediaHost );
    #endif 

    #ifdef RAD_GAMECUBE
        radPlatformInitialize( );
    #endif

    //
    // First lets try sleeping with a few values
    //
    rReleaseString( "Testing sleep. Should wake in 2 seconds ...\n");
    unsigned int startTime = radTimeGetMilliseconds( );
    radThreadSleep( 2000 );
    sprintf( Outbuf, "Sleep time was actually %d ms\n", radTimeGetMilliseconds( ) - startTime );    
    rReleaseString( Outbuf );

    rReleaseString( "Testing sleep. Should wake in 5.5 seconds ...\n");
    startTime = radTimeGetMilliseconds( );
    radThreadSleep( 5500 );
    sprintf( Outbuf, "Sleep time was actually %d ms\n", radTimeGetMilliseconds( ) - startTime );    
    rReleaseString( Outbuf );

    rReleaseString( "Testing sleep. Should wake in 0.52 seconds ...\n");
    startTime = radTimeGetMilliseconds( );
    radThreadSleep( 52 );
    sprintf( Outbuf, "Sleep time was actually %d ms\n", radTimeGetMilliseconds( ) - startTime );    
    rReleaseString( Outbuf );

    rReleaseString( "Testing yeild. Should wake in 0.0 seconds ...\n");
    startTime = radTimeGetMilliseconds( );
    radThreadSleep( 0 );
    sprintf( Outbuf, "Yeild time was actually %d ms\n", radTimeGetMilliseconds( ) - startTime );    
    rReleaseString( Outbuf );

    //
    // Lets do some testing of fibers. Create a fiber and switch between them
    // until we reach a fixed count. We will use the fibers user data to pass
    // the fiber they should switch to.
    //
    IRadThreadFiber* pFiber;

    radThreadCreateFiber( &pFiber, FiberEntryPoint, NULL );
    
    rAssert( pFiber != NULL );
    
    //
    // Lets pass the actively running fiber as the user data. We don't worry
    // about reference counting it. The other fiber will use this data to switch
    // back to us.
    //    
    pFiber->SetValue( radThreadGetActiveFiber( ) );

    //
    // Lets loop five times, switching between the two fibers.
    //
    for( unsigned int i = 0 ; i < 5 ; i++ )
    {
        rReleaseString( "Main fiber switching to child fiber\n");

        pFiber->SwitchTo( );

        rReleaseString( "Main fiber is running\n");
    }
    
    //
    // Lets kill the fiber,
    //
    pFiber->Release( );

    //
    // Now lets try some of the threading services. We will create an exclusion
    // object and us this object to serialize access to a counter.
    // The main thread will just run waiting for the
    // two threads to terminate. The first thread will suspend itself on a semaphore
    // until the second thread runs.
    //
    radThreadCreateMutex( &s_pMutex );
    radThreadCreateSemaphore( &s_pSemaphore );

    //
    // Lets also create a thread local storage object. Each thread will set its own
    // value including the main. Before terminating, they will each make sure there
    // value is the same.
    //
    radThreadCreateLocalStorage( &s_ThreadLocalStorage );

    //
    // Main thread sets is value to 0x1234

    s_ThreadLocalStorage->SetValue( (void*) 0x1234 );

    //
    // Pass 0 as the user data to the first thread to indicate it is the one that 
    // suspends on the mutex waiting for the second thread to signal it.
    //
    IRadThread* pThread1;
    IRadThread* pThread2;

    radThreadCreateThread( &pThread1, ThreadEntryPoint, (void*) 0 );
    radThreadCreateThread( &pThread2, ThreadEntryPoint, (void*) 1 );

    //
    // Now lets wait for the threads to terminate. We do not need to yield as the system
    // is pre-emptive. However by doing so, we act as a good citizen.
    //
    unsigned int returnCode1;
    unsigned int returnCode2;

    while( pThread1->IsRunning( &returnCode1 ) || pThread2->IsRunning( &returnCode2 ) )
    {
        //
        //  Update a count of our loop interactions and allow others to play
        //
        radThreadSleep( 0 );

        s_MainThreadCount++;

        //
        // This code is done just to make sure the suspend and resume code works.
        //
        if( s_MainThreadCount == 500 )
        {
            //
            // Here we suspend one thread form another,
            //
            pThread1->Suspend( );
        }

        if( s_MainThreadCount == 1000 )
        {
            pThread1->Resume( ); 
            pThread2->Resume( ); 
        }
    }
    //
    // Verify retunr code mechansim worked.
    //
    rAssert( returnCode1 == 0 );
    rAssert( returnCode2 == 1 );
    
    //
    // Lets make sure the main threads local storage is still in tack.
    //
    rAssert( s_ThreadLocalStorage->GetValue( ) == (void*) 0x1234 );

    s_ThreadLocalStorage->Release( );

    //
    // Threads have terminated themselves. This is the correct behavior. Free our objects
    // used it the test.
    //
    pThread1->Release( );
    pThread2->Release( );
    
    s_pMutex->Release( );
    s_pSemaphore->Release( );

    //
    // Display message showing number to times each thread executed its loop.
    //
    sprintf( Outbuf, "Threads Terminated. Main count = %d, Thread1 count = %d, Thread2 count = %d\n",
                      s_MainThreadCount, s_ChildThread1Count, s_ChildThread2Count );  
    rReleaseString( Outbuf );
   
    //
    // Clean up.     
    //
	radPlatformTerminate( );
    radTimeTerminate( );
    radMemoryTerminate( );
    radThreadTerminate( );

    #ifdef RAD_DEBUG
        //
        // Verify all objects have been deleted.
        //
        radObject::DumpObjects( );    
    #endif

    rReleaseString( "Sample terminated correctly\n");

    #ifndef RAD_GAMECUBE
	return( 0 );
    #endif
}

//=============================================================================
// Function:    FiberEntryPoint
//=============================================================================
// Description: This is the entry for our fiber. We just print a message and
//              then switch back to the fiber passed as our thread user data.
//
// Parameters:  userData - not used.
//
// Returns:     n/a
//
// Notes:       Fibers should never return. 
//------------------------------------------------------------------------------

void FiberEntryPoint( void* userData )
{
    (void ) userData;

    //
    // Enter an inifinite loop. We print a message and switch back to the 
    // fiber that was passed as our fiber data.
    //
    rReleaseString( "Child fiber is running\n");

    while( true )
    {
        rReleaseString( "Child fiber switching to main fiber\n");
        
        IRadThreadFiber* pFiber = (IRadThreadFiber*) radThreadGetActiveFiber( )->GetValue( );
 
        pFiber->SwitchTo( );

        rReleaseString( "Child fiber is running\n");
    }
}

//=============================================================================
// Function:    ThreadEntryPoint
//=============================================================================
// Description: This is the entry for threads. Based on the user data, one
//              thread suspends on the semphore while the other signals it.
//              They increment a shared counter (under
//              protection of mutex ). Finally they return the user data as
//              the return value just to make sure return code mechanism works.
//
// Parameters:  userData - 0 - wait on semaphore
//                         1 - signal semaphore
//
// Returns:     userData
//
// Notes:       
//------------------------------------------------------------------------------

unsigned int ThreadEntryPoint( void* userData )
{
    //
    // Set our local storgare to user data.
    //
    s_ThreadLocalStorage->SetValue( userData );

    //
    // Check which thread is started.
    //
    if( (unsigned int) userData == 0 )
    {

        //
        // We are the first thread. We wait for the other thread to start us.
        //
        s_pSemaphore->Wait( );
    }
    else
    {
        //
        // We are the second thread. Start the other thread by signalling it.
        //
        s_pSemaphore->Signal( );
    }

    //
    // Now both threads increment a counter. When the total is reached we 
    // terminate. Both threads increment their own counters so we can determine
    // the nature of the pre-emption.
    //
    s_pMutex->Lock( );

    while( s_ChildThreadSharedCount < 5000 )
    {
        s_ChildThreadSharedCount++;

        //
        // Each thread updates is count
        //
        if( (unsigned int) userData == 0 )
        {
            s_ChildThread1Count++;
        }
        else
        {
            s_ChildThread2Count++;
        }

        //
        // Release mutex to allow other threads to run.
        //
        s_pMutex->Unlock( );

        //
        // Thread 2 suspends itself expliticitly after a count of 250 is arrived at.
        //
        if( (s_ChildThread2Count == 250 ) && (userData == (void*) 1) )
        {
            radThreadGetActiveThread( )->Suspend( );            
        }

        //
        // Allow others to play
        //
        radThreadSleep( 0 );

        s_pMutex->Lock( );
    }
          
    s_pMutex->Unlock( );

    //
    // Lets verify that our value was perserved.
    //
    rAssert( s_ThreadLocalStorage->GetValue( ) == userData );

 
    return( (unsigned int) userData );
}