#include <radoptions.hpp>

#ifdef RAD_MOVIEPLAYER_CTR
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <raddebug.hpp>
#include <radmovie2.hpp>
#include <radobject.hpp>

extern "C" int  radSoundCtrAllocChannel( void );
extern "C" void radSoundCtrFreeChannel( int channel );

const char * radMovieDebugChannel2 = "radMovie";
unsigned int const radMovie_NoAudioTrack = 0xFFFFFFFF;

namespace
{
    struct MovieHeader
    {
        char magic[ 8 ];
        u32  version;
        u16  width;
        u16  height;
        u32  fpsNum;
        u32  fpsDen;
        u32  frameCount;
        u32  audioRate;
        u16  audioChannels;
        u16  reserved;
        u32  audioOffset;
        u32  audioBytes;
        u32  videoOffset;
        u32  videoBytes;
    };

    const u32 MOVIE_VERSION      = 1;
    const u32 NAL_START_CAPACITY = 32 * 1024;
    const int AUDIO_BUFFERS      = 4;
    const u32 AUDIO_BUFFER_BYTES = 8 * 1024;

    const u32 LINEAR_RESERVE     = 256 * 1024;
    const int MAX_PLAYERS        = 4;
    const int MAX_CATCHUP_FRAMES = 4;

    void ConvertedName( const char * in, char * out, size_t outSize )
    {
        snprintf( out, outSize, "%s", in );
        char * dot = strrchr( out, '.' );
        char * slash = strrchr( out, '/' );
        if( dot != NULL && ( slash == NULL || dot > slash ) )
        {
            *dot = '\0';
        }
        strncat( out, ".3dm", outSize - strlen( out ) - 1 );
    }

    bool HasDecoder( void )
    {
        s64 emulator = 0;
        if( R_SUCCEEDED( svcGetSystemInfo( &emulator, 0x20000, 0 ) ) && emulator == 1 )
        {
            return false;
        }
        bool isNew3DS = false;
        if( R_FAILED( APT_CheckNew3DS( &isNew3DS ) ) || !isNew3DS )
        {
            return false;
        }
        return true;
    }

    u32 CalculateWorkSize( u16 width, u16 height )
    {
        srvSetBlockingPolicy( true );
        Result rc = mvdstdInit( MVDMODE_COLORFORMATCONV, MVD_INPUT_YUYV422, MVD_OUTPUT_BGR565, 0, NULL );
        srvSetBlockingPolicy( false );
        if( R_FAILED( rc ) )
        {
            return 0;
        }

        MVDSTD_CalculateWorkBufSizeConfig calc;
        memset( &calc, 0, sizeof( calc ) );
        calc.ref_frames_a.enable     = 1;
        calc.ref_frames_a.ref_frames = 2;
        calc.width  = width;
        calc.height = height;
        u32 byRefs = 0;
        const Result rcRefs = mvdstdCalculateBufferSize( &calc, &byRefs );

        mvdstdExit( );

        return ( R_SUCCEEDED( rcRefs ) && byRefs > 0 ) ? byRefs : 0;
    }
}

class radCtrMoviePlayer
    :
    public IRadMoviePlayer2,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radCtrMoviePlayer" )

    radCtrMoviePlayer( void );
    ~radCtrMoviePlayer( void );

    void Initialize( IRadMovieRenderLoop * pIRadMovieRenderLoop,
                     IRadMovieRenderStrategy * pIRadMovieRenderStrategy );

    bool Render( void );
    void Load( const char * pFileName, unsigned int audioTrackIndex );
    void Unload( void );
    void Play( void );
    void Pause( void );

    void  SetVolume( float volume );
    float GetVolume( void ) { return m_Volume; }
    void  SetPan( float pan ) { m_Pan = pan; }
    float GetPan( void ) { return m_Pan; }

    State GetState( void ) { return m_State; }
    bool  GetVideoFrameInfo( VideoFrameInfo * pFrameInfo );

    float        GetFrameRate( void ) { return 1000.0f; }
    unsigned int GetCurrentFrameNumber( void ) { return m_PresentationMs; }

    void Service( void );

    void Suspend( void );
    void Resume( void );

    static radCtrMoviePlayer * s_Players[ MAX_PLAYERS ];

    private:

    bool OpenFiles( void );
    void CloseFiles( void );
    bool StartAudio( void );
    void StopAudio( void );
    void PumpAudio( void );
    bool DecodeFrame( void );
    void CopyFrameToStrategy( void );
    u64  Elapsed( void ) const;
    void Fail( void );
    void FreeResources( void );

    ref< IRadMovieRenderLoop >     m_refRenderLoop;
    ref< IRadMovieRenderStrategy > m_refRenderStrategy;

    State       m_State;
    char        m_Path[ 128 ];
    MovieHeader m_Header;

    FILE *      m_Video;
    FILE *      m_Audio;
    u32         m_VideoPos;
    u32         m_AudioPos;
    bool        m_Suspended;

    bool          m_MvdUp;
    u8 *          m_Nal;
    u32           m_NalCapacity;
    u16 *         m_Out;
    MVDSTD_Config m_Config;

    int         m_Channel;
    u8 *        m_Pcm;
    ndspWaveBuf m_Wave[ AUDIO_BUFFERS ];
    int         m_NextFill;
    bool        m_AudioDone;

    u32         m_FramesDecoded;
    bool        m_FrameReady;
    u32         m_PresentationMs;
    u64         m_StartMs;
    u64         m_PausedMs;
    u64         m_SuspendMs;
    float       m_Volume;
    float       m_Pan;
};

radCtrMoviePlayer * radCtrMoviePlayer::s_Players[ MAX_PLAYERS ];

static void radCtrMovieAptHook( APT_HookType hook, void * )
{
    for( int i = 0; i < MAX_PLAYERS; i++ )
    {
        if( radCtrMoviePlayer::s_Players[ i ] != NULL )
        {
            if( hook == APTHOOK_ONSUSPEND || hook == APTHOOK_ONSLEEP )
            {
                radCtrMoviePlayer::s_Players[ i ]->Suspend( );
            }
            else if( hook == APTHOOK_ONRESTORE || hook == APTHOOK_ONWAKEUP )
            {
                radCtrMoviePlayer::s_Players[ i ]->Resume( );
            }
        }
    }
}

radCtrMoviePlayer::radCtrMoviePlayer( void )
    :
    radRefCount( 0 ),
    m_State( NoData ),
    m_Video( NULL ),
    m_Audio( NULL ),
    m_VideoPos( 0 ),
    m_AudioPos( 0 ),
    m_Suspended( false ),
    m_MvdUp( false ),
    m_Nal( NULL ),
    m_NalCapacity( 0 ),
    m_Out( NULL ),
    m_Channel( -1 ),
    m_Pcm( NULL ),
    m_NextFill( 0 ),
    m_AudioDone( true ),
    m_FramesDecoded( 0 ),
    m_FrameReady( false ),
    m_PresentationMs( 0 ),
    m_StartMs( 0 ),
    m_PausedMs( 0 ),
    m_SuspendMs( 0 ),
    m_Volume( 1.0f ),
    m_Pan( 0.0f )
{
    m_Path[ 0 ] = '\0';
    memset( &m_Header, 0, sizeof( m_Header ) );
    memset( &m_Config, 0, sizeof( m_Config ) );
    memset( m_Wave, 0, sizeof( m_Wave ) );

    static bool s_hooked = false;
    if( !s_hooked )
    {
        static aptHookCookie s_cookie;
        aptHook( &s_cookie, radCtrMovieAptHook, NULL );
        s_hooked = true;
    }

    for( int i = 0; i < MAX_PLAYERS; i++ )
    {
        if( s_Players[ i ] == NULL )
        {
            s_Players[ i ] = this;
            break;
        }
    }
}

radCtrMoviePlayer::~radCtrMoviePlayer( void )
{
    FreeResources( );

    for( int i = 0; i < MAX_PLAYERS; i++ )
    {
        if( s_Players[ i ] == this )
        {
            s_Players[ i ] = NULL;
        }
    }
}

void radCtrMoviePlayer::Initialize
(
    IRadMovieRenderLoop * pIRadMovieRenderLoop,
    IRadMovieRenderStrategy * pIRadMovieRenderStrategy
)
{
    rAssert( pIRadMovieRenderStrategy != NULL );
    m_refRenderLoop = pIRadMovieRenderLoop;
    m_refRenderStrategy = pIRadMovieRenderStrategy;
    m_State = NoData;
}

void radCtrMoviePlayer::Load( const char * pFileName, unsigned int audioTrackIndex )
{
    rAssert( m_State == NoData );
    FreeResources( );

    m_State = Loading;
    ConvertedName( pFileName, m_Path, sizeof( m_Path ) );

    m_FramesDecoded  = 0;
    m_FrameReady     = false;
    m_PresentationMs = 0;
    m_PausedMs       = 0;

    m_Video = fopen( m_Path, "rb" );
    if( m_Video == NULL )
    {
        Fail( );
        return;
    }

    if( fread( &m_Header, sizeof( m_Header ), 1, m_Video ) != 1 ||
        memcmp( m_Header.magic, "SRR3DSMV", 8 ) != 0 ||
        m_Header.version != MOVIE_VERSION ||
        m_Header.width == 0 || m_Header.height == 0 ||
        m_Header.width > 512 || m_Header.height > 256 ||
        m_Header.fpsNum == 0 || m_Header.fpsDen == 0 )
    {
        Fail( );
        return;
    }

    if( !HasDecoder( ) )
    {
        Fail( );
        return;
    }

    u32 workSize = CalculateWorkSize( m_Header.width, m_Header.height );
    if( workSize == 0 )
    {
        workSize = MVD_DEFAULT_WORKBUF_SIZE;
    }

    {
        u32 texW = 8, texH = 8;
        while( texW < m_Header.width )  texW <<= 1;
        while( texH < m_Header.height ) texH <<= 1;
        const u32 need = workSize + NAL_START_CAPACITY + m_Header.width * m_Header.height * 2 +
                         AUDIO_BUFFERS * AUDIO_BUFFER_BYTES + texW * texH * 2;
        if( linearSpaceFree( ) < need + LINEAR_RESERVE )
        {
            Fail( );
            return;
        }
    }

    Result rc = -1;
    for( int attempt = 0; attempt < 20 && R_FAILED( rc ); attempt++ )
    {
        if( attempt > 0 )
        {
            svcSleepThread( 10000000LL );
        }
        srvSetBlockingPolicy( true );
        rc = mvdstdInit( MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264, MVD_OUTPUT_BGR565, workSize, NULL );
        srvSetBlockingPolicy( false );
    }
    if( R_FAILED( rc ) )
    {
        Fail( );
        return;
    }
    m_MvdUp = true;

    m_NalCapacity = NAL_START_CAPACITY;
    m_Nal = (u8 *) linearMemAlign( m_NalCapacity, 0x40 );
    m_Out = (u16 *) linearMemAlign( m_Header.width * m_Header.height * 2, 0x40 );
    if( m_Nal == NULL || m_Out == NULL )
    {
        Fail( );
        return;
    }

    mvdstdGenerateDefaultConfig( &m_Config, m_Header.width, m_Header.height,
                                 m_Header.width, m_Header.height,
                                 NULL, (u32 *) m_Out, (u32 *) m_Out );

    m_VideoPos = m_Header.videoOffset;
    fseek( m_Video, (long) m_VideoPos, SEEK_SET );

    if( audioTrackIndex != radMovie_NoAudioTrack && m_Header.audioBytes > 0 &&
        m_Header.audioRate > 0 && m_Header.audioChannels > 0 )
    {
        StartAudio( );
    }

    m_refRenderStrategy->ChangeParameters( m_Header.width, m_Header.height );
    m_refRenderStrategy->ResetDestinations( );
    IRadMovieRenderStrategy::LockedDestination probe;
    if( m_refRenderStrategy->LockNextDestination( &probe ) == 0 )
    {
        Fail( );
        return;
    }
    m_refRenderStrategy->UnlockDestination( );

    m_State = ReadyToPlay;
}

void radCtrMoviePlayer::Fail( void )
{
    FreeResources( );
    m_State = NoData;
}

void radCtrMoviePlayer::FreeResources( void )
{
    StopAudio( );
    if( m_MvdUp )
    {
        mvdstdExit( );
        m_MvdUp = false;
    }
    if( m_Nal != NULL ) { linearFree( m_Nal ); m_Nal = NULL; }
    m_NalCapacity = 0;
    if( m_Out != NULL ) { linearFree( m_Out ); m_Out = NULL; }
    CloseFiles( );
    m_FrameReady = false;
}

void radCtrMoviePlayer::Unload( void )
{
    if( m_State != NoData )
    {
        FreeResources( );
        m_State = NoData;
    }
}

bool radCtrMoviePlayer::OpenFiles( void )
{
    m_Video = fopen( m_Path, "rb" );
    if( m_Video == NULL )
    {
        return false;
    }
    fseek( m_Video, (long) m_VideoPos, SEEK_SET );

    if( m_Channel >= 0 )
    {
        m_Audio = fopen( m_Path, "rb" );
        if( m_Audio == NULL )
        {
            return false;
        }
        fseek( m_Audio, (long) m_AudioPos, SEEK_SET );
    }
    return true;
}

void radCtrMoviePlayer::CloseFiles( void )
{
    if( m_Video != NULL ) { fclose( m_Video ); m_Video = NULL; }
    if( m_Audio != NULL ) { fclose( m_Audio ); m_Audio = NULL; }
}

void radCtrMoviePlayer::Suspend( void )
{
    if( m_State == NoData || m_Suspended )
    {
        return;
    }
    m_Suspended = true;
    m_SuspendMs = osGetTime( );
    if( m_Channel >= 0 )
    {
        ndspChnSetPaused( m_Channel, true );
    }
    CloseFiles( );
}

void radCtrMoviePlayer::Resume( void )
{
    if( !m_Suspended )
    {
        return;
    }
    m_Suspended = false;
    m_StartMs += osGetTime( ) - m_SuspendMs;
    if( !OpenFiles( ) )
    {
        Fail( );
        return;
    }
    if( m_Channel >= 0 && m_State == Playing )
    {
        ndspChnSetPaused( m_Channel, false );
    }
}

bool radCtrMoviePlayer::StartAudio( void )
{
    m_Channel = radSoundCtrAllocChannel( );
    if( m_Channel < 0 )
    {
        return false;
    }
    m_Pcm = (u8 *) linearAlloc( AUDIO_BUFFERS * AUDIO_BUFFER_BYTES );
    m_Audio = fopen( m_Path, "rb" );
    if( m_Pcm == NULL || m_Audio == NULL )
    {
        StopAudio( );
        return false;
    }
    m_AudioPos = m_Header.audioOffset;
    fseek( m_Audio, (long) m_AudioPos, SEEK_SET );

    ndspChnReset( m_Channel );
    ndspChnSetInterp( m_Channel, NDSP_INTERP_LINEAR );
    ndspChnSetRate( m_Channel, (float) m_Header.audioRate );
    ndspChnSetFormat( m_Channel, ( m_Header.audioChannels > 1 ) ? NDSP_FORMAT_STEREO_PCM16
                                                                : NDSP_FORMAT_MONO_PCM16 );
    SetVolume( m_Volume );

    ndspChnSetPaused( m_Channel, true );
    memset( m_Wave, 0, sizeof( m_Wave ) );
    for( int i = 0; i < AUDIO_BUFFERS; i++ )
    {
        m_Wave[ i ].data_vaddr = m_Pcm + i * AUDIO_BUFFER_BYTES;
        m_Wave[ i ].status = NDSP_WBUF_DONE;
    }
    m_NextFill = 0;
    m_AudioDone = false;
    PumpAudio( );
    return true;
}

void radCtrMoviePlayer::StopAudio( void )
{
    if( m_Channel >= 0 )
    {
        ndspChnWaveBufClear( m_Channel );
        ndspChnReset( m_Channel );
        radSoundCtrFreeChannel( m_Channel );
        m_Channel = -1;
    }
    if( m_Pcm != NULL )
    {
        linearFree( m_Pcm );
        m_Pcm = NULL;
    }
    m_AudioDone = true;
}

void radCtrMoviePlayer::PumpAudio( void )
{
    if( m_Channel < 0 || m_AudioDone || m_Audio == NULL )
    {
        return;
    }

    const u32 frameBytes = 2 * m_Header.audioChannels;
    const u32 audioEnd = m_Header.audioOffset + m_Header.audioBytes;

    for( int n = 0; n < AUDIO_BUFFERS; n++ )
    {
        ndspWaveBuf & wave = m_Wave[ m_NextFill ];
        if( wave.status != NDSP_WBUF_DONE && wave.status != NDSP_WBUF_FREE )
        {
            break;
        }

        u32 want = AUDIO_BUFFER_BYTES;
        if( m_AudioPos + want > audioEnd )
        {
            want = audioEnd - m_AudioPos;
        }
        want -= want % frameBytes;
        const u32 got = ( want > 0 ) ? (u32) fread( (void *) wave.data_vaddr, 1, want, m_Audio ) : 0;
        if( got < frameBytes )
        {
            m_AudioDone = true;
            break;
        }
        m_AudioPos += got;

        DSP_FlushDataCache( wave.data_vaddr, got );
        wave.nsamples = got / frameBytes;
        wave.looping = false;
        ndspChnWaveBufAdd( m_Channel, & wave );

        m_NextFill = ( m_NextFill + 1 ) % AUDIO_BUFFERS;
    }
}

void radCtrMoviePlayer::SetVolume( float volume )
{
    m_Volume = volume;
    if( m_Channel >= 0 )
    {
        float mix[ 12 ];
        memset( mix, 0, sizeof( mix ) );
        mix[ 0 ] = mix[ 1 ] = volume;
        ndspChnSetMix( m_Channel, mix );
    }
}

void radCtrMoviePlayer::Play( void )
{
    if( m_State == ReadyToPlay )
    {
        m_StartMs = osGetTime( ) - m_PausedMs;
        m_State = Playing;
        if( m_Channel >= 0 )
        {
            ndspChnSetPaused( m_Channel, false );
        }
        Service( );
    }
    else if( m_State == Loading )
    {
        m_State = LoadToPlay;
    }
}

void radCtrMoviePlayer::Pause( void )
{
    if( m_State == Playing )
    {
        m_PausedMs = Elapsed( );
        m_State = ReadyToPlay;
        if( m_Channel >= 0 )
        {
            ndspChnSetPaused( m_Channel, true );
        }
    }
    else if( m_State == LoadToPlay )
    {
        m_State = Loading;
    }
}

u64 radCtrMoviePlayer::Elapsed( void ) const
{
    return osGetTime( ) - m_StartMs;
}

bool radCtrMoviePlayer::DecodeFrame( void )
{
    for( ;; )
    {
        u32 size = 0;
        if( m_Video == NULL || fread( &size, sizeof( size ), 1, m_Video ) != 1 || size == 0 )
        {
            return false;
        }
        if( size > m_NalCapacity )
        {
            linearFree( m_Nal );
            m_NalCapacity = ( size + 0x7fff ) & ~0x7fffu;
            m_Nal = (u8 *) linearMemAlign( m_NalCapacity, 0x40 );
            if( m_Nal == NULL )
            {
                m_NalCapacity = 0;
                return false;
            }
        }
        if( fread( m_Nal, 1, size, m_Video ) != size )
        {
            return false;
        }
        m_VideoPos += sizeof( size ) + size;

        GSPGPU_FlushDataCache( m_Nal, size );
        MVDSTD_ProcessNALUnitOut out;
        memset( &out, 0, sizeof( out ) );
        Result rc = mvdstdProcessVideoFrame( m_Nal, size, 0, &out );
        if( !MVD_CHECKNALUPROC_SUCCESS( rc ) )
        {
            return false;
        }
        if( rc == MVD_STATUS_PARAMSET || rc == MVD_STATUS_INCOMPLETEPROCESSING )
        {
            continue;
        }

        rc = mvdstdRenderVideoFrame( &m_Config, true );
        if( rc != MVD_STATUS_OK )
        {
            return false;
        }
        m_FramesDecoded++;
        return true;
    }
}

void radCtrMoviePlayer::CopyFrameToStrategy( void )
{
    const u32 w = m_Header.width;
    const u32 h = m_Header.height;
    GSPGPU_InvalidateDataCache( m_Out, w * h * 2 );

    m_refRenderStrategy->ResetDestinations( );

    IRadMovieRenderStrategy::LockedDestination dest;
    while( m_refRenderStrategy->LockNextDestination( &dest ) > 0 )
    {
        for( u32 y = 0; y < dest.m_Height && dest.m_SrcPosY + y < h; y++ )
        {
            const u16 * src = m_Out + ( dest.m_SrcPosY + y ) * w + dest.m_SrcPosX;
            u32 * dst = (u32 *) ( (u8 *) dest.m_pDest +
                                  ( dest.m_DestPosY + y ) * dest.m_DestPitch ) + dest.m_DestPosX;
            for( u32 x = 0; x < dest.m_Width && dest.m_SrcPosX + x < w; x++ )
            {
                const u32 p = src[ x ];
                const u32 r = ( ( ( p >> 11 ) & 31 ) * 527 + 23 ) >> 6;
                const u32 g = ( ( ( p >> 5 ) & 63 ) * 259 + 33 ) >> 6;
                const u32 b = ( ( p & 31 ) * 527 + 23 ) >> 6;
                dst[ x ] = 0xff000000u | ( b << 16 ) | ( g << 8 ) | r;
            }
        }
        m_refRenderStrategy->UnlockDestination( );
    }
}

void radCtrMoviePlayer::Service( void )
{
    if( m_State != Playing || m_Suspended )
    {
        return;
    }

    PumpAudio( );

    if( !m_FrameReady )
    {
        const u64 elapsed = Elapsed( );
        const u64 due = ( elapsed * m_Header.fpsNum ) / ( 1000ull * m_Header.fpsDen );
        if( m_FramesDecoded > due )
        {
            return;
        }

        int decoded = 0;
        while( m_FramesDecoded <= due && decoded < MAX_CATCHUP_FRAMES )
        {
            if( !DecodeFrame( ) )
            {
                Unload( );
                return;
            }
            decoded++;
        }

        CopyFrameToStrategy( );
        m_PresentationMs = (u32) ( ( (u64) ( m_FramesDecoded - 1 ) * 1000ull * m_Header.fpsDen ) /
                                   m_Header.fpsNum );
        m_FrameReady = true;
    }

    if( m_FrameReady && m_refRenderLoop != NULL )
    {
        m_refRenderLoop->IterateLoop( this );
    }
}

bool radCtrMoviePlayer::Render( void )
{
    const bool ret = m_refRenderStrategy->Render( );
    m_FrameReady = false;
    return ret;
}

bool radCtrMoviePlayer::GetVideoFrameInfo( VideoFrameInfo * pFrameInfo )
{
    rAssert( pFrameInfo != NULL );
    if( m_State == Playing || m_State == ReadyToPlay )
    {
        pFrameInfo->Width  = m_Header.width;
        pFrameInfo->Height = m_Header.height;
        return true;
    }
    return false;
}

IRadMoviePlayer2 * radMoviePlayerCreate2( radMemoryAllocator alloc )
{
    return new( alloc ) radCtrMoviePlayer( );
}

void radMovieInitialize2( radMemoryAllocator alloc )
{
    ( void ) alloc;
}

void radMovieTerminate2( void )
{
}

void radMovieService2( void )
{
    for( int i = 0; i < MAX_PLAYERS; i++ )
    {
        if( radCtrMoviePlayer::s_Players[ i ] != NULL )
        {
            radCtrMoviePlayer::s_Players[ i ]->Service( );
        }
    }
}
#endif
