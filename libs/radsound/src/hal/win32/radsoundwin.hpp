//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUNDWIN_HPP
#define RADSOUNDWIN_HPP

#include <al.h>

//============================================================================
// Helper Functions
//============================================================================

unsigned int radSoundFloatAngleToULongWin( float angle );
float        radSoundULongAngleToFloatWin( unsigned int angle );
float        radSoundVolumeDbToHardwareWin( float volume );
float        radSoundVolumeHardwareToDbWin( float hardwareVolume );

#endif // RADSOUNDWIN32_HPP
