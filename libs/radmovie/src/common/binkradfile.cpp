//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

//=============================================================================
//
// File:        
//
// Subsystem:	Foundation Technologies - Movie Player
//
// Description:	
//
// Date:    	
//
//=============================================================================

//=============================================================================
// Includes
//=============================================================================

#include "pch.hpp"
#include "binkradfile.hpp"
#include <radfile.hpp>
#include <string.h>

//=============================================================================
// Forward Declarations
//=============================================================================
static void ReadKickoff( BINKIO* bio );
class BinkCallback;
struct radBinkFile;

//=============================================================================
// Constants and Statics
//=============================================================================

extern "C" {
    extern U32 RADLINK RADTimerRead( void );
}

#define BASEREADSIZE   ( 16 * 1024 )

static BinkCallback* s_BinkCallback = NULL;

//=============================================================================
// struct radBinkFile
//=============================================================================
// All of the info we need to make the file system functions work.
//------------------------------------------------------------------------------
struct radBinkFile
{
    IRadFile*    filePtr;           // our file!
    unsigned int fileSize;          // the file size
    unsigned int filePosition;      // the current file position
    unsigned int fileBufPos;        // current file position for start of buffer
    unsigned int readEndPosition;   // the file pointer after a read
    unsigned int readSize;          // the size that the read was meant to be 

    char*        buffer;            // the start of our buffer (total memory)
    char*        bufferEnd;         // the end of our buffer (total memory)
    char*        bufferFront;       // start of the used buffer
    char*        bufferBack;        // end of the used buffer
    unsigned int bufferEmpty;       // how much empty space is there in our buffer

    S32          binkWorking;       // Bink timer stuff ... I just copied it
    U32          binkTimer;         // Bink timer

    char         stopReads;         // flag to indicate not to issue more reads

    unsigned int alignPos;
};
typedef radBinkFile* radBinkFilePtr;

//=============================================================================
// BinkCallback
//=============================================================================
// Class used to catch file system callbacks. Intances are created with 
// initialize/destroy, so it's not actually refcounted. Therefore addref/release
// just keep a count/
//-----------------------------------------------------------------------------

class BinkCallback : public radObject, public IRadFileCompletionCallback
{
public:
    BinkCallback( void ) : refCount( 0 ) { }
    void OnFileOperationsComplete( void* pUserData );
    void AddRef( ) { refCount++; };
    void Release( ) { refCount--; };

    unsigned int refCount;
};

//=============================================================================
// Function: BinkCallback::OnFileOperationsComplete
//=============================================================================
// Description: Set variables to indicate that the read finished, and start a
//              new read.
//-----------------------------------------------------------------------------
void BinkCallback::OnFileOperationsComplete( void* pUserData )
{
    BINKIO* bio = (BINKIO*) pUserData;
    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;

    //
    // check that the read succeeded
    //
    unsigned int bytesRead = binkFile->readEndPosition - binkFile->filePosition;

    if ( 
        bytesRead != BASEREADSIZE && 
        bytesRead != ::radMemoryRoundUp( binkFile->readSize, binkFile->filePtr->GetOptimalSize( ) )
       )
    {
        bio->DoingARead = 0;
        bio->ReadError = 1;
        return;
    }

    bio->BytesRead += bytesRead;
    binkFile->filePosition = binkFile->readEndPosition;
    binkFile->bufferBack += bytesRead;

    //
    // wrap around the circular buffer.
    //
    if ( binkFile->bufferBack >= binkFile->bufferEnd )
    {
        binkFile->bufferBack = binkFile->buffer;
    }

    //
    // update free space and high water mark
    //
    binkFile->bufferEmpty -= bytesRead;
    bio->CurBufUsed += bytesRead;

    if ( bio->CurBufUsed > bio->BufHighUsed )
    {
        bio->BufHighUsed = bio->CurBufUsed;
    }

    //
    // update timings
    //
    binkFile->binkTimer = RADTimerRead( ) - binkFile->binkTimer;
    bio->TotalTime += binkFile->binkTimer;

    if ( ( binkFile->binkWorking ) || ( bio->Working ) )
    {
        bio->ThreadTime += binkFile->binkTimer;
    }
    else
    {
        bio->IdleTime += binkFile->binkTimer;
    }

    bio->DoingARead = 0;

    //
    // Kickoff another read.
    //
    ReadKickoff( bio );
}

//=============================================================================
// Global Functions
//=============================================================================

//=============================================================================
// Function: radBinkFileInitialize
//=============================================================================
// Description: Create the binkcallback instance
//-----------------------------------------------------------------------------

void radBinkFileInitialize( radMemoryAllocator alloc )
{
    rAssert( s_BinkCallback == NULL );
    s_BinkCallback = new( alloc ) BinkCallback;
    rAssert( s_BinkCallback != NULL );
}

//=============================================================================
// Function: radBinkFileTerminate
//=============================================================================
// Description: Destroy the binkcallback instance
//-----------------------------------------------------------------------------

void radBinkFileTerminate( void )
{
    rAssert( s_BinkCallback != NULL );
    rAssertMsg( s_BinkCallback->refCount == 0, "radMovie2: trying to terminate with outstanding requests." );
    delete s_BinkCallback;
    s_BinkCallback = NULL;
}

//=============================================================================
// Function: intelendian
//=============================================================================
// Description: Convert a buffer to intel endian.
//------------------------------------------------------------------------------

#ifdef RAD_GAMECUBE
static void intelendian( void* dest, U32 size )
{
    U32  s = ( size + 3 ) / 4;
    U32* d = (U32*) dest;
    while ( s-- )
    {
        U32 v = __lwbrx( d, 0 );
        *d++ = v;
    }
}

#else
#define intelendian( dest, size )
#endif

//=============================================================================
// Function: BinkFileReadHeader
//=============================================================================
// Description: Synchronously (!) read bytes from the file.
//-----------------------------------------------------------------------------

static U64 RADLINK BinkFileReadHeader( BINKIO* bio, S64 offset, void* dest, U64 size )
{
    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;

    if ( offset != -1 && binkFile->filePosition != offset )
    {
        binkFile->filePosition = (unsigned int)offset;
    }

    //
    // Make an aligned buffer.
    //
    unsigned int alignment = binkFile->filePtr->GetOptimalSize( );
    if ( alignment > 1024 ) alignment = 1024;

    char buffer[ 1024 + radFileOptimalMemoryAlignment ];
    char* pBuffer = (char*) ::radMemoryRoundUp( (uintptr_t) buffer, radFileOptimalMemoryAlignment );
    unsigned int bufferSize = buffer + sizeof( buffer ) - pBuffer;
    bufferSize = ::radMemoryRoundDown( bufferSize, alignment );

    char* ptr = (char*) dest;
    unsigned int bytesRead = 0;
    while( size )
    {
        unsigned int readPosition = ::radMemoryRoundDown( binkFile->filePosition, alignment );
        unsigned int readOffset = binkFile->filePosition - readPosition;

        unsigned int copySize = bufferSize - readOffset;
        unsigned int readSize = bufferSize;
        if ( size < bufferSize )
        {
            copySize = (unsigned int)size;
            readSize = ::radMemoryRoundUp((unsigned int)size + readOffset, alignment );
        }

        //
        // Do a synchronous read for the header.
        //
        unsigned int newPosition;
        binkFile->filePtr->SetPositionSync( readPosition );
        binkFile->filePtr->ReadSync( pBuffer, readSize );
        binkFile->filePtr->GetPositionSync( &newPosition );

        //
        // Error!
        //
        if ( readSize != ( newPosition - readPosition ) )
        {
            return 0;
        }

        //
        // Copy over the info.
        //
        memcpy( ptr, &pBuffer[ readOffset ], copySize );
        ptr += copySize;
        size -= copySize;

        //
        // Set the new file position.
        //
        binkFile->filePosition += copySize;
        binkFile->fileBufPos = binkFile->filePosition;
        bytesRead += copySize;
    }

    unsigned int temp = ( binkFile->fileSize - binkFile->fileBufPos );
    bio->CurBufSize = ( temp < bio->BufSize ) ? temp : bio->BufSize;

    intelendian( dest, bytesRead );

    return( bytesRead );
}

//=============================================================================
// Function: ReadKickoff
//=============================================================================
// Description: Start a new read if possible.
//-----------------------------------------------------------------------------

static void ReadKickoff( BINKIO* bio )
{
    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;

    if ( binkFile->stopReads == 1 ) return;
    if ( bio->DoingARead ) return;

    unsigned int amountLeft = 
        binkFile->filePosition >= binkFile->fileSize ? 0 : binkFile->fileSize - binkFile->filePosition;

    if ( ( binkFile->bufferEmpty >= BASEREADSIZE ) && amountLeft > 0 )
    {
        // issue a new read.
        binkFile->binkWorking = bio->Working;
        binkFile->binkTimer = RADTimerRead( );

        bio->DoingARead = 1;

        unsigned int readSize;
        if ( amountLeft > BASEREADSIZE )
        {
            binkFile->readSize = BASEREADSIZE;
            readSize = BASEREADSIZE;
        }
        else
        {
            binkFile->readSize = amountLeft;
            readSize = ::radMemoryRoundUp( amountLeft, binkFile->filePtr->GetOptimalSize( ) );
        }

        //
        // Check our alignment.
        //
        unsigned int alignSize = binkFile->filePosition % binkFile->filePtr->GetOptimalSize( );
        if ( alignSize != 0 )
        {
            rAssert( binkFile->alignPos == 0 );
            rAssert( binkFile->bufferBack == binkFile->buffer );
            rAssert( binkFile->filePosition == binkFile->fileBufPos );

            binkFile->alignPos = alignSize;
            binkFile->filePosition -= alignSize;
            binkFile->fileBufPos -= alignSize;
        }

        //
        // issue a read request.
        //
        binkFile->filePtr->SetPositionAsync( binkFile->filePosition );
        binkFile->filePtr->ReadAsync( binkFile->bufferBack, readSize );
        binkFile->filePtr->GetPositionAsync( &binkFile->readEndPosition );
        binkFile->filePtr->AddCompletionCallback( s_BinkCallback, bio );
    }
    else
    {
        // if we can't fill anymore, then set the max size to the current size
        bio->CurBufSize = bio->CurBufUsed;
    }
}

//=============================================================================
// Function: BinkFileIdle
//=============================================================================
// Description: Start a new read if possible and service.
//-----------------------------------------------------------------------------

static U64 RADLINK BinkFileIdle( BINKIO* bio )
{
    if ( bio->ReadError )
    {
        return( 0 );
    }

    if ( bio->DoingARead )
    {
        ::radFileService( );
    }

    if ( !bio->DoingARead )
    {
        ReadKickoff( bio );
    }

    return( bio->DoingARead );
}

//=============================================================================
// Function: CancelReadRequests
//=============================================================================
// Description: Synchronous tear-down. TODO: remove this.
//-----------------------------------------------------------------------------

static void CancelReadRequests( BINKIO* bio )
{
    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;
    binkFile->stopReads = 1;
    while( bio->DoingARead )
    {
        ::radFileService( );
    }
    binkFile->stopReads = 0;
}

//=============================================================================
// Function: BinkFileReadFrame
//=============================================================================
// Description: Read a frame. Reads from our buffer if possible, otherwise
//              the frame needs to be skipped.
//-----------------------------------------------------------------------------

static U64  RADLINK BinkFileReadFrame( BINKIO* bio, U32 framenum, S64 offset, void * dest, U64 size )
{
    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;

    if ( bio->ReadError ) return 0;

    U32 timer = RADTimerRead( );
    void* odest = dest;
    unsigned int transferAmount = 0;

    if ( ( offset != -1 ) && ( binkFile->fileBufPos != offset ) )
    {
        rReleaseAssert( false );
        if ( ( binkFile->fileBufPos < (unsigned int) offset ) && ( binkFile->filePosition >= (unsigned int) offset ) )
        {
            unsigned int amount = offset - binkFile->fileBufPos;
            binkFile->fileBufPos = offset;
            binkFile->bufferEmpty += amount;
            bio->CurBufUsed -= amount;
            if ( binkFile->bufferFront > binkFile->bufferEnd )
            {
                binkFile->bufferFront -= bio->BufSize;
            }
        }
        else
        {
            // We've got nothing in our buffer!
            CancelReadRequests( bio );
            
            // check this set of the file position
            rReleaseAssert( false );

            binkFile->filePosition = offset;
            binkFile->fileBufPos = offset;
            binkFile->bufferEmpty = bio->BufSize;
            bio->CurBufUsed = 0;
            binkFile->bufferFront = binkFile->buffer;
            binkFile->bufferBack = binkFile->buffer;
        }
    }

    if ( !binkFile->buffer )
    {
        //
        // No read buffer, so read synchronously.
        //
        U32 timer2 = RADTimerRead( );

        unsigned int newPosition;
        binkFile->filePtr->SetPositionSync( binkFile->filePosition );
        binkFile->filePtr->ReadSync( dest, size );
        binkFile->filePtr->GetPositionSync( &newPosition );

        //
        // Set the new file position.
        //
        unsigned int bytesRead = newPosition - binkFile->filePosition;
        if ( bytesRead < size )
        {
            bio->ReadError = 1;
        }
        binkFile->filePosition = newPosition;
        binkFile->fileBufPos = newPosition;
        bio->BytesRead += bytesRead;
        transferAmount += bytesRead;

        //
        // Update the timers.
        //
        U32 endTime = RADTimerRead( );
        bio->TotalTime += ( endTime - timer2 );
        bio->ForegroundTime += ( endTime - timer );
    }
    else
    {
        while( ( size > 0 ) && ( bio->ReadError == 0 ) )
        {
            //
            // Make sure we're we've got reads going on.
            //
            if ( bio->DoingARead )
            {
                ::radFileService( );
            }
            ReadKickoff( bio );

            U32 copySize = bio->CurBufUsed;

            if ( copySize )
            {
                copySize = ( copySize > size ) ? size : copySize;
                copySize -= binkFile->alignPos;

                size -= copySize;
                transferAmount += copySize;
                binkFile->fileBufPos += copySize + binkFile->alignPos;

                U32 front = binkFile->bufferEnd - binkFile->bufferFront - binkFile->alignPos;
                if ( front <= copySize )
                {
                    memcpy( dest, binkFile->bufferFront + binkFile->alignPos, front );
                    dest = ( (U8*) dest ) + front;
                    binkFile->bufferFront = binkFile->buffer;
                    copySize -= front;

                    bio->CurBufUsed -= ( (S32) front + binkFile->alignPos );
                    binkFile->bufferEmpty += front + binkFile->alignPos;
                    binkFile->alignPos = 0;
                }

                if ( copySize > 0 )
                {
                    memcpy( dest, binkFile->bufferFront + binkFile->alignPos, copySize );
                    dest = ( (U8*) dest ) + copySize;
                    binkFile->bufferFront += copySize + binkFile->alignPos;
                    bio->CurBufUsed -= ( (S32) copySize + binkFile->alignPos );
                    binkFile->bufferEmpty += copySize + binkFile->alignPos;
                    binkFile->alignPos = 0;
                }
            }
        }
        bio->ForegroundTime += RADTimerRead() - timer;
    }

    unsigned int tmp = binkFile->fileSize - binkFile->fileBufPos;
    bio->CurBufSize = ( tmp < bio->BufSize ) ? tmp : bio->BufSize;
    if ( ( bio->CurBufUsed + BASEREADSIZE ) > bio->CurBufSize )
    {
        bio->CurBufSize = bio->CurBufUsed;
    }

    intelendian( odest, transferAmount );

    return( transferAmount );
}

//=============================================================================
// Function: BinkFileGetBufferSize
//=============================================================================
// Description: Return the recommended buffer size.
//-----------------------------------------------------------------------------

static U64 RADLINK BinkFileGetBufferSize( BINKIO* bio, U64 size )
{
    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;

    size = ::radMemoryRoundUp( size, BASEREADSIZE );
    return( size );
}

//=============================================================================
// Function: BinkFileSetInfo
//=============================================================================
// Description: Initialize our internal variables.
//-----------------------------------------------------------------------------

static void RADLINK BinkFileSetInfo( BINKIO* bio, void* buf, U64 size, U64 filesize, U32 simulate )
{
    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;

    rAssert( (uintptr_t) buf % radFileOptimalMemoryAlignment == 0 );

    unsigned int realSize = ::radMemoryRoundDown( size, BASEREADSIZE );

    binkFile->alignPos = 0;
    binkFile->buffer = (char*) buf;
    binkFile->bufferFront = binkFile->buffer;
    binkFile->bufferBack = binkFile->buffer;
    binkFile->bufferEnd = binkFile->buffer + realSize;
    binkFile->bufferEmpty = realSize;
    binkFile->fileSize = filesize;
    binkFile->stopReads = 0;

    bio->BufSize = realSize;
    bio->CurBufUsed = 0;
}

//=============================================================================
// Function: BinkFileClose
//=============================================================================
// Description: Close the file.
//-----------------------------------------------------------------------------

static void RADLINK BinkFileClose( BINKIO* bio )
{
    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;

    CancelReadRequests( bio );

    binkFile->filePtr->Release( );
    binkFile->filePtr = NULL;
}

//=============================================================================
// Function: radBinkFileOpen
//=============================================================================
// Description: Open the file.
//-----------------------------------------------------------------------------

S32 RADLINK radBinkFileOpen( BINKIO* bio, const char* name, U32 flags )
{
    rAssert( sizeof( radBinkFile ) <= sizeof( bio->iodata ) );
//    rAssert( flags & BINKFILEHANDLE );

    memset( bio, 0, sizeof( BINKIO ) );

    radBinkFilePtr binkFile = (radBinkFilePtr) bio->iodata;

    ::radFileOpenSync( &binkFile->filePtr, name );
    rAssert( binkFile->filePtr != NULL );
    rAssert( binkFile->filePtr->IsOpen( ) );
    binkFile->filePtr->SetBufferedRead( IRadFile::BufferedReadOff );

    bio->ReadHeader = BinkFileReadHeader;
    bio->ReadFrame = BinkFileReadFrame;
    bio->GetBufferSize = BinkFileGetBufferSize;
    bio->SetInfo = BinkFileSetInfo;
    bio->Idle = BinkFileIdle;
    bio->Close = BinkFileClose;

    return( 1 );
}
