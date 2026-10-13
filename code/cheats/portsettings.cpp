#include <cheats/portsettings.h>

#include <stdio.h>
#include <string.h>

namespace
{
#ifdef RAD_3DS
    const char* SETTINGS_FILE = "sdmc:/3ds/simpsons/srr2_settings.ini";
#else
    const char* SETTINGS_FILE = "srr2_settings.ini";
#endif

    bool s_Loaded   = false;
    bool s_NoTimers = false;
    bool s_MapOnTop = false;

    void LoadOnce()
    {
        if( s_Loaded )
        {
            return;
        }
        s_Loaded = true;

        FILE* f = fopen( SETTINGS_FILE, "r" );
        if( f == NULL )
        {
            return;
        }

        char line[ 64 ];
        while( fgets( line, sizeof( line ), f ) != NULL )
        {
            int value = 0;
            if( sscanf( line, "notimers=%d", &value ) == 1 )
            {
                s_NoTimers = ( value != 0 );
            }
            else if( sscanf( line, "maptop=%d", &value ) == 1 )
            {
                s_MapOnTop = ( value != 0 );
            }
        }
        fclose( f );
    }
}

bool PortSettings::NoTimers()
{
    LoadOnce();
    return s_NoTimers;
}

void PortSettings::SetNoTimers( bool enabled )
{
    LoadOnce();
    s_NoTimers = enabled;
}

bool PortSettings::MapOnTop()
{
    LoadOnce();
    return s_MapOnTop;
}

void PortSettings::SetMapOnTop( bool onTop )
{
    LoadOnce();
    s_MapOnTop = onTop;
}

void PortSettings::Save()
{
    LoadOnce();

    FILE* f = fopen( SETTINGS_FILE, "w" );
    if( f == NULL )
    {
        return;
    }
    fprintf( f, "notimers=%d\n", s_NoTimers ? 1 : 0 );
    fprintf( f, "maptop=%d\n", s_MapOnTop ? 1 : 0 );
    fclose( f );
}
