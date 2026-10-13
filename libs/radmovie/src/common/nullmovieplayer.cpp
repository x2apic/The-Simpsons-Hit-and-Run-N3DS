#include <radoptions.hpp>

#ifdef RAD_MOVIEPLAYER_NULL
#include <raddebug.hpp>
#include <radmovie2.hpp>
#include <radobject.hpp>

class radNullMoviePlayer
    :
    public IRadMoviePlayer2,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radNullMoviePlayer" )

    radNullMoviePlayer( void ) : m_Volume( 1.0f ), m_Pan( 0.0f ) { }
    ~radNullMoviePlayer( void ) { }

    void Initialize(
        IRadMovieRenderLoop * pIRadMovieRenderLoop,
        IRadMovieRenderStrategy * pIRadMovieRenderStrategy )
    {
        ( void ) pIRadMovieRenderLoop;
        ( void ) pIRadMovieRenderStrategy;
    }

    bool Render( void ) { return false; }

    void Load( const char * pFileName, unsigned int audioTrackIndex )
    {
        ( void ) pFileName;
        ( void ) audioTrackIndex;
    }

    void Unload( void ) { }
    void Play( void ) { }
    void Pause( void ) { }

    void  SetVolume( float volume ) { m_Volume = volume; }
    float GetVolume( void ) { return m_Volume; }
    void  SetPan( float pan ) { m_Pan = pan; }
    float GetPan( void ) { return m_Pan; }

    State GetState( void ) { return NoData; }

    bool GetVideoFrameInfo( VideoFrameInfo * pFrameInfo )
    {
        ( void ) pFrameInfo;
        return false;
    }

    float GetFrameRate( void ) { return 0.0f; }
    unsigned int GetCurrentFrameNumber( void ) { return 0; }

    private:

    float m_Volume;
    float m_Pan;
};

IRadMoviePlayer2 * radMoviePlayerCreate2( radMemoryAllocator alloc )
{
    return new( alloc )radNullMoviePlayer( );
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
}
#endif
