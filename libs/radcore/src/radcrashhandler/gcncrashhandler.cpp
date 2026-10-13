//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        gcncrashhandler.cpp
//
// Subsystem:   FTech Crash Handler (Gamecube platform)
//
// Description:	This file contains the implementation of the gamecube crashhandler
//
// Creation:	March 18, 2002 RWS
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <dolphin/os.h>
#include <dolphin/card.h>
#include <radcrashhandler.hpp>
#include <radobject.hpp>
#include <radthread.hpp>
#include <radtextdisplay.hpp>
#include <radfile.hpp>
#include "crashrecords.hpp"

//=============================================================================
// Definitions
//=============================================================================

#define CARD_SLOT 0  // slot A
#define SP 1
#define RECORD_FILE_NAME "CrashRecordGcn.crh"

//=============================================================================
// Forward Declarations
//=============================================================================

static void radExceptionHandlerGcn( OSError error, OSContext* context );
static void radCrashCardCallback( long channel, long result );
static unsigned int radCrashHandlerGcnThreadEntry( void* userData );
void radCrashHandlerInitialize( void );
void radCrashHandlerTerminate( void );

//=============================================================================
// Global Variables
//=============================================================================

ref< IRadThread > g_pIRadThread = NULL;
ref< IRadThreadSemaphore > g_pIRadThreadSemaphore = NULL;
radCrashRecordGcn * g_pRadCrashRecordGcn = NULL;
unsigned int radExceptionCountGcn = 0;
radMemoryAllocator g_RadCrashHandlerGcnAllocator = RADMEMORY_ALLOC_DEFAULT;

//=============================================================================
// ::radExceptionHandlerGcn
//=============================================================================

//
// The operating system will invoke radExceptionHandlerGcn when an exception
// occurs.  This function gets installed as the exception handler for a
// number of possible exceptions during radCrashHandlerInitialize( )
//

static void radExceptionHandlerGcn( OSError error, OSContext* context )
{
    // We don't listen to nested exceptions

	if( radExceptionCountGcn < 1 )
	{
		radExceptionCountGcn++;	

        //
        // Store the details about the exception
        //

        if( g_pRadCrashRecordGcn != NULL )
        {
            g_pRadCrashRecordGcn->m_OSError = ( radCrashRecordGcn::radCrashOSErrorGcn ) error;
            g_pRadCrashRecordGcn->m_OSContext = * ( radCrashRecordGcn::OSContext * )( context );
        }

        //
        // Slip the high priority g_pIRadThread back into the scheduler.
        // When radExceptionHandlerGcn exists, the scheduler will give
        // control of the cpu to g_pIRadThread, not the exception generating
        // thread.  If all goes well, g_pIRadThread should wake up and record
        // a report to the memory card without a problem.
        //
       
        if( g_pIRadThread != NULL )
        {
            g_pIRadThread->Resume( );
        }
        else
        {
            rReleasePrintf( "ERROR: radExceptionHandlerGcn ----- g_pIRadThread == NULL -----\n" );
        }
    }
}

//=============================================================================
// ::radCrashCardCallback
//=============================================================================

// Having this callback seems to be a requirement of the memory card

static void radCrashCardCallback( long channel, long result )
{
    rReleasePrintf( "RAD CRASH CARD CALLBACK: channel [%d] result [%d]\n", channel, result );
}

//=============================================================================
// ::radCrashHandlerGcnThreadEntry
//=============================================================================

//
// radCrashHandlerGcnThreadEntry defines the entry point for a high priority
// thread that will be resumed in the event of an exception.  If possible this
// thread will record exception details to the memory card in slot 'A' ( '0' )
//

static unsigned int radCrashHandlerGcnThreadEntry( void* userData )
{
    //
    // The thread will take control when it is created.  This gives 
    // some time to prepare for a crash
    //

    // error testing for CARD functions

    unsigned int ret = 0;
    char buf[ 256 ];
    ref< IRadTextDisplay > display;
    ::radTextDisplayGet( & display );


    // The memory card requires a work area

    void * pCardWorkArea = ::radMemoryAllocAligned( 
        g_RadCrashHandlerGcnAllocator, CARD_WORKAREA_SIZE, 64 );  // only requires 32 bytes alignment
    
    // For creating and writing to files

    CARDFileInfo cardFileInfo;

    //
    // Prepare the memory card
    //

    CARDInit( );
    
    ret = CARDMount( CARD_SLOT, pCardWorkArea, NULL );

    if( ret == CARD_RESULT_BROKEN )
    {
        // The card isn't formatted
        ret = CARDFormat( CARD_SLOT );
    }

    if( ret != CARD_RESULT_READY )
    {
        rReleasePrintf( "     * CRASH HANDLER THREAD: FAILED to mount card [%d]\n", ret );
        return 0;
    }

ret = CARDFormat( CARD_SLOT );

    // All card access must occur in multiples of the card's sector size...

    unsigned long sectorSize = 0;
    ret = CARDGetSectorSize( CARD_SLOT, & sectorSize );
    if( ret != CARD_RESULT_READY )
    {
        rReleasePrintf( "     * CRASH HANDLER THREAD: FAILED to obtain sector size of card [%d]\n", ret );
        return 0;
    }

    // ... The filesize must be multiple of the sector size

    unsigned long fileSize = sizeof( radCrashRecordGcn );
    fileSize = fileSize - ( fileSize % sectorSize ) + sectorSize;

    // Prepare a crash record for the big event

    g_pRadCrashRecordGcn = static_cast< radCrashRecordGcn * >( ::radMemoryAllocAligned( g_RadCrashHandlerGcnAllocator, fileSize, 1024 ) );
    for( unsigned int i = 0; i < sizeof( radCrashRecordGcn ); i++ )
    {
        * ( ( char * ) g_pRadCrashRecordGcn + i ) = 0xAD;
    }

    // Finish whatever can be finished now

    sprintf( g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Title,    RAD_CRASH_RECORD_TITLE );
    sprintf( g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Platform, RAD_CRASH_RECORD_GCN_PLATFORM );
    sprintf( g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Version,  RAD_CRASH_RECORD_GCN_VERSION );

    //
    // Let the main thread know that we're finished initializing
    // and then go to sleep and wait for a crash to occur
    //    

    g_pIRadThreadSemaphore->Signal( );
    ::radThreadGetActiveThread( )->Suspend( );

    //
    // Oh no! If we reach this point, we've experienced a crash.
    //

    //
    // Print out some info before we start doing risky things
    //

    sprintf( buf, " * radCrashHandlerGcn: Exception at [0x%08x]\n", g_pRadCrashRecordGcn->m_OSContext.srr0 );
    rReleasePrintf( buf );
    display->TextOut( buf );

    sprintf( buf, " *                     OSError [%d]\n", g_pRadCrashRecordGcn->m_OSError );
    rReleasePrintf( buf );
    display->TextOut( buf );

    //
    // Record a time stamp for this
    //

    OSCalendarTime calendarTime;
    ::OSTicksToCalendarTime( OSGetTime( ), & calendarTime );

    g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Day = calendarTime.mday;
    g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Month = calendarTime.mon + 1;
    g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Year = calendarTime.year;
    g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Hour = calendarTime.hour;
    g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Minute = calendarTime.min;
    g_pRadCrashRecordGcn->m_RadCrashRecordInfo.m_Second = calendarTime.sec;
    
    //
    // Perform a stack dump
    //

    // A pointer to memory to help our dump of stuff above and below the stack pointer

    unsigned int * pMemStackDump = ( unsigned int * )( g_pRadCrashRecordGcn->m_OSContext.gpr[ SP ] - RAD_CRASH_RECORD_GCN_STACK_DUMP_SP_OFFSET * sizeof( unsigned int ) );

    if( g_pRadCrashRecordGcn->m_OSContext.gpr[ SP ] - RAD_CRASH_RECORD_GCN_STACK_DUMP_SP_OFFSET * sizeof( unsigned int ) < 0 )
    {
        pMemStackDump = ( unsigned int * )( 0x00000000 );  // stay valid
    }

    // Record the contents of the stack

    for( unsigned int dumpIndex = 0; dumpIndex < RAD_CRASH_RECORD_GCN_STACK_DUMP_SIZE; dumpIndex++ )
    {
        g_pRadCrashRecordGcn->m_StackDump[ dumpIndex ] = * pMemStackDump;

        if( ( unsigned int ) pMemStackDump > 0xFFFFFFFC )
        {
            break;   // 
        }

        pMemStackDump++;
    }

    //
    // Create a file on the memory card
    //

    rReleasePrintf( "     * CRASH HANDLER THREAD: Creating file\n" );

    ret = CARDCreate( CARD_SLOT, RECORD_FILE_NAME, fileSize, & cardFileInfo );
    
    if( ret == CARD_RESULT_EXIST )
    {
        ret = CARDDelete( CARD_SLOT, RECORD_FILE_NAME );
        ret = CARDCreate( CARD_SLOT, RECORD_FILE_NAME, fileSize, & cardFileInfo );
    }

    if( ret != CARD_RESULT_READY )
    {
        rReleasePrintf( "     * CRASH HANDLER THREAD: FAILED to create file [%d]\n", ret );
        return 0;
    }

    //
    // Write the crash record out to the memory card file
    //

    rReleasePrintf( "     * CRASH HANDLER THREAD: Writing File\n" );
    ret = CARDWrite( & cardFileInfo, g_pRadCrashRecordGcn, fileSize, 0 );
    if( ret != CARD_RESULT_READY )
    {
        rReleasePrintf( "     * CRASH HANDLER THREAD: FAILED to write to file [%d]\n", ret );
        return 0;
    }

    //
    // Might as well close the file
    //

    rReleasePrintf( "     * CRASH HANDLER THREAD: Closing File\n" );
    ret = CARDClose( & cardFileInfo );
    if( ret != CARD_RESULT_READY )
    {
        rReleasePrintf( "     * CRASH HANDLER THREAD: FAILED to close file [%d]\n", ret );
        return 0;
    }

    return 1;
}

//=============================================================================
// ::radCrashHandlerInitialize
//=============================================================================

void radCrashHandlerInitialize( radMemoryAllocator allocator )
{
    rAssert( g_pIRadThread == NULL );
    g_RadCrashHandlerGcnAllocator = allocator;

    //
    // Use a semaphore to ensure that the crash handler thread initializes itself
    // before a problem can occur
    //

    ::radThreadCreateSemaphore( & g_pIRadThreadSemaphore, 0, allocator );
    
    //
    // Create a high priority thread that can take over when an 
    // exception occurs
    // 

    ::radThreadCreateThread( & g_pIRadThread, radCrashHandlerGcnThreadEntry,
        NULL, IRadThread::PriorityHigh, 4096, allocator );

    //
    // Wait for the thread to signal on the semaphore
    //

    g_pIRadThreadSemaphore->Wait( );

    //
    // Install radExceptionHandlerGcn as the exception handler
    // for every exception that we're allowed to listen to.
    // (Note: See dolphin docs for info about which exceptions
    // have to be ignored.)
    //

    //
    // Note that if the program is being debugged, the exception handler
    // isn't able to do any thing
    //

    OSSetErrorHandler(OS_ERROR_SYSTEM_RESET, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_MACHINE_CHECK, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_DSI, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_ISI, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_ALIGNMENT, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_PROGRAM, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_TRACE, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_PERFORMACE_MONITOR, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_SYSTEM_INTERRUPT, (OSErrorHandler) radExceptionHandlerGcn);
    OSSetErrorHandler(OS_ERROR_PROTECTION, (OSErrorHandler) radExceptionHandlerGcn);
}

//=============================================================================
// ::radCrashHandlerTerminate
//=============================================================================

void radCrashHandlerTerminate( void )
{

}