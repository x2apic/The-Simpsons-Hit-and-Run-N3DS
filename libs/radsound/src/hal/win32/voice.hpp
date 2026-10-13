//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#ifndef VOICE_HPP
#define VOICE_HPP

//============================================================================
// Interface: Include Files
//============================================================================

#include <radlinkedclass.hpp>
#include <radsound_hal.hpp>
#include <radsoundobject.hpp>
#include "buffer.hpp"
#include "positionalgroup.hpp"

//============================================================================
// Interface: IRadSoundVoiceWin
//============================================================================

class radSoundHalVoiceWin
    :
    public IRadSoundHalVoice,
	public radSoundHalPositionalEntity,
	public radLinkedClass< radSoundHalVoiceWin >,
	public radSoundObject
{

	public:

		virtual void SetPriority( unsigned int priority );
		virtual unsigned int GetPriority( void );

		virtual void Play( void );
		virtual void Stop( void );
		virtual bool IsPlaying( void );
		virtual unsigned int GetPlaybackPositionInSamples( void );
		virtual void SetPlaybackPositionInSamples( unsigned int positionInSamples );
		         
		virtual void SetBuffer( IRadSoundHalBuffer * pIRadSoundHalBuffer );
		virtual IRadSoundHalBuffer * GetBuffer( void );

		virtual void  SetMuted( bool );
		virtual bool  GetMuted( void );
		virtual void  SetVolume( float );
		virtual float GetVolume( void );
		virtual void  SetTrim( float );
		virtual float GetTrim( void ); 
		virtual void  SetPitch( float );
		virtual float GetPitch( void );
		virtual void  SetPan( float );
		virtual float GetPan( void );

		virtual void SetAuxMode( unsigned int aux, radSoundAuxMode  mode );
		virtual radSoundAuxMode GetAuxMode( unsigned int aux );
		virtual void SetAuxGain( unsigned int aux, float gain );
		virtual float GetAuxGain( unsigned int aux );

		virtual void SetPositionalGroup( IRadSoundHalPositionalGroup * pIRadSoundHalPositionalGroup );
		virtual IRadSoundHalPositionalGroup * GetPositionalGroup( void );

		virtual void OnApplyPositionalInfo( float listenerRolloffFactor );
	
		radSoundHalVoiceWin( void );

	protected:

		virtual ~radSoundHalVoiceWin( void );

	private:

		IMPLEMENT_REFCOUNTED( "radSoundHalVoiceWin" )

		bool IsHardwarePlaying( void );

		void SetVolumeInternal( void );
		void SetPitchInternal( void );

		unsigned int m_Priority;

		float m_Pitch;
		float m_Volume;
		float m_MuteFactor;
		float m_Trim;

		ALuint m_Source;

		ref< radSoundHalBufferWin >		m_xRadSoundHalBufferWin;
		ref< IRadSoundHalAudioFormat >	m_xIRadSoundHalAudioFormat;
		ref< radSoundHalPositionalGroup > m_xRadSoundHalPositionalGroup;
};

#endif // VOICE_HPP