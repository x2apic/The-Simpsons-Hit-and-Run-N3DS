//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUND_GCN
#define RADSOUND_GCN

#include <radsound_hal.hpp>
#include <radsoundfile.hpp>

struct IRadSoundEffectStdReverbGcn;
struct IRadSoundEffectDelayGcn;
struct IRadSoundHalAudioFormatGcn;

struct IRadSoundEffectStdReverbGcn * radSoundEffectStdReverbGcnCreate( radMemoryAllocator allocator );
struct IRadSoundEffectDelayGcn     * radSoundEffectDelayGcnCreate( radMemoryAllocator allocator );
struct IRadSoundHalAudioFormatGcn  * radSoundHalAudioFormatGcnCreate( radMemoryAllocator allocator );

struct IRadSoundHalAudioFormatGcn : public IRefCount
{
    virtual void Initialize(
        radSoundHalAdpcmInfoGcn * pRadSoundHalAdpcmInfoGcn,
        IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat ) = 0;

    virtual unsigned long LogicalUnitsToSamples( unsigned int logicalUnits ) = 0;
    virtual unsigned long SamplesToLogicalUnits( unsigned int samples ) = 0;

    virtual radSoundHalAdpcmInfoGcn * GetAdpcmInfo( unsigned int channel ) = 0;
};

struct IRadSoundEffectHqReverbGcn
    :
    public IRadSoundHalEffect
{
	virtual void SetEnabled( bool enabled ) = 0;
	virtual bool GetEnabled( void ) = 0;

	virtual void SetGain( float gain ) = 0;
	virtual float GetGain( void ) = 0;

    virtual void SetPreDelay( float milliseconds ) = 0; // 0 to 100
    virtual float GetPreDelay( void ) = 0;
    
    virtual void SetReverbTime( float milliseconds ) = 0; // 100 to 10,000
    virtual float GetReverbTime( void ) = 0;
    
    virtual void SetColoration( float coloration ) = 0; // 0.0f to 1.0f 
    virtual float GetColoration( void ) = 0;
    
    virtual void SetDamping( float damping ) = 0; // 0.0f to 1.0f
    virtual float GetDamping( void ) = 0;
    
    virtual void SetCrossTalk( float crosstalk ) = 0; // 0.0 to 1.0f ( really 0 to 100 )
    virtual float GetCrossTalk( void ) = 0;
};

struct IRadSoundEffectStdReverbGcn
    :
    public IRadSoundHalEffect
{
	virtual void SetEnabled( bool enabled ) = 0;
	virtual bool GetEnabled( void ) = 0;
        
    virtual void SetGain( float gain ) = 0;
	virtual float GetGain( void ) = 0;

    virtual void SetPreDelay( float milliseconds ) = 0; // 0 to 100
    virtual float GetPreDelay( void ) = 0;
    
    virtual void SetReverbTime( float milliseconds ) = 0; // 100 to 10,000
    virtual float GetReverbTime( void ) = 0;
    
    virtual void SetColoration( float coloration ) = 0; // 0.0f to 1.0f 
    virtual float GetColoration( void ) = 0;
    
    virtual void SetDamping( float damping ) = 0; // 0.0f to 1.0f
    virtual float GetDamping( void ) = 0;    
};

struct IRadSoundEffectChorusGcn
    :
    public IRadSoundHalEffect
{
	virtual void SetEnabled( bool enabled ) = 0;
	virtual bool GetEnabled( void ) = 0;

    virtual void SetGain( float gain ) = 0;
	virtual float GetGain( void ) = 0;

    virtual void SetDelay( float ms ) = 0; // 5 to 15
    virtual float GetDelay( void ) = 0;
    
    virtual void SetDepth( float depth ) = 0; // 0 to 5
    virtual float GetDepth( void ) = 0;
    
    virtual void SetPeriod( float period ) = 0; // 500 to 10,000
    virtual float GetPeriod( void ) =0;
};

struct IRadSoundEffectDelayGcn
    :
    public IRadSoundHalEffect
{
	virtual void SetEnabled( bool enabled ) = 0;
	virtual bool GetEnabled( void ) = 0;

    virtual void SetGain( float gain ) = 0;
	virtual float GetGain( void ) = 0;

    virtual void SetDelay ( unsigned int channel, float ms ) = 0; // 10 to 5,000
    virtual float GetDelay( unsigned int channel ) = 0;
    
    virtual void SetFeedback( unsigned int channel, float feedback ) = 0; // 0.0 to 1.0 (really 0 to 100 )
    virtual float GetFeedback( unsigned int channel ) = 0;
};

#endif // RADSOUND_GCN