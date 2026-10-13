//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        
//
// Subsystem:	Foundation Technologies - Movie Player
//
// Description:	
//
// Date:    	
//
//=============================================================================

#ifndef BINKRADFILE_HPP
#define BINKRADFILE_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radoptions.hpp>
#ifdef RAD_MOVIEPLAYER_USE_BINK

#include <bink.h>
#include <radmemory.hpp>

void radBinkFileInitialize( radMemoryAllocator alloc );
void radBinkFileTerminate( void );

S32 RADLINK radBinkFileOpen( BINKIO* bio, const char* name, U32 flags );

#endif // RAD_MOVIEPLAYER_USE_BINK
#endif // BINKRADFILE_HPP
