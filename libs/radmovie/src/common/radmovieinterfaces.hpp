//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radmovieinterfaces.hpp
//
// Subsystem:	Foundation Technologies - Movie Player
//
// Description:	This file contains the declaration for the Foundation Tech 
//              Movie Player
//
// Date:    	May 16, 2002
//
//=============================================================================

#ifndef RADMOVIEINTERFACES_HPP
#define RADMOVIEINTERFACES_HPP

//=============================================================================
// Include Files
//=============================================================================

#ifdef RAD_GAMECUBE

#include <hvqm4dec.h>

#endif // RAD_GAMECUBE

#include <radobject.hpp>
#include <radfile.hpp>

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

struct IRadMovieVideoDecoder;
struct radMovieVideoFrame;

//=============================================================================
// Global Functions
//=============================================================================

// Platform dependent initialization and termination

void radMovieVideoDecoderInitialize( radMemoryAllocator allocator );
void radMovieVideoDecoderTerminate( void );

// Creation factory for the platform dependent video decoder

IRadMovieVideoDecoder * radMovieVideoDecoderCreate( radMemoryAllocator allocator );

#ifdef RAD_GAMECUBE 

radMovieVideoFrame * radMovieVideoFrameCreate( radMemoryAllocator allocator, const HVQM4SeqObj * pHVQM4SeqObj );

#endif // RAD_GAMECUBE

//=============================================================================
// Interface IRadMovieVideoDecoder
//=============================================================================

//
// The IRadMovieVideoDecoder handles the decoding of video,
// which is a platform dependent operation
//

struct IRadMovieVideoDecoder : IRefCount
{
    enum State { NoData, DataPending, Decoding, Ready, Error };
    
    //
    // Initialize( ) will allocate the buffers required for
    // decoding and storing the video data.  The size of the output 
    // video frame is based on its max height and width in pixels.
    // The size of the code data Buffer is specified in bytes.
    //

    virtual void Initialize( 
        unsigned int maxWidthPixels, 
        unsigned int maxHeightPixels, 
        unsigned int codedBufferBytes ) = 0;

    virtual void SetSource( IRadFile * pIRadFile, unsigned int widthPixels, unsigned int heightPixels, unsigned int startPosition, unsigned int frameRate ) = 0;
    virtual void Reset( void ) = 0;

    virtual void * GetLockedVideoFrame( unsigned int * pPresentationTime ) = 0;
    virtual void UnlockVideoFrame( void ) = 0;

    virtual State GetState( void ) = 0;
    virtual float GetFrameRate( void ) = 0;
    virtual unsigned int GetCurrentFrameNumber( void ) = 0;
    virtual void Service( void ) = 0;
};

//=============================================================================
// Interface IRadMovieVideoDecoder
//=============================================================================

struct radMovieVideoFrame : radObject
{
    IMPLEMENT_BASEOBJECT( "radMovieVideoFrame" )

    #ifdef RAD_GAMECUBE

    inline radMovieVideoFrame( const HVQM4SeqObj * pHVQM4SeqObj );
    inline virtual ~radMovieVideoFrame( void );

    void * m_pFrameBuffer;
    unsigned int m_SizeInBytes;

    #endif // RAD_GAMECUBE
};

//=============================================================================
// Gamecube implementations
//=============================================================================

#ifdef RAD_GAMECUBE

//=============================================================================
// radMovieVideoFrame::radMovieVideoFrame
//=============================================================================

radMovieVideoFrame::radMovieVideoFrame( const HVQM4SeqObj * pHVQM4SeqObj )
    :
    m_pFrameBuffer( NULL )
{
    rAssert( pHVQM4SeqObj != NULL );

    // Allocate enough memory for a decompressed video
    // frame using a magical formula

    m_pFrameBuffer = ::radMemoryAlloc( GetThisAllocator( ), 
        ::radMemoryRoundUp( ( pHVQM4SeqObj->frame_width * pHVQM4SeqObj->frame_height * 
            ( pHVQM4SeqObj->h_samp * pHVQM4SeqObj->v_samp + 2 ) ) / ( pHVQM4SeqObj->h_samp * pHVQM4SeqObj->v_samp), 32 ) );
}

//=============================================================================
// radMovieVideoFrame::~radMovieVideoFrame
//=============================================================================

radMovieVideoFrame::~radMovieVideoFrame( void )
{
    ::radMemoryFree( m_pFrameBuffer );
    m_pFrameBuffer = NULL;
}

#endif // RAD_GAMECUBE

#endif // RADMOVIEINTERFACES_HPP