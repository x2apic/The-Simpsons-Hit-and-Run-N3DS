#include "pch.hpp"

#include <3ds.h>
#include <math.h>
#include <string.h>

#include <radsound_hal.hpp>
#include <radsound_win32.hpp>
#include <radsoundmath.hpp>
#include <radlinkedclass.hpp>
#include <radsoundobject.hpp>
#include <radmemory.hpp>
#include <raddebug.hpp>
#include <radtime.hpp>

#include "../common/memoryregion.hpp"
#include "../common/softwarelistener.hpp"
#include "../common/softwarepositionalgroup.hpp"
#include "../../common/radsoundupdatableobject.hpp"
#include "bufferloader.hpp"

template< class T > static inline void SafeAddRef( T * p )
{
    if( p != NULL ) ::radAddRef( p, NULL );
}

template< class T > static inline void SafeRelease( T * & p )
{
    if( p != NULL ) { ::radRelease( p, NULL ); p = NULL; }
}

static bool g_NdspAvailable = false;

static unsigned char * g_pSoundMemory = NULL;

enum { NDSP_NUM_CHANNELS = 24 };

static bool     s_ChannelUsed[ NDSP_NUM_CHANNELS ];

static int AllocChannel( void )
{
    for( int i = 0; i < NDSP_NUM_CHANNELS; i++ )
    {
        if( !s_ChannelUsed[ i ] )
        {
            s_ChannelUsed[ i ] = true;
            return i;
        }
    }

    return -1;
}

static void FreeChannel( int channel )
{
    if( channel >= 0 && channel < NDSP_NUM_CHANNELS )
    {
        ndspChnWaveBufClear( channel );
        s_ChannelUsed[ channel ] = false;
    }
}

extern "C" int radSoundCtrAllocChannel( void )
{
    return g_NdspAvailable ? AllocChannel( ) : -1;
}

extern "C" void radSoundCtrFreeChannel( int channel )
{
    FreeChannel( channel );
}

static u16 s_AdpcmCoefs[ 16 ] =
{
       0,                0,
    2048,                0,
    4096, (u16) ( -2048 ),
    1536,              512,
    1920,                0,
    3680, (u16) ( -1664 ),
    3136, (u16) ( -1856 ),
    3968, (u16) ( -1984 ),
};

enum { ADPCM_FRAME_SAMPLES = 14, ADPCM_FRAME_BYTES = 8 };

static inline int AdpcmClamp16( int v )
{
    return ( v < -32768 ) ? -32768 : ( ( v > 32767 ) ? 32767 : v );
}

static bool AdpcmEncodeFrame( const s16 * in, int p, int shift, int & h1, int & h2, u8 * out )
{
    const int c1 = (s16) s_AdpcmCoefs[ p * 2 ];
    const int c2 = (s16) s_AdpcmCoefs[ p * 2 + 1 ];
    const int half = ( 1 << shift ) >> 1;
    bool fits = true;
    int y1 = h1, y2 = h2;

    out[ 0 ] = (u8) ( ( p << 4 ) | shift );
    for( int i = 0; i < ADPCM_FRAME_SAMPLES; i++ )
    {
        const int pred = ( c1 * y1 + c2 * y2 + 1024 ) >> 11;
        const int resid = in[ i ] - pred;

        int nib = ( resid >= 0 ) ? ( ( resid + half ) >> shift ) : -( ( -resid + half ) >> shift );
        if( nib > 7 )       { nib = 7;  fits = false; }
        else if( nib < -8 ) { nib = -8; fits = false; }

        const int y = AdpcmClamp16( pred + ( nib << shift ) );
        y2 = y1;
        y1 = y;

        if( i & 1 ) out[ 1 + i / 2 ] |= (u8) ( nib & 0xf );
        else        out[ 1 + i / 2 ]  = (u8) ( ( nib & 0xf ) << 4 );
    }

    h1 = y1;
    h2 = y2;
    return fits;
}

static unsigned int AdpcmEncodeInPlace( void * pData, unsigned int numSamples )
{
    const s16 * pcm = (const s16 *) pData;
    u8 * out = (u8 *) pData;
    const unsigned int frames = ( numSamples + ADPCM_FRAME_SAMPLES - 1 ) / ADPCM_FRAME_SAMPLES;
    int h1 = 0, h2 = 0;

    for( unsigned int f = 0; f < frames; f++ )
    {
        s16 in[ ADPCM_FRAME_SAMPLES ];
        const unsigned int first = f * ADPCM_FRAME_SAMPLES;
        for( unsigned int i = 0; i < ADPCM_FRAME_SAMPLES; i++ )
        {
            in[ i ] = ( first + i < numSamples ) ? pcm[ first + i ] : 0;
        }

        int bestP = 0, bestMax = 0, bestBits = 99;
        unsigned int bestEnergy = 0xffffffffu;
        for( int p = 0; p < 8; p++ )
        {
            const int c1 = (s16) s_AdpcmCoefs[ p * 2 ];
            const int c2 = (s16) s_AdpcmCoefs[ p * 2 + 1 ];
            int x1 = h1, x2 = h2, maxRes = 0;
            unsigned int energy = 0;
            for( int i = 0; i < ADPCM_FRAME_SAMPLES; i++ )
            {
                int r = in[ i ] - ( ( c1 * x1 + c2 * x2 + 1024 ) >> 11 );
                if( r < 0 ) r = -r;
                if( r > maxRes ) maxRes = r;
                energy += (unsigned int) ( r >> 4 ) * (unsigned int) ( r >> 4 );
                x2 = x1;
                x1 = in[ i ];
            }

            int scaleBits = 0;
            for( int m = maxRes; m > 7; m >>= 1 ) scaleBits++;
            if( scaleBits < bestBits || ( scaleBits == bestBits && energy < bestEnergy ) )
            {
                bestP = p;
                bestMax = maxRes;
                bestBits = scaleBits;
                bestEnergy = energy;
            }
        }

        int shift = 0;
        while( shift < 12 && ( ( bestMax + ( ( 1 << shift ) >> 1 ) ) >> shift ) > 7 )
        {
            shift++;
        }

        u8 * frame = out + f * ADPCM_FRAME_BYTES;
        for( ;; )
        {
            int t1 = h1, t2 = h2;
            if( AdpcmEncodeFrame( in, bestP, shift, t1, t2, frame ) || shift >= 12 )
            {
                h1 = t1;
                h2 = t2;
                break;
            }
            shift++;
        }
    }

    return frames * ADPCM_FRAME_BYTES;
}

class radSoundHalBufferCtr
    :
    public IRadSoundHalBuffer,
    public IRadSoundHalBufferLoadCallback,
    public radLinkedClass< radSoundHalBufferCtr >,
    public radSoundObject
{
    public:

    IMPLEMENT_REFCOUNTED( "radSoundHalBufferCtr" )

    radSoundHalBufferCtr( void )
        :
        m_SizeInFrames( 0 ),
        m_Looping( false ),
        m_Streaming( false ),
        m_pLoadTarget( NULL ),
        m_LoadBytes( 0 ),
        m_Adpcm( false ),
        m_AdpcmBytes( 0 ),
        m_refIRadSoundHalAudioFormat( NULL ),
        m_refIRadMemoryObject( NULL ),
        m_refIRadSoundHalBufferLoadCallback( NULL )
    {
    }

    virtual ~radSoundHalBufferCtr( void )
    {
    }

    void Initialize(
        IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat,
        IRadMemoryObject * pIRadMemoryObject,
        unsigned int sizeInFrames,
        bool looping,
        bool streaming )
    {
        rAssert( pIRadSoundHalAudioFormat != NULL );

        m_refIRadSoundHalAudioFormat = pIRadSoundHalAudioFormat;
        m_refIRadMemoryObject        = pIRadMemoryObject;
        m_Looping                    = looping;
        m_Streaming                  = streaming;
        m_SizeInFrames               = sizeInFrames;
    }

    IRadSoundHalAudioFormat * GetFormat( void ) { return m_refIRadSoundHalAudioFormat; }
    IRadMemoryObject * GetMemoryObject( void ) { return m_refIRadMemoryObject; }
    bool IsLooping( void ) { return m_Looping; }
    unsigned int GetSizeInFrames( void ) { return m_SizeInFrames; }

    void LoadAsync(
        IRadSoundHalDataSource * pIRadSoundHalDataSource,
        unsigned int startPositionInFrames,
        unsigned int numberOfFrames,
        IRadSoundHalBufferLoadCallback * pIRadSoundHalBufferLoadCallback )
    {
        rAssert( m_refIRadSoundHalBufferLoadCallback == NULL );

        if( m_Adpcm )
        {
            if( pIRadSoundHalBufferLoadCallback != NULL )
            {
                pIRadSoundHalBufferLoadCallback->OnBufferLoadComplete( 0 );
            }
            return;
        }

        const unsigned int startBytes = m_refIRadSoundHalAudioFormat->FramesToBytes( startPositionInFrames );

        m_pLoadTarget = (unsigned char *) GetData( ) + startBytes;
        m_LoadBytes   = m_refIRadSoundHalAudioFormat->FramesToBytes( numberOfFrames );
        m_refIRadSoundHalBufferLoadCallback = pIRadSoundHalBufferLoadCallback;

        new( "radSoundBufferLoaderCtr", RADMEMORY_ALLOC_TEMP ) radSoundBufferLoaderCtr(
            static_cast< IRadSoundHalBuffer * >( this ),
            m_pLoadTarget,
            pIRadSoundHalDataSource,
            m_refIRadSoundHalAudioFormat,
            numberOfFrames,
            this );
    }

    void ClearAsync(
        unsigned int startPositionInFrames,
        unsigned int numberOfFrames,
        IRadSoundHalBufferClearCallback * pIRadSoundHalBufferClearCallback )
    {
        if( GetData( ) != NULL && !m_Adpcm )
        {
            rAssert( startPositionInFrames + numberOfFrames <= m_SizeInFrames );

            unsigned char * p = (unsigned char *) GetData( ) +
                                m_refIRadSoundHalAudioFormat->FramesToBytes( startPositionInFrames );
            const unsigned int bytes = m_refIRadSoundHalAudioFormat->FramesToBytes( numberOfFrames );
            const int fill = ( m_refIRadSoundHalAudioFormat->GetBitResolution( ) == 8 ) ? 128 : 0;

            ::memset( p, fill, bytes );
            DSP_FlushDataCache( p, bytes );
        }

        if( pIRadSoundHalBufferClearCallback != NULL )
        {
            pIRadSoundHalBufferClearCallback->OnBufferClearComplete( );
        }
    }

    unsigned int GetMinTransferSize( IRadSoundHalAudioFormat::SizeType sizeType )
    {
        rAssert( m_refIRadSoundHalAudioFormat != NULL );
        return m_refIRadSoundHalAudioFormat->ConvertSizeType( sizeType,
            radMemorySpace_OptimalMultiple * m_refIRadSoundHalAudioFormat->GetNumberOfChannels( ),
            IRadSoundHalAudioFormat::Bytes );
    }

    void CancelAsyncOperations( void )
    {
        radSoundBufferLoaderCtr::CancelOperations( static_cast< IRadSoundHalBuffer * >( this ) );
        m_pLoadTarget = NULL;
        m_LoadBytes = 0;
        m_refIRadSoundHalBufferLoadCallback = NULL;
    }

    void ReSetAudioFormat( IRadSoundHalAudioFormat * pIRadSoundHalAudioFormat )
    {
        m_refIRadSoundHalAudioFormat = pIRadSoundHalAudioFormat;
    }

    void OnBufferLoadComplete( unsigned int dataSourceFrames )
    {
        if( m_pLoadTarget != NULL && m_LoadBytes != 0 )
        {
            if( ! m_Streaming && m_pLoadTarget == GetData( ) &&
                m_LoadBytes == GetSizeInBytes( ) && CanEncodeAdpcm( ) )
            {
                EncodeAdpcm( );
                if( GetData( ) != NULL )
                {
                    DSP_FlushDataCache( GetData( ), m_AdpcmBytes );
                }
            }
            else
            {
                DSP_FlushDataCache( m_pLoadTarget, m_LoadBytes );
            }
        }
        m_pLoadTarget = NULL;
        m_LoadBytes = 0;

        if( m_refIRadSoundHalBufferLoadCallback != NULL )
        {
            ref< IRadSoundHalBufferLoadCallback > cb = m_refIRadSoundHalBufferLoadCallback;
            m_refIRadSoundHalBufferLoadCallback = NULL;
            cb->OnBufferLoadComplete( dataSourceFrames );
        }
    }

    void * GetData( void )
    {
        return ( m_refIRadMemoryObject != NULL ) ? m_refIRadMemoryObject->GetMemoryAddress( ) : NULL;
    }

    unsigned int GetSizeInBytes( void )
    {
        return m_refIRadSoundHalAudioFormat != NULL
            ? m_refIRadSoundHalAudioFormat->FramesToBytes( m_SizeInFrames ) : 0;
    }

    unsigned int GetResidentBytes( void )
    {
        return m_Adpcm ? m_AdpcmBytes : GetSizeInBytes( );
    }

    bool IsAdpcm( void ) { return m_Adpcm; }
    ndspAdpcmData * GetAdpcmStart( void ) { return & m_AdpcmStart; }

    private:

    bool CanEncodeAdpcm( void )
    {
        return m_refIRadSoundHalAudioFormat != NULL &&
               m_refIRadSoundHalAudioFormat->GetEncoding( ) == IRadSoundHalAudioFormat::PCM &&
               m_refIRadSoundHalAudioFormat->GetBitResolution( ) == 16 &&
               m_refIRadSoundHalAudioFormat->GetNumberOfChannels( ) == 1 &&
               m_SizeInFrames >= ADPCM_FRAME_SAMPLES &&
               m_refIRadMemoryObject != NULL;
    }

    void EncodeAdpcm( void )
    {
        void * pOld = GetData( );
        const unsigned int bytes = AdpcmEncodeInPlace( pOld, m_SizeInFrames );

        ::memset( & m_AdpcmStart, 0, sizeof( m_AdpcmStart ) );
        m_AdpcmStart.index = ( (u8 *) pOld )[ 0 ];

        radSoundHalMemoryRegion * pRoot = radSoundHalMemoryRegion::GetRootRegion( );
        radSoundHalMemoryRegion * pOwner = ( pRoot != NULL ) ? pRoot->FindOwner( pOld ) : NULL;
        if( pOwner != NULL )
        {
            m_refIRadMemoryObject = NULL;

            ref< IRadMemoryObject > xNew;
            pOwner->CreateMemoryObject( & xNew, bytes, "adpcm clip" );
            if( xNew != NULL )
            {
                ::memmove( xNew->GetMemoryAddress( ), pOld, bytes );
                m_refIRadMemoryObject = xNew;
            }
            else
            {
                m_SizeInFrames = 0;
            }
        }

        m_Adpcm = true;
        m_AdpcmBytes = bytes;
    }

    unsigned int m_SizeInFrames;
    bool m_Looping;
    bool m_Streaming;

    unsigned char * m_pLoadTarget;
    unsigned int m_LoadBytes;

    bool m_Adpcm;
    unsigned int m_AdpcmBytes;
    ndspAdpcmData m_AdpcmStart;

    ref< IRadSoundHalAudioFormat > m_refIRadSoundHalAudioFormat;
    ref< IRadMemoryObject > m_refIRadMemoryObject;
    ref< IRadSoundHalBufferLoadCallback > m_refIRadSoundHalBufferLoadCallback;
};

template<> radSoundHalBufferCtr * radLinkedClass< radSoundHalBufferCtr >::s_pLinkedClassHead = NULL;
template<> radSoundHalBufferCtr * radLinkedClass< radSoundHalBufferCtr >::s_pLinkedClassTail = NULL;

class radSoundHalVoiceCtr
    :
    public IRadSoundHalVoice,
    public radSoundHalPositionalEntity,
    public radLinkedClass< radSoundHalVoiceCtr >,
    public radSoundObject
{
    public:

    IMPLEMENT_REFCOUNTED( "radSoundHalVoiceCtr" )

    radSoundHalVoiceCtr( void )
        :
        m_Priority( 5 ),
        m_Channel( -1 ),
        m_StartOffset( 0 ),
        m_Volume( 1.0f ),
        m_Trim( 1.0f ),
        m_MuteFactor( 1.0f ),
        m_Pan( 0.0f ),
        m_Pitch( 1.0f ),
        m_PosVolume( 1.0f ),
        m_PosPan( 0.0f ),
        m_PosPitch( 1.0f ),
        m_pGroup( NULL )
    {
        ::memset( & m_WaveBuf, 0, sizeof( m_WaveBuf ) );
    }

    virtual ~radSoundHalVoiceCtr( void )
    {
        Stop( );

        if( m_pGroup != NULL )
        {
            m_pGroup->RemovePositionalEntity( this );
            SafeRelease( m_pGroup );
        }
    }

    void SetPriority( unsigned int priority ) { m_Priority = priority; }
    unsigned int GetPriority( void ) { return m_Priority; }

    void SetBuffer( IRadSoundHalBuffer * pIRadSoundHalBuffer )
    {
        Stop( );
        m_StartOffset = 0;
        m_xBuffer = static_cast< radSoundHalBufferCtr * >( pIRadSoundHalBuffer );
    }

    IRadSoundHalBuffer * GetBuffer( void ) { return m_xBuffer; }

    void Play( void )
    {
        if( IsPlaying( ) || !g_NdspAvailable || m_xBuffer == NULL )
        {
            return;
        }

        void * data = m_xBuffer->GetData( );
        IRadSoundHalAudioFormat * pFormat = m_xBuffer->GetFormat( );
        if( data == NULL || pFormat == NULL || m_xBuffer->GetSizeInFrames( ) == 0 )
        {
            return;
        }

        if( m_Channel < 0 )
        {
            m_Channel = AllocChannel( );
            if( m_Channel < 0 )
            {
                return;
            }
        }

        ndspChnWaveBufClear( m_Channel );
        ndspChnReset( m_Channel );
        ndspChnSetInterp( m_Channel, NDSP_INTERP_LINEAR );
        if( m_xBuffer->IsAdpcm( ) )
        {
            ndspChnSetFormat( m_Channel, NDSP_FORMAT_MONO_ADPCM );
            ndspChnSetAdpcmCoefs( m_Channel, s_AdpcmCoefs );
        }
        else
        {
            ndspChnSetFormat( m_Channel, ( pFormat->GetNumberOfChannels( ) > 1 )
                                             ? NDSP_FORMAT_STEREO_PCM16
                                             : NDSP_FORMAT_MONO_PCM16 );
        }
        ApplyRate( );
        ApplyMix( );

        ::memset( & m_WaveBuf, 0, sizeof( m_WaveBuf ) );
        m_WaveBuf.data_vaddr = data;
        if( m_xBuffer->IsAdpcm( ) )
        {
            m_WaveBuf.adpcm_data = m_xBuffer->GetAdpcmStart( );
        }
        m_WaveBuf.nsamples   = m_xBuffer->GetSizeInFrames( );
        m_WaveBuf.looping    = m_xBuffer->IsLooping( );
        m_WaveBuf.offset     = ( m_StartOffset < m_WaveBuf.nsamples ) ? m_StartOffset : 0;

        ndspChnWaveBufAdd( m_Channel, & m_WaveBuf );
    }

    void Stop( void )
    {
        if( m_Channel >= 0 )
        {
            m_StartOffset = ReadPosition( );
            FreeChannel( m_Channel );
            m_Channel = -1;
        }
        m_WaveBuf.status = NDSP_WBUF_FREE;
    }

    bool IsPlaying( void )
    {
        if( m_Channel < 0 )
        {
            return false;
        }

        const u8 status = m_WaveBuf.status;
        if( status == NDSP_WBUF_QUEUED || status == NDSP_WBUF_PLAYING )
        {
            return true;
        }

        if( status == NDSP_WBUF_DONE )
        {
            m_StartOffset = 0;
            FreeChannel( m_Channel );
            m_Channel = -1;
        }
        return false;
    }

    void SetPlaybackPositionInSamples( unsigned int position )
    {
        m_StartOffset = position;
        if( IsPlaying( ) )
        {
            FreeChannel( m_Channel );
            m_Channel = -1;
            m_WaveBuf.status = NDSP_WBUF_FREE;
            Play( );
        }
    }

    unsigned int GetPlaybackPositionInSamples( void )
    {
        if( m_Channel >= 0 && IsPlaying( ) )
        {
            return ReadPosition( );
        }
        return m_StartOffset;
    }

    void SetVolume( float volume )
    {
        ::radSoundVerifyAnalogVolume( volume );
        if( volume != m_Volume ) { m_Volume = volume; ApplyMix( ); }
    }
    float GetVolume( void ) { return m_Volume; }

    void SetTrim( float trim )
    {
        ::radSoundVerifyAnalogVolume( trim );
        if( trim != m_Trim ) { m_Trim = trim; ApplyMix( ); }
    }
    float GetTrim( void ) { return m_Trim; }

    void SetMuted( bool muteOn )
    {
        const float f = muteOn ? 0.0f : 1.0f;
        if( f != m_MuteFactor ) { m_MuteFactor = f; ApplyMix( ); }
    }
    bool GetMuted( void ) { return m_MuteFactor == 0.0f; }

    void SetPan( float pan )
    {
        ::radSoundVerifyAnalogPan( pan );
        if( pan != m_Pan ) { m_Pan = pan; ApplyMix( ); }
    }
    float GetPan( void ) { return m_Pan; }

    void SetPitch( float pitch )
    {
        ::radSoundVerifyAnalogPitch( pitch );
        if( pitch != m_Pitch ) { m_Pitch = pitch; ApplyRate( ); }
    }
    float GetPitch( void ) { return m_Pitch; }

    void SetAuxMode( unsigned int, radSoundAuxMode ) { }
    radSoundAuxMode GetAuxMode( unsigned int ) { return radSoundAuxMode_Off; }
    void SetAuxGain( unsigned int, float ) { }
    float GetAuxGain( unsigned int ) { return 0.0f; }

    void SetPositionalGroup( IRadSoundHalPositionalGroup * pIRshpg )
    {
        radSoundHalPositionalGroup * pGroup = static_cast< radSoundHalPositionalGroup * >( pIRshpg );
        if( pGroup == m_pGroup )
        {
            return;
        }

        if( m_pGroup != NULL )
        {
            m_pGroup->RemovePositionalEntity( this );
            SafeRelease( m_pGroup );
        }

        m_pGroup = pGroup;

        if( m_pGroup != NULL )
        {
            SafeAddRef( m_pGroup );
            m_pGroup->AddPositionalEntity( this );
            m_pGroup->ForceUpdate( );
            OnApplyPositionalSettings( );
        }
        else
        {
            m_PosVolume = 1.0f;
            m_PosPan    = 0.0f;
            m_PosPitch  = 1.0f;
            ApplyMix( );
            ApplyRate( );
        }
    }

    IRadSoundHalPositionalGroup * GetPositionalGroup( void ) { return m_pGroup; }

    void OnApplyPositionalSettings( void )
    {
        if( m_pGroup == NULL )
        {
            return;
        }

        const radSoundHalPositionalInformation & info = m_pGroup->m_RadSoundHalPositionalInformation;
        m_PosVolume = info.m_VolumeAdjust;
        m_PosPan    = info.m_PanAdjust;
        m_PosPitch  = info.m_PitchAdjust;

        ApplyMix( );
        ApplyRate( );
    }

    private:

    unsigned int ReadPosition( void )
    {
        if( m_Channel < 0 )
        {
            return m_StartOffset;
        }
        const unsigned int n   = m_WaveBuf.nsamples;
        const unsigned int pos = ndspChnGetSamplePos( m_Channel );
        return ( n != 0 ) ? ( pos % n ) : 0;
    }

    void ApplyMix( void )
    {
        if( m_Channel < 0 )
        {
            return;
        }

        float analog = m_Trim * m_Volume * m_MuteFactor * m_PosVolume;
        if( analog < 0.0f ) analog = 0.0f;
        if( analog > 1.0f ) analog = 1.0f;
        const float amplitude = analog * analog;

        float pan = m_Pan + m_PosPan;
        if( pan < -1.0f ) pan = -1.0f;
        if( pan >  1.0f ) pan =  1.0f;

        float left, right;
        IRadSoundHalAudioFormat * pFormat = m_xBuffer != NULL ? m_xBuffer->GetFormat( ) : NULL;
        if( pFormat != NULL && pFormat->GetNumberOfChannels( ) > 1 )
        {
            left  = ( pan > 0.0f ) ? ( 1.0f - pan ) : 1.0f;
            right = ( pan < 0.0f ) ? ( 1.0f + pan ) : 1.0f;
        }
        else
        {
            const float angle = ( pan + 1.0f ) * ( 3.14159265f / 4.0f );
            left  = cosf( angle );
            right = sinf( angle );
        }

        float mix[ 12 ];
        ::memset( mix, 0, sizeof( mix ) );
        mix[ 0 ] = amplitude * left;
        mix[ 1 ] = amplitude * right;
        ndspChnSetMix( m_Channel, mix );
    }

    void ApplyRate( void )
    {
        if( m_Channel < 0 || m_xBuffer == NULL || m_xBuffer->GetFormat( ) == NULL )
        {
            return;
        }

        float rate = (float) m_xBuffer->GetFormat( )->GetSampleRate( ) * m_Pitch * m_PosPitch;
        if( rate < 1.0f ) rate = 1.0f;
        ndspChnSetRate( m_Channel, rate );
    }

    unsigned int m_Priority;
    int m_Channel;
    unsigned int m_StartOffset;

    float m_Volume;
    float m_Trim;
    float m_MuteFactor;
    float m_Pan;
    float m_Pitch;

    float m_PosVolume;
    float m_PosPan;
    float m_PosPitch;

    ndspWaveBuf m_WaveBuf;
    ref< radSoundHalBufferCtr > m_xBuffer;
    radSoundHalPositionalGroup * m_pGroup;
};

template<> radSoundHalVoiceCtr * radLinkedClass< radSoundHalVoiceCtr >::s_pLinkedClassHead = NULL;
template<> radSoundHalVoiceCtr * radLinkedClass< radSoundHalVoiceCtr >::s_pLinkedClassTail = NULL;

class radSoundHalSystemCtr
    :
    public IRadSoundHalSystem,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radSoundHalSystemCtr" )

    radSoundHalSystemCtr( radMemoryAllocator allocator )
        :
        m_Allocator( allocator ),
        m_NumAuxSends( 0 ),
        m_LastServiceTime( ::radTimeGetMilliseconds( ) )
    {
        for( unsigned int i = 0; i < s_MaxAuxSends; i++ )
        {
            m_pAuxEffect[ i ] = NULL;
            m_AuxGain[ i ] = 0.0f;
        }
    }

    ~radSoundHalSystemCtr( void )
    {
        for( unsigned int i = 0; i < s_MaxAuxSends; i++ )
        {
            SafeRelease( m_pAuxEffect[ i ] );
        }

        if( g_NdspAvailable )
        {
            for( int c = 0; c < NDSP_NUM_CHANNELS; c++ )
            {
                ndspChnWaveBufClear( c );
            }
            ndspExit( );
            g_NdspAvailable = false;
        }

        radSoundHalMemoryRegion::Terminate( );

        if( g_pSoundMemory != NULL )
        {
            linearFree( g_pSoundMemory );
            g_pSoundMemory = NULL;
        }
    }

    void Initialize( const SystemDescription & systemDescription )
    {
        m_NumAuxSends = systemDescription.m_NumAuxSends;

        g_NdspAvailable = R_SUCCEEDED( ndspInit( ) );
        if( g_NdspAvailable )
        {
            ndspSetOutputMode( NDSP_OUTPUT_STEREO );
            ndspSetOutputCount( 1 );
            ndspSetMasterVol( 1.0f );
        }

        const unsigned int size  = systemDescription.m_ReservedSoundMemory;
        unsigned int       align = ::radSoundHalDataSourceReadAlignmentGet( );
        if( align < 0x80 ) align = 0x80;

        g_pSoundMemory = (unsigned char *) linearMemAlign( size, align );

        rAssert( g_pSoundMemory != NULL );

        radSoundHalMemoryRegion::Initialize(
            g_pSoundMemory,
            size,
            systemDescription.m_MaxRootAllocations,
            ::radSoundHalDataSourceReadAlignmentGet( ),
            radMemorySpace_Local,
            m_Allocator );
    }

    IRadSoundHalMemoryRegion * GetRootMemoryRegion( void )
    {
        return radSoundHalMemoryRegion::GetRootRegion( );
    }

    unsigned int GetNumAuxSends( ) { return m_NumAuxSends; }

    void SetAuxEffect( unsigned int auxNumber, IRadSoundHalEffect * pIRadSoundHalEffect )
    {
        if( auxNumber < s_MaxAuxSends )
        {
            SafeAddRef( pIRadSoundHalEffect );
            SafeRelease( m_pAuxEffect[ auxNumber ] );
            m_pAuxEffect[ auxNumber ] = pIRadSoundHalEffect;
        }
    }

    IRadSoundHalEffect * GetAuxEffect( unsigned int auxNumber )
    {
        return ( auxNumber < s_MaxAuxSends ) ? m_pAuxEffect[ auxNumber ] : NULL;
    }

    void SetAuxGain( unsigned int aux, float gain )
    {
        if( aux < s_MaxAuxSends ) m_AuxGain[ aux ] = gain;
    }

    float GetAuxGain( unsigned int aux )
    {
        return ( aux < s_MaxAuxSends ) ? m_AuxGain[ aux ] : 0.0f;
    }

    void SetOutputMode( radSoundOutputMode mode ) { ( void ) mode; }
    radSoundOutputMode GetOutputMode( void ) { return radSoundOutputMode_Stereo; }

    void Service( void )
    {
        const unsigned int now = ::radTimeGetMilliseconds( );
        radSoundUpdatableObject::UpdateAll( now - m_LastServiceTime );
        m_LastServiceTime = now;
    }

    void ServiceOncePerFrame( void )
    {
        radSoundHalListener::GetInstance( )->UpdatePositionalSettings( );
    }

    void GetStats( IRadSoundHalSystem::Stats * pStats )
    {
        ::memset( pStats, 0, sizeof( IRadSoundHalSystem::Stats ) );

        for( radSoundHalVoiceCtr * v = radSoundHalVoiceCtr::GetLinkedClassHead( );
             v != NULL; v = v->GetLinkedClassNext( ) )
        {
            if( v->GetPositionalGroup( ) != NULL )
            {
                pStats->m_NumPosVoices++;
                if( v->IsPlaying( ) ) pStats->m_NumPosVoicesPlaying++;
            }
            else
            {
                pStats->m_NumVoices++;
                if( v->IsPlaying( ) ) pStats->m_NumVoicesPlaying++;
            }
        }

        for( radSoundHalBufferCtr * b = radSoundHalBufferCtr::GetLinkedClassHead( );
             b != NULL; b = b->GetLinkedClassNext( ) )
        {
            pStats->m_NumBuffers++;
            pStats->m_BufferMemoryUsed += b->GetResidentBytes( );
        }

        radSoundHalMemoryRegion * pRoot = radSoundHalMemoryRegion::GetRootRegion( );
        if( pRoot != NULL )
        {
            unsigned int freeBytes = 0;
            pRoot->GetStats( & freeBytes, NULL, NULL, false );
            pStats->m_TotalFreeSoundMemory = freeBytes;
        }
    }

    private:

    enum { s_MaxAuxSends = 4 };

    radMemoryAllocator m_Allocator;
    unsigned int m_NumAuxSends;
    unsigned int m_LastServiceTime;
    IRadSoundHalEffect * m_pAuxEffect[ s_MaxAuxSends ];
    float m_AuxGain[ s_MaxAuxSends ];
};

static radSoundHalSystemCtr * g_pSystem = NULL;

void radSoundHalSystemInitialize( radMemoryAllocator allocator )
{
    rAssert( g_pSystem == NULL );

    g_pSystem = new( allocator ) radSoundHalSystemCtr( allocator );
    g_pSystem->AddRef( );

    radSoundHalListener::Initialize( allocator );
}

void radSoundHalSystemTerminate( void )
{
    radSoundHalListener::Terminate( );
    SafeRelease( g_pSystem );
}

IRadSoundHalSystem * radSoundHalSystemGet( void )
{
    rAssert( g_pSystem != NULL );
    return g_pSystem;
}

IRadSoundHalVoice * radSoundHalVoiceCreate( radMemoryAllocator allocator )
{
    return new( "radSoundHalVoiceCtr", allocator ) radSoundHalVoiceCtr( );
}

IRadSoundHalBuffer * radSoundHalBufferCreate( radMemoryAllocator allocator )
{
    return new( "radSoundHalBufferCtr", allocator ) radSoundHalBufferCtr( );
}

IRadSoundHalEffectEAX2Reverb * radSoundHalEffectEAX2ReverbCreate( radMemoryAllocator allocator )
{
    ( void ) allocator;
    return NULL;
}

extern "C" void radSoundHal3dsShutdownForExit( void )
{
    if( g_NdspAvailable )
    {
        for( int c = 0; c < NDSP_NUM_CHANNELS; c++ )
        {
            ndspChnWaveBufClear( c );
        }
        ndspExit( );
        g_NdspAvailable = false;
    }
}
