//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include "xboxwma.hpp"
#include "initradcore.hpp"
#include "initradsound.hpp"
#include <radsound.hpp>
#include <radsound_xbox.hpp>


int xboxWmaTest( int argc, char * argv[] )
{
    ::rstRadCoreInitialize( );
    ::rstRadSoundInitialize( );

    // One way to do wma... (not the soundtrack method)

    ref< IRadFile > refIRadFile = NULL;
    ::radFileOpen( & refIRadFile, "wma.rsd" );
    refIRadFile->WaitForCompletion( );

    ref< IRadSoundWmaFileDataSource > refIRadSoundWmafileDataSource = ::radSoundWmaFileDataSourceCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundWmafileDataSource->InitializeFromFile( refIRadFile );

    ref< IRadSoundStreamPlayer > refIRadSoundStreamPlayer = ::radSoundStreamPlayerCreate( RADMEMORY_ALLOC_DEFAULT );
    refIRadSoundStreamPlayer->InitializeAsync(
        3000,
        IRadSoundHalAudioFormat::Milliseconds,
        ::radSoundHalSystemGet( )->GetRootMemoryRegion( ),
        "StreamPlayer" );
    refIRadSoundStreamPlayer->SetDataSource( refIRadSoundWmafileDataSource );

    refIRadFile = NULL;
    refIRadSoundWmafileDataSource = NULL;

    refIRadSoundStreamPlayer->Play( );

    while( true )
    {
        ::rstRadCoreService( );
        ::rstRadSoundService( );     
    }

    ::rstRadSoundTerminate( );
    ::rstRadCoreTerminate( );

    return 0;
}
