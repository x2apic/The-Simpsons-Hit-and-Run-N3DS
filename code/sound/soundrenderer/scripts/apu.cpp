#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {

#pragma GCC push_options
#pragma GCC optimize("O1")
void daSoundRenderingManager::RunApuSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_APU ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_APU );
    #include "apu.inl"
}
#pragma GCC pop_options

}
