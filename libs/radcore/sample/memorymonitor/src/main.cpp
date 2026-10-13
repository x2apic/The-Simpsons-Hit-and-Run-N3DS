//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:   Radical Foundation Tech Memory Monitor Client Sample Program
//
// Description:	
//
// Revisions:	Nov 30, 2001
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <raddebugcommunication.hpp>
#include <radmemorymonitor.hpp>
#include <radmemory.hpp>
#include <radtime.hpp>
#include <radthread.hpp>
#include <radstacktrace.hpp>

#if defined RAD_WIN32
#include "window.hpp"
#endif

bool g_Done = true;

//=============================================================================
// Local Defintions
//=============================================================================

//=============================================================================
// Forward References
//=============================================================================


//=============================================================================
// Static Data Defintions
//=============================================================================

//=============================================================================
// Class Declarations
//=============================================================================

//=============================================================================
// Public Functions
//=============================================================================

//=============================================================================
// Public Member Functions
//=============================================================================

void RunBasicSampleCode( void );

void RunAdvancedSampleCode( void );

void RunCoverageTestCode( void );

//=============================================================================
// Function:    main
//=============================================================================
// Description: Main entry point.
//
// Parameters:  on PS2, pass IP address if you want to use TCPIP via USB.
//
// Returns:     0
//
// Notes:
//------------------------------------------------------------------------------
#ifdef RAD_WIN32
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#endif

#ifdef RAD_XBOX
int _cdecl main (void)
#endif

#if defined RAD_PS2 || defined RAD_GAMECUBE
int main( int argc, char* argv[ ] )
#endif // RAD_PS2 RAD_GAMECUBE
{
#if defined RAD_WIN32
    SetPriorityClass( GetCurrentProcess( ), IDLE_PRIORITY_CLASS );
#endif

    //
    // Display a banner message.
    //
    rDebugString( "Foundation Tech - Memory Monitor Sample V2.00\n" );

    ::radThreadInitialize( );
    ::radMemoryInitialize( );
    ::radTimeInitialize( );

    radMemoryMonitorInitialize( 50 * 1024 );

    //
    // First initialized the platform. Each platform has specific initialization,
    // PS2 requires path to where IRXs reside and where to load them form. We
    // also intialize the debug communication system. This is required if the
    // Radical File Server will be used.
    //
    #ifdef RAD_PS2
    ::radPlatformInitialize( "usr\\local\\sce\\iop\\modules\\", IOPMediaHost );
    
    //
    // For the PS2, we accept a command line arguement. If one is present, we set
    // up debug communicaiton system for USBTcpip/
    //
    if( argc == 2 )
    {
        //
        // When using TCPIP via USB, must provide address info.
        //
    	radDbgComUsbTcpIpInitInfo info;

    	strcpy( info.m_IPAddress, argv[ 1 ] );
	    strcpy( info.m_SubMask, "255.255.0.0" );
	    strcpy( info.m_Gateway, "10.1.1.14" );
    
        ::radDbgComTargetInitialize( UsbTcpIp, radDbgComDefaultPort, &info );
    }
    else
    {
        //
        // Initialize using DECI as the communication avenue.
        //
        ::radDbgComTargetInitialize( Deci );
    }
    #endif

    #ifdef RAD_WIN32
    //
    // Windows initialization requires instance and main window. This program does
    // not use a main window, so just set it to null,
    //
	::radPlatformInitialize(
		::CreateMainWindow( hInstance, SW_SHOWNORMAL ),
			hInstance, RADMEMORY_ALLOC_DEFAULT );
    ::radDbgComTargetInitialize( WinSocket );
    #endif

    #ifdef RAD_XBOX
    //
    // XBOX does not need any info.
    //
    ::radPlatformInitialize( );
    ::radDbgComTargetInitialize( WinSocket );
    #endif

    //
    // Game Cube
    //
    #ifdef RAD_GAMECUBE
    ::radPlatformInitialize( );
    ::radDbgComTargetInitialize( HostIO );
    #endif

    RunBasicSampleCode( );

    RunAdvancedSampleCode( );

    RunCoverageTestCode( );

    while( true )
    {
        radDbgComService( );
        radMemoryMonitorService( );
    }
    
    //
    // Finally clean up.
    //
    ::radPlatformTerminate( );
    ::radMemoryMonitorTerminate( );
    ::radDbgComTargetTerminate( );
    ::radTimeTerminate( );
    ::radMemoryTerminate( );
    ::radThreadTerminate( );

    //
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
    #ifdef RAD_DEBUG
    radObject::DumpObjects( );    
    #endif

    rDebugString("Memory Monitor Sample - Terminates\n");
    return( 0 );
}

void RunBasicSampleCode( void )
{
    rDebugString("Basic Sample Code Start\n");

    radMemoryMonitorIssueFlag( "Basic [Start]" );

    //
    // create few heaps and allocate memory in it.
    // Since FTech's memory manager has MemoryMonitor intergrated in it,
    // memory allocation will show up on host PC automatically.
    //

    radMemoryMonitorIssueFlag( "Basic [Create Heap/Pool]" );

    //
    // create a memory pool
    //
    ref< IRadMemoryPool > pSampleMemoryPool = NULL;
    const unsigned int MEMORYPOOL_ELEMENT_SIZE = 45;
    const unsigned int MEMORYPOOL_ELEMENT_COUNT = 200;

    radMemoryCreatePool( & pSampleMemoryPool, MEMORYPOOL_ELEMENT_SIZE, MEMORYPOOL_ELEMENT_COUNT, 0, false, Moderate, RADMEMORY_ALLOC_DEFAULT, "Test Pool" );
    
    //
    // create a heap
    //
    ref< IRadMemoryHeap > pSampleMemoryHeap = NULL;
    const unsigned int MEMORYHEAP_SIZE = 1024 * 1024;
    pSampleMemoryHeap = radMemoryCreateDougLeaHeap( MEMORYHEAP_SIZE, RADMEMORY_ALLOC_DEFAULT, "Test DL Heap" );

    radMemoryMonitorSuspend( );

    radMemoryMonitorIssueFlag( "Basic [Allocation]" );

    //
    // now do some simple allocations
    //
    
    //
    // get some memory allocations from pool
    //
    const unsigned int MEMORYPOOL_ALLOCATION_COUNT = 100;
    void * ppMemoryPoolElements[ MEMORYPOOL_ALLOCATION_COUNT ];
    memset( ppMemoryPoolElements, 0, sizeof( ppMemoryPoolElements ) );

    unsigned int i, j;

    for ( i = 0; i < MEMORYPOOL_ALLOCATION_COUNT; i ++ )
    {
        ppMemoryPoolElements[ i ] = pSampleMemoryPool->GetMemory( );
    }

    //
    // get some memory allocations from heap
    //
    const unsigned int MEMORYHEAP_ALLOCATION_COUNT = 1000;
    void * ppMemoryHeapElements[ MEMORYHEAP_ALLOCATION_COUNT ];
    memset( ppMemoryHeapElements, 0, sizeof( ppMemoryHeapElements ) );

    for ( j = 0; j < MEMORYHEAP_ALLOCATION_COUNT / 2; j ++ )
    {
        ppMemoryHeapElements[ j * 2 + 0 ] = pSampleMemoryHeap->GetMemory( rand() % 1024 );
        ppMemoryHeapElements[ j * 2 + 1 ] = pSampleMemoryHeap->GetMemoryAligned( rand() % 1024, 32 );
    }
    memset( ppMemoryHeapElements, 0, sizeof( ppMemoryHeapElements ) );

    radMemoryMonitorSuspend( );

    radMemoryMonitorIssueFlag( "Basic [Deallocation]" );

    //
    // now free all memory pool elements
    //
    for ( i = 0; i < MEMORYPOOL_ALLOCATION_COUNT; i ++ )
    {
        pSampleMemoryPool->FreeMemory( ppMemoryPoolElements[ i ] );
    }

    for ( j = 0; j < MEMORYHEAP_ALLOCATION_COUNT / 2; j ++ )
    {
        pSampleMemoryHeap->FreeMemory( ppMemoryHeapElements[ j * 2 + 0 ] );
        pSampleMemoryHeap->FreeMemoryAligned( ppMemoryHeapElements[ j * 2 + 1 ] );
    }
    memset( ppMemoryPoolElements, 0, sizeof( ppMemoryPoolElements ) );

    radMemoryMonitorSuspend( );

    radMemoryMonitorIssueFlag( "Basic [Destory Heap/Pool]" );

    pSampleMemoryPool = NULL;

    pSampleMemoryHeap = NULL;

    rDebugString("Basic Sample Code End\n");
}

void* operator new ( size_t size )
#ifdef PS2EE
throw ( std::bad_alloc )
#endif
{
    void* memory;

    memory = malloc (size);

    if ( memory != NULL )
    {
        radMemoryMonitorDeclareAllocation( memory, size );
    }

    return memory;
}

void operator delete ( void * p )
#ifdef PS2EE
throw ( )
#endif
{
    if ( p )
    {
        free( p );
        radMemoryMonitorRescindAllocation( p );
    }
}

void* operator new[ ] ( size_t size )
#ifdef PS2EE
throw ( std::bad_alloc )
#endif
{
    void* memory;
    void* callerAddr = NULL;

    memory = malloc (size);

    if ( memory )
    {
        radMemoryMonitorDeclareAllocation( memory, size );
    }

    return( memory );
}

void operator delete[ ] (void* p)
#ifdef PS2EE
throw ()
#endif
{
    operator delete ( p );
}

void RunAdvancedSampleCode( void )
{
    //
    // Demanstrates how to overload new/delete operator to track memory allocation/deallocation
    // overloading global new/delete is not the best solution. See class radObject in radcore
    // library for the best solution.
    //
    rDebugString("Advanced Sample Code Start\n");

    radMemoryMonitorIssueFlag( "Advanced [Start]" );


	//
	// allocating two chunck of memory 1000 byte and 2000 byte
	//
	char * pMem1 = new char[ 1000 ];
	radMemoryMonitorIdentifyAllocation( pMem1, "Sample", "pMem1" );

	char * pMem2 = new char[ 2000 ];
	radMemoryMonitorIdentifyAllocation( pMem2, "Sample", "pMem2" );

    radMemoryMonitorSuspend( );

	delete[ ] pMem1;

	delete[ ] pMem2;

    radMemoryMonitorIssueFlag( "Advanced [End]" );

    rDebugString("Advanced Sample Code End\n");
}

void RunCoverageTestCode( void )
{
	//
	// coverage test is not implemented at this time.
	//
}
