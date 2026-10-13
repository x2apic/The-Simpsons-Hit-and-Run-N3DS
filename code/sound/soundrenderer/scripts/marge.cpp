#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {

#pragma GCC push_options
#pragma GCC optimize("O1")
void daSoundRenderingManager::RunMargeSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_MARGE ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_MARGE );
    #include "marge.inl"
}
#pragma GCC pop_options

}
