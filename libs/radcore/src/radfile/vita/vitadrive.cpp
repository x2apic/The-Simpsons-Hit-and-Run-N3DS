//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        sdldrive.cpp
//
// Subsystem:   Radical Drive System
//
// Description:	This file contains the implementation of the radVitaDrive class.
//
// Revisions:
//
// Notes:       We keep a serial number when the first file is opened. Then if the
//              media is removed, we don't allow ops until the original serial number
//              is detected, or all files are closed.
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <algorithm>
#include <limits.h>
#include "vitadrive.hpp"
#include <string>
#include <psp2/io/devctl.h>
#include <psp2/io/fcntl.h>
#include <unistd.h>

//=============================================================================
// Public Functions 
//=============================================================================

//=============================================================================
// Function:    radVitaDriveFactory
//=============================================================================
// Description: This member is responsible for constructing a radVitaDriveObject.
//
// Parameters:  pointer to receive drive object
//              pointer to the drive name
//              allocator
//              
// Returns:     
//------------------------------------------------------------------------------

void radVitaDriveFactory
( 
    radDrive**         ppDrive, 
    const char*        pDriveName,
    radMemoryAllocator alloc
)
{
    //
    // Simply constuct the drive object.
    //
    *ppDrive = new( alloc ) radVitaDrive( pDriveName, alloc );
    rAssert( *ppDrive != NULL );
}


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radVitaDrive::radVitaDrive
//=============================================================================

radVitaDrive::radVitaDrive( const char* pdrivespec, radMemoryAllocator alloc )
    : 
    radDrive( ),
    m_OpenFiles( 0 ),
    m_pMutex( NULL )
{
    //
    // Create a mutex for lock/unlock
    //
    radThreadCreateMutex( &m_pMutex, alloc );
    rAssert( m_pMutex != NULL );

    //
    // Create the drive thread.
    //
    m_pDriveThread = new( alloc ) radDriveThread( m_pMutex, alloc );
    rAssert( m_pDriveThread != NULL );

    //
    // Copy the drivename
    //
    radGetDefaultDrive( m_DriveName );
    if ( strcmp(m_DriveName, pdrivespec ) != 0 )
    {
        strncpy( m_DriveName, pdrivespec, radFileDrivenameMax );
        strncpy( m_DrivePath, pdrivespec, radFileFilenameMax );
        m_DriveName[radFileDrivenameMax] = '\0';
        m_DrivePath[radFileFilenameMax] = '\0';
        strupr( m_DriveName );
        strlwr( m_DrivePath );
    }

    if(!m_DrivePath[0])
    {
        getcwd( m_DrivePath, radFileFilenameMax );
        strncat(m_DrivePath, "/", radFileFilenameMax);
        m_DrivePath[radFileFilenameMax] = '\0';
    }

    m_Capabilities = ( radDriveWriteable | radDriveFile );
}

//=============================================================================
// Function:    radVitaDrive::~radVitaDrive
//=============================================================================

radVitaDrive::~radVitaDrive( void )
{
    m_pMutex->Release( );
    m_pDriveThread->Release( );
}

//=============================================================================
// Function:    radVitaDrive::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radVitaDrive::Lock( void )
{
    m_pMutex->Lock( );
}

//=============================================================================
// Function:    radVitaDrive::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radVitaDrive::Unlock( void )
{
    m_pMutex->Unlock( );
}

//=============================================================================
// Function:    radVitaDrive::GetCapabilities
//=============================================================================

unsigned int radVitaDrive::GetCapabilities( void )
{
    return m_Capabilities;
}

//=============================================================================
// Function:    radGcnDVDDrive::GetDriveName
//=============================================================================

const char* radVitaDrive::GetDriveName( void )
{
    return m_DriveName;
}

//=============================================================================
// Function:    radVitaDrive::Initialize
//=============================================================================

radDrive::CompletionStatus radVitaDrive::Initialize( void )
{
    SetMediaInfo();

    //
    // Success
    //
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radVitaDrive::OpenFile
//=============================================================================

radDrive::CompletionStatus radVitaDrive::OpenFile
( 
    const char*         fileName, 
    radFileOpenFlags    flags, 
    bool                writeAccess, 
    radFileHandle*      pHandle, 
    unsigned int*       pSize 
)
{
    //
    // Build the full filename
    //
    char fullName[ radFileFilenameMax + 1 ];
    BuildFileSpec( fileName, fullName, radFileFilenameMax + 1 );

    //
    // Translate flags to SDL
    //
    int createFlags = writeAccess ? SCE_O_RDWR : SCE_O_RDONLY;
    switch( flags )
    {
    case OpenExisting:
        break;
    case OpenAlways:
        createFlags |= SCE_O_CREAT;
        break;
    case CreateAlways:
        createFlags |= SCE_O_CREAT | SCE_O_TRUNC;
        break;
    default:
        rAssertMsg( false, "radFileSystem: vitadrive: attempting to open file with unknown flag" );
        return Error;
    }

    *pHandle = (radFileHandle)sceIoOpen( fullName, createFlags, 0777 );

    if ( *pHandle )
    {
        m_OpenFiles++;
        *pSize = sceIoLseek((SceUID)*pHandle, 0, SCE_SEEK_END);
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = FileNotFound;
        return Error;
    }
}

//=============================================================================
// Function:    radVitaDrive::CloseFile
//=============================================================================

radDrive::CompletionStatus radVitaDrive::CloseFile( radFileHandle handle, const char* fileName )
{
    sceIoClose( (SceUID)handle );
    m_OpenFiles--;
    return Complete;
}

//=============================================================================
// Function:    radVitaDrive::ReadFile
//=============================================================================

radDrive::CompletionStatus radVitaDrive::ReadFile
( 
    radFileHandle   handle, 
    const char*     fileName,
    IRadFile::BufferedReadState buffState,
    unsigned int    position, 
    void*           pData, 
    unsigned int    bytesToRead, 
    unsigned int*   bytesRead, 
    radMemorySpace  pDataSpace 
)
{
    rAssertMsg( pDataSpace == radMemorySpace_Local, 
                "radFileSystem: radVitaDrive: External memory not supported for reads." );

    //
    // set file pointer
    //
    if ( sceIoPread( (SceUID)handle, pData, bytesToRead, position ) >= 0 )
    {
        //
        // Successful read!
        //
            
        //
        // Change this during buffered read!!
        //
        *bytesRead = bytesToRead;
        m_LastError = Success;
        return Complete;
    }

    //
    // Failed!
    //
    m_LastError = FileNotFound;
    return Error;
}

//=============================================================================
// Function:    radVitaDrive::WriteFile
//=============================================================================

radDrive::CompletionStatus radVitaDrive::WriteFile
( 
    radFileHandle     handle,
    const char*       fileName,
    IRadFile::BufferedReadState buffState,
    unsigned int      position, 
    const void*       pData, 
    unsigned int      bytesToWrite, 
    unsigned int*     bytesWritten, 
    unsigned int*     pSize, 
    radMemorySpace    pDataSpace 
)
{
    if ( !( m_Capabilities & radDriveWriteable ) )
    {
        rWarningMsg( m_Capabilities & radDriveWriteable, "This drive does not support the WriteFile function." );
        return Error;
    }

    rAssertMsg( pDataSpace == radMemorySpace_Local, 
                "radFileSystem: radVitaDrive: External memory not supported for reads." );

    //
    // do the write
    //
    *bytesWritten = sceIoPwrite( (SceUID)handle, pData, bytesToWrite, position );
    if ( *bytesWritten == bytesToWrite )
    {
        //
        // Sucessful write
        //
        *pSize = sceIoLseek((SceUID)handle, 0, SCE_SEEK_END);
        m_LastError = Success;
        return Complete;
    }

    //
    // Failed!
    //
    m_LastError = FileNotFound;
    return Error;
}

//=============================================================================
// Private Member Functions
//=============================================================================

//=============================================================================
// Function:    radVitaDrive::SetMediaInfo
//=============================================================================

void radVitaDrive::SetMediaInfo( void )
{
    //
    // Get volume information.
    //
    const char* realDriveName = m_DriveName;

    //rAssert( strlen( realDriveName ) == 2 );
    strcpy(m_MediaInfo.m_VolumeName, realDriveName );
    //strcat(m_MediaInfo.m_VolumeName, "\\");

    m_MediaInfo.m_SectorSize = VITA_DEFAULT_SECTOR_SIZE;

    SceIoDevInfo info;
    int error = sceIoDevctl("ux0:", 0x3001, NULL, 0, &info, sizeof(SceIoDevInfo));
    if(!error)
    {
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent;
        m_MediaInfo.m_FreeSpace = info.free_size;
        m_MediaInfo.m_SectorSize = info.cluster_size;

        //
        // No file limit, so set it to the available space
        //
        m_MediaInfo.m_FreeFiles = m_MediaInfo.m_FreeSpace / m_MediaInfo.m_SectorSize;
        m_LastError = Success;
    }
    else
    {
        //
        // Don't have media info, so fill structure in with dummy info
        //
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent;
        m_MediaInfo.m_FreeSpace = UINT_MAX;
        m_MediaInfo.m_FreeFiles = m_MediaInfo.m_FreeSpace / m_MediaInfo.m_SectorSize;
        m_LastError = Success;
    }
}

//=============================================================================
// Function:    radVitaDrive::BuildFileSpec
//=============================================================================

void radVitaDrive::BuildFileSpec( const char* fileName, char* fullName, unsigned int size )
{
    std::string path(m_DrivePath);
    path += fileName;
    std::replace(path.begin(), path.end(), '\\', '/');

    strncpy( fullName, path.c_str(), size - 1 );
    fullName[ size - 1 ] = '\0';
}
