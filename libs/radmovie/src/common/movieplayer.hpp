//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        movieplayer.hpp
//
// Subsystem:	Foundation Technologies - Movie Player
//
// Description:	This file contains the declaration for the Foundation Tech 
//              Movie Player
//
// Date:    	May 16, 2002
//
//=============================================================================

#ifndef MOVIEPLAYER_HPP
#define MOVIEPLAYER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radoptions.hpp>

#ifndef RAD_MOVIEPLAYER_USE_BINK

#include <radtime.hpp>
#include <radmovie2.hpp>
#include <radlinkedclass.hpp>
#include <radsound.hpp>
#include <radstring.hpp>
#include "radmovieinterfaces.hpp"

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

class radMoviePlayer;

//=============================================================================
// Type Definitions
//=============================================================================

//=============================================================================
// Class radMoviePlayer
//=============================================================================

class radMoviePlayer 
    : 
    public IRadMoviePlayer2,
    public IRadFileCompletionCallback,
    public radLinkedClass< radMoviePlayer >,
    public radRefCount
{
    public:

        IMPLEMENT_REFCOUNTED( "radMoviePlayer" )
    
        //
        // Constructor / Destructor
        //

        radMoviePlayer( void );
        virtual ~radMoviePlayer( void );

        //
        // IRadMoviePlayer
        //

        virtual void Initialize( 
            IRadMovieRenderLoop * pIRadMovieRenderLoop, 
            IRadMovieRenderStrategy * pIRadMovieRenderStrategy,
            unsigned int maxWidthPixels, 
            unsigned int maxHeightPixels, unsigned int codedVideoBufferBytes, 
            unsigned int audioPrimaryBufferSize, unsigned int audioSecondaryBufferSize,
            IRadSoundHalAudioFormat::SizeType audioSizeType );

        virtual bool Render( void );

        virtual void Load( const char * pVideoFileName, unsigned int audioTrackIndex );
        virtual void Unload( void );
        virtual void Play( void );
        virtual void Pause( void );

        virtual void  SetPan( float pan ); // -1.0 to 1.0 ( left to right )
        virtual float GetPan( void );
        virtual void  SetVolume( float volume ); // 0.0 to 1.0 ( min to max )
        virtual float GetVolume( void );
        virtual State GetState( void );
        virtual bool GetVideoFrameInfo(VideoFrameInfo *frameInfo);
        virtual float GetFrameRate( void );
        virtual unsigned int GetCurrentFrameNumber( void );

        //
        // IRadFileCompletionCallback
        //
        
        virtual void OnFileOperationsComplete( void* pUserData );

        //
        // Internal public functions
        //

        void Service( void );

    private:

        unsigned int GetCurrentTime( void );
        void SetState( IRadMoviePlayer2::State state );
        void InternalPlay( void );

        ref< IRadSoundBufferedDataSource > m_refIRadSoundBufferedDataSource;
        ref< IRadMovieRenderStrategy > m_refIRadMovieRenderStrategy;
        ref< IRadMovieVideoDecoder > m_refIRadMovieVideoDecoder;
        ref< IRadSoundStreamPlayer > m_refIRadSoundStreamPlayer;
        ref< IRadMovieRenderLoop > m_refIRadMovieRenderLoop;
        ref< IRadStopwatch > m_refIRadStopwatch;
        ref< IRadFile > m_refIRadFile_Current;
        ref< IRadFile > m_refIRadFile_Stale;
        ref< IRadString > m_refIRadString_PendingFileName;
        
        IRadMoviePlayer2::State m_State;

        enum VideoFrameState { VideoFrame_Unlocked, VideoFrame_Locked };
        VideoFrameState m_VideoFrameState;

        unsigned int m_AudioTrackIndex;
        unsigned int m_PresentationTime;
        bool m_SkippingFrames;

        void * m_pVideoFrame;
        void * m_pTempHeaderBuffer;

        float m_pSoundClockFixupTaps[ 8 ];

        VideoFrameInfo m_VideoFrameInfo;
};

#endif // ! RAD_MOVIEPLAYER_USE_BINK
#endif // MOVIEPLAYER_HPP
