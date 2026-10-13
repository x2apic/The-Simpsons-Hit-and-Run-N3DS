#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {

#pragma GCC push_options
#pragma GCC optimize("O1")
void daSoundRenderingManager::RunBartSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_BART ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_BART );
    #include "bart.inl"
}
#pragma GCC pop_options

}
