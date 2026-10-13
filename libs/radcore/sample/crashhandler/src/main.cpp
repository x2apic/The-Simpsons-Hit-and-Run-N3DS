//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        Main.cpp
//
// Subsystem:   Radical Crash Handler - Sample 
//
// Description: This file contains code to illustrate the use of the 
//              radCrashHandler
//
// Revisions:   V1.00   Apr 4, 2002        RWS
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include <stdlib.h>
#include <raddebug.hpp>
#include <raddebugconsole.hpp>
#include <raddebugcommunication.hpp>
#include <radcontroller.hpp>
#include <radplatform.hpp>
#include <radtime.hpp>
#include <radtextdisplay.hpp>
#include <radfile.hpp>
#include <radthread.hpp>
#include <radcrashhandler.hpp>
#include <radstacktrace.hpp>

#ifdef RAD_GAMECUBE
void * ::operator new( size_t size )
{
    return ::radMemoryAlloc( RADMEMORY_ALLOC_DEFAULT, size );
}

void ::operator delete( void * pMem )
{
    ::radMemoryFree( RADMEMORY_ALLOC_DEFAULT, pMem );
}
#endif


//============================================================================
// struct ExceptionGenerator
//============================================================================

struct ExceptionGenerator
{
    // This function will eventually Dereference a null pointer

    void DereferenceNullPointer( unsigned int depth )    
    {
        if( depth > 1 )
        {
            depth--;

            ExceptionGenerator * pExceptionGenerator = new ExceptionGenerator( );
            pExceptionGenerator->DereferenceNullPointer( depth );
        }
        else
        {
            uintptr_t result[ 5 ];
            unsigned int numResults = 5;
            ::radStackTraceGet( result, numResults );


            depth = 0;

		    rReleasePrintf( "*** About To Dereference a null pointer\n" );            

            ExceptionGenerator * pExceptionGenerator = NULL;
            ExceptionGenerator otherExceptionGenerator;
            otherExceptionGenerator = * pExceptionGenerator;
        }
    }

    void TLBMissWrite( void )
    {
        rReleasePrintf( "*** TLB Miss Write...\n" );

        * ( unsigned int * )( NULL ) = 4;
    }

    void DivideByZero( void )
    {
        rReleasePrintf( "*** About to divide by zero...\n" );
            
        unsigned int non_zero = 100;
        unsigned int zero = 0;
        unsigned int divide_by_zero = non_zero / zero;
    }

    void ReadIllegalAddress( void )
    {
        rReleasePrintf( "*** About to read illegal address\n" );

        unsigned int exception_will_happen_here = * ( unsigned int * ) 0x80000000;
    }

    void WriteIllegalAddress( void )
    {
        rReleasePrintf( "*** About to write illegal address\n" );

        * ( unsigned int * ) 0x80000000 = 4;
    }   

    void DataBusReadError( void )
    {
        unsigned number = * ( unsigned int * ) 0xFFFFFFFF;
    }

    static unsigned int s_FunctionCallCount;
};

// Initialize the static member

unsigned int ExceptionGenerator::s_FunctionCallCount = 0;

static unsigned int robert[ 5 ];

//============================================================================
// Main
//============================================================================

#ifdef RAD_XBOX
int _cdecl main (void)
#endif

#if defined RAD_PS2 || defined RAD_GAMECUBE
int main( void )
#endif

{
    //
    // FTech Initialization
    //
    
    ::radMemoryInitialize( );
    ::radTimeInitialize( );
    ::radPlatformInitialize
    (
        #ifdef RAD_PS2
		    "IRX\\", IOPMediaHost
        #endif
    );
    ::radThreadInitialize( );
    ::radFileInitialize( );
    ::radControllerInitialize( );

    //
    // To install the crash handler, call radCrashHandlerInitialize( )
    //

    ::radCrashHandlerInitialize( );


    for( unsigned int i = 0; i < 100000; i++ )
    {
        radFileService( );
    }

    //
    // Now when an exception occurs, the crash handler will save a report
    // on the memorycard
    //

    ExceptionGenerator * pExceptionGenerator = new ExceptionGenerator( );

    //
    // Take your pick of exceptions (not all generate exceptions on all platforms)
    // You can change this variable with a debugger.
    //

    unsigned int excpt = 5;
    
    switch( excpt )
    {
        case 0:
        {
            pExceptionGenerator->DereferenceNullPointer( 5 );
            break;
        }
        case 1:
        {
            pExceptionGenerator->DivideByZero( );   
            break;
        }
        case 2:
        {
            pExceptionGenerator->TLBMissWrite( );
            break;
        }
        case 3:
        {
            pExceptionGenerator->ReadIllegalAddress( );
            break;
        }
        case 4:
        {
            pExceptionGenerator->WriteIllegalAddress( );
            break;
        }
        case 5:
        {
            pExceptionGenerator->DataBusReadError( );
            break;
        }
        default:
        {
            break;
        }
    }
    rReleasePrintf( "This is after the the exception\n" );

    //
    // Terminate the crash handler at the end of your program 
    //

    ::radCrashHandlerTerminate( );

    //
    // Terminate FTech
    //
    
    ::radControllerTerminate( );
    ::radFileTerminate( );
    ::radThreadTerminate( );
    ::radPlatformTerminate( );
    ::radTimeTerminate( );
    ::radMemoryTerminate( );

    //
    // In a debug build we can dump all objects that may have been left stranded. Lets
    // do it to make sure everything cleaned up.
    //
    #ifdef RAD_DEBUG
    radObject::DumpObjects( );    
    #endif

    return 0;
}



