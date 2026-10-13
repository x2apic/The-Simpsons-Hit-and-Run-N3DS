#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {

#pragma GCC push_options
#pragma GCC optimize("O1")
void daSoundRenderingManager::RunHomerSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_HOMER ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_HOMER );
    #include "homer.inl"
}
#pragma GCC pop_options

}
