#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {

#pragma GCC push_options
#pragma GCC optimize("O1")
void daSoundRenderingManager::RunLisaSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_LISA ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_LISA );
    #include "lisa.inl"
}
#pragma GCC pop_options

}
