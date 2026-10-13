//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        binkmovieplayerxbox.hpp
//
// Subsystem:	Foundation Technologies - Movie Player
//
// Description:	This is the Bink movie player.  It's good for pc, xbox, & gcn
//
// Date:    	October, 2002 RWS
//
//=============================================================================

#ifndef BINKMOVIEPLAYER_HPP
#define BINKMOVIEPLAYER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radoptions.hpp>
#ifdef RAD_MOVIEPLAYER_USE_BINK

#include <bink.h>
#include <radmovie2.hpp>
#include <radlinkedclass.hpp>

//=============================================================================
// Definitions
//=============================================================================

//=============================================================================
// Forward Class/Struct Declarations
//=============================================================================

class radMoviePlayerBink;

//=============================================================================
// Class radMoviePlayerBink
//=============================================================================

class radMoviePlayerBink
    : 
    public IRadMoviePlayer2,
    public radLinkedClass< radMoviePlayerBink >,
    public radRefCount
{
    public:

        IMPLEMENT_REFCOUNTED( "radMoviePlayerBink" )
    
        //
        // Constructor / Destructor
        //

        radMoviePlayerBink( void );
        virtual ~radMoviePlayerBink( void );

        //
        // IRadMoviePlayer
        //

        virtual void Initialize( 
            IRadMovieRenderLoop * pIRadMovieRenderLoop, 
            IRadMovieRenderStrategy * pIRadMovieRenderStrategy );

        virtual bool Render( void );

        virtual void Load( const char * pVideoFileName, unsigned int audioTrackIndex );
        virtual void Unload( void );
        virtual void Play( void );
        virtual void Pause( void );
        virtual void  SetPan( float pan ); 
        virtual float GetPan( void );
        virtual void  SetVolume( float volume );
        virtual float GetVolume( void );
        virtual State GetState( void );
        virtual bool GetVideoFrameInfo(VideoFrameInfo * pFrameInfo);
        virtual float GetFrameRate( void );
        virtual unsigned int GetCurrentFrameNumber( void );

        void Service( void );

    private:

        void SetState( IRadMoviePlayer2::State state );
        void SetBinkVolume( void );
		void SetBinkPan( void );
        
        ref< IRadMovieRenderLoop > m_refIRadMovieRenderLoop;
        ref< IRadMovieRenderStrategy > m_refIRadMovieRenderStrategy;
        IRadMoviePlayer2::State m_State;
        HBINK m_BinkHandle;
        float m_Volume;
        float m_Pan;
        U32 m_AudioTrackIndex;
        bool m_IsPresentationOutstanding;
        bool m_CheckAudio;
};

#endif // RAD_MOVIEPLAYER_USE_BINK
#endif // BINKMOVIEPLAYERXBOX_HPP
