//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        platformdrives.hpp
//
// Subsystem:	Radical File System
//
// Description:	This file contains the platform specific names of all drives.
//              It also contains a function to give back the name of the
//              default drive. This allows our other classes to be platform
//              independent.
//
// Revisions:	
//
//=============================================================================

#ifndef	PLATFORMDRIVES_HPP
#define PLATFORMDRIVES_HPP

#include <raddebug.hpp>
#include <radmemory.hpp>
#include <radfile.hpp>

class radDrive;

//=============================================================================
// Typedefs
//=============================================================================

#if defined( RAD_WIN32 ) || defined( RAD_XBOX ) || defined( RAD_SDL )
typedef void* radFileHandle;
typedef void* radFileDirHandle;
#endif

#ifdef RAD_PS2
typedef unsigned int radFileHandle;

//
// When we have thread safe allocation, make this dynamic, since it's a lot of
// stuff to just carry around when you don't use it.
//
#define RAD_PS2_SPEC_LEN    31 // length of wildcard part of search spec
struct radFileDirHandle
{
    char m_pSpec[ RAD_PS2_SPEC_LEN + 1 ]; // search spec
    unsigned int m_Lsn;                   // lsn and offset are the location of current one
    unsigned int m_Offset;
    unsigned int m_RemSect;               // number of entries left to search
};
#endif

#ifdef RAD_GAMECUBE
#include <dolphin/dvd.h>
typedef unsigned int radFileHandle;

//
// Structure for memory cards
//
struct MEMDir
{
    unsigned int m_fileNo;
};

#define RAD_GCN_SPEC_LEN    31
struct radFileDirHandle
{
    char m_pSpec[ RAD_GCN_SPEC_LEN + 1 ];
    union
    {
        DVDDir  m_DVD;
        MEMDir  m_MEM;

    };
};
#endif

//=============================================================================
// Functions
//=============================================================================

//
// Default default drive
//
void PlatformDrivesGetDefaultDrive( char* driveSpec );

//
// Indicate whether we have a valid drive
//
bool PlatformDrivesValidateDriveName( const char* driveSpec );

//
// Get the drive object, name should be validated.
//
void PlatformDrivesFactory( class radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

#endif // PLATFORMDRIVES_HPP
