//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        win32drive.cpp
//
// Subsystem:   Radical Drive System
//
// Description:	This file contains the implementation of the radWin32Drive class.
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
#include <limits.h>
#include "win32drive.hpp"

//=============================================================================
// Public Functions 
//=============================================================================

//=============================================================================
// Function:    radWin32DriveFactory
//=============================================================================
// Description: This member is responsible for constructing a radWin32DriveObject.
//
// Parameters:  pointer to receive drive object
//              pointer to the drive name
//              allocator
//              
// Returns:     
//------------------------------------------------------------------------------

void radWin32DriveFactory
( 
    radDrive**         ppDrive, 
    const char*        pDriveName,
    radMemoryAllocator alloc
)
{
    //
    // Simply constuct the drive object.
    //
    *ppDrive = new( alloc ) radWin32Drive( pDriveName, alloc );
    rAssert( *ppDrive != NULL );
}


//=============================================================================
// Public Member Functions
//=============================================================================

//=============================================================================
// Function:    radWin32Drive::radWin32Drive
//=============================================================================

radWin32Drive::radWin32Drive( const char* pdrivespec, radMemoryAllocator alloc )
    : 
    radDrive( ),
    m_OpenFiles( 0 ),
    m_SerialNumber( 0 ),
    m_pMutex( NULL )
{
    m_OldErrorSetting = ::SetErrorMode( SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS );

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
    strncpy( m_DriveName, pdrivespec, radFileDrivenameMax );
    m_DriveName[ radFileDrivenameMax ] = '\0';

    //
    // Determine the type of drive and set our capabities under Win32
    //
    UINT dType = ::GetDriveType( pdrivespec );
 
    if( dType == DRIVE_CDROM )
    {
        m_Capabilities = ( radDriveRemovable | radDriveEnumerable | radDriveFile );
    }
    else if( dType == DRIVE_REMOVABLE )
    {
        m_Capabilities = ( radDriveRemovable | radDriveEnumerable | radDriveWriteable | radDriveDirectory | radDriveFile);
    }
    else 
    {
        m_Capabilities = ( radDriveEnumerable | radDriveWriteable | radDriveDirectory | radDriveFile );
    }
}

//=============================================================================
// Function:    radWin32Drive::~radWin32Drive
//=============================================================================

radWin32Drive::~radWin32Drive( void )
{
    m_pMutex->Release( );
    m_pDriveThread->Release( );
    SetErrorMode( m_OldErrorSetting );
}

//=============================================================================
// Function:    radWin32Drive::Lock
//=============================================================================
// Description: Start a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radWin32Drive::Lock( void )
{
    m_pMutex->Lock( );
}

//=============================================================================
// Function:    radWin32Drive::Unlock
//=============================================================================
// Description: End a critical section
//
// Parameters:  
//
// Returns:     
//------------------------------------------------------------------------------

void radWin32Drive::Unlock( void )
{
    m_pMutex->Unlock( );
}

//=============================================================================
// Function:    radWin32Drive::GetCapabilities
//=============================================================================

unsigned int radWin32Drive::GetCapabilities( void )
{
    return m_Capabilities;
}

//=============================================================================
// Function:    radGcnDVDDrive::GetDriveName
//=============================================================================

const char* radWin32Drive::GetDriveName( void )
{
    return m_DriveName;
}

//=============================================================================
// Function:    radWin32Drive::Initialize
//=============================================================================

radDrive::CompletionStatus radWin32Drive::Initialize( void )
{
    if ( m_OpenFiles != 0 )
    {
        //
        // Check if there was a media change
        //
        if ( SetMediaInfo( ) != m_SerialNumber )
        {
            m_LastError = m_MediaInfo.m_MediaState == IRadDrive::MediaInfo::MediaNotPresent ? NoMedia : WrongMedia;
            return Error;
        }
    }
    else
    {
        SetMediaInfo( );
        m_SerialNumber = 0;
    }

    //
    // Success
    //
    m_LastError = Success;
    return Complete;
}

//=============================================================================
// Function:    radWin32Drive::OpenFile
//=============================================================================

radDrive::CompletionStatus radWin32Drive::OpenFile
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
    // Translate flags to windows
    //
    DWORD accessFlag = writeAccess ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ;
    DWORD sharingFlag = writeAccess ? FILE_SHARE_READ | FILE_SHARE_WRITE : FILE_SHARE_READ;                
    DWORD createFlags;    
    switch( flags )
    {
    case OpenExisting:
        createFlags = OPEN_EXISTING;
        break;
    case OpenAlways:
        createFlags = OPEN_ALWAYS;
        break;
    case CreateAlways:
        createFlags = CREATE_ALWAYS;
        break;
    default:
        rAssertMsg( false, "radFileSystem: win32drive: attempting to open file with unknown flag" );
    }

    //
    // Right now using buffered reads, change it later for performance.
    //
    *pHandle = ::CreateFile( fullName, accessFlag, sharingFlag, NULL, 
                             createFlags, FILE_ATTRIBUTE_NORMAL, NULL );

    if ( *pHandle != INVALID_HANDLE_VALUE )
    {
        //
        // Success! Set file size.
        //
        if ( m_OpenFiles == 0 )
        {
            //
            // This is the first file to be opened, so set up the serial number.
            //
            m_SerialNumber = SetMediaInfo( );
        }

        m_OpenFiles++;
        *pSize = ::GetFileSize( *pHandle, NULL );
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = TranslateError( ::GetLastError( ) );
        return Error;
    }
}

//=============================================================================
// Function:    radWin32Drive::CloseFile
//=============================================================================

radDrive::CompletionStatus radWin32Drive::CloseFile( radFileHandle handle, const char* fileName )
{
    ::CloseHandle( handle );
    m_OpenFiles--;
    return Complete;
}

//=============================================================================
// Function:    radWin32Drive::ReadFile
//=============================================================================

radDrive::CompletionStatus radWin32Drive::ReadFile
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
                "radFileSystem: radWin32Drive: External memory not supported for reads." );

    //
    // set file pointer
    //
    if ( ::SetFilePointer( handle, (LONG) position, NULL, FILE_BEGIN ) == (DWORD) position )
    {
        if ( ::ReadFile( handle, pData, bytesToRead, (unsigned long*) bytesRead, NULL ) )
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
    }

    //
    // Failed!
    //
    m_LastError = TranslateError( ::GetLastError( ) );
    return Error;
}

//=============================================================================
// Function:    radWin32Drive::WriteFile
//=============================================================================

radDrive::CompletionStatus radWin32Drive::WriteFile
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
                "radFileSystem: radWin32Drive: External memory not supported for reads." );
    
    //
    // presize the write
    //
    if ( position + bytesToWrite > *pSize )
    {
        ::SetFilePointer( handle, (LONG) position + bytesToWrite, NULL, FILE_BEGIN );
        ::SetEndOfFile( handle );
    }

    //
    // do the write
    //
    if ( ::SetFilePointer( handle, (LONG) position, NULL, FILE_BEGIN ) == (DWORD) position )
    {
        if ( ::WriteFile( handle, pData, bytesToWrite, (unsigned long*) bytesWritten, NULL ) )
        {
            //
            // Sucessful write
            //
            *pSize = ::GetFileSize( handle, NULL );
            m_LastError = Success;
            return Complete;
        }
    }

    //
    // Failed!
    //
    m_LastError = TranslateError( ::GetLastError( ) );
    return Error;
}

//=============================================================================
// Function:    radWin32Drive::FindFirst
//=============================================================================

radDrive::CompletionStatus radWin32Drive::FindFirst
( 
    const char*                 searchSpec, 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    radFileDirHandle*           pHandle,
    bool                        firstSearch
)
{
    //
    // Build the full filename
    //
    char fullSpec[ radFileFilenameMax + 1 ];
    BuildFileSpec( searchSpec, fullSpec, radFileFilenameMax + 1 );

    //
    // Close this search if it already exists
    //
    if ( !firstSearch && *pHandle != INVALID_HANDLE_VALUE )
    {
        ::FindClose( *pHandle );
    }

    //
    // Find first
    //
    WIN32_FIND_DATA FindData;
    *pHandle = ::FindFirstFile( fullSpec, &FindData );

    //
    // Fill in our directory info structure
    //
    m_LastError = TranslateDirInfo( pDirectoryInfo, &FindData, pHandle );

    if ( m_LastError == Success )
    {
        return Complete;
    }
    else
    {
        return Error;
    }
}

//=============================================================================
// Function:    radWin32Drive::FindNext
//=============================================================================

radDrive::CompletionStatus radWin32Drive::FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo )
{
    //
    // If we don't have a handle, return file not found.
    //
    if ( *pHandle == INVALID_HANDLE_VALUE )
    {
        m_LastError = FileNotFound;
        return Error;
    }

    //
    // Find the next entry
    //
    WIN32_FIND_DATA FindData;
    if ( !::FindNextFile( *pHandle, &FindData ) )
    {
        radFileDirHandle h = INVALID_HANDLE_VALUE;
        m_LastError = TranslateDirInfo( pDirectoryInfo, &FindData, &h );
    }
    else
    {
        m_LastError = TranslateDirInfo( pDirectoryInfo, &FindData, pHandle );
    }

    if ( m_LastError == Success )
    {
        return Complete;
    }
    else
    {
        return Error;
    }
}

//=============================================================================
// Function:    radWin32Drive::FindClose
//=============================================================================

radDrive::CompletionStatus radWin32Drive::FindClose( radFileDirHandle* pHandle )
{
    ::FindClose( *pHandle );
    *pHandle = INVALID_HANDLE_VALUE;

    return Complete;
}

//=============================================================================
// Function:    radWin32Drive::CreateDir
//=============================================================================

radDrive::CompletionStatus radWin32Drive::CreateDir( const char* pName )
{
    rWarningMsg( m_Capabilities & radDriveDirectory, 
        "This drive does not support the CreateDir function." );

    //
    // Build the full filename
    //
    char fullSpec[ radFileFilenameMax + 1 ];
    BuildFileSpec( pName, fullSpec, radFileFilenameMax + 1 );

    if ( ::CreateDirectory( fullSpec, NULL ) )
    {
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = TranslateError( ::GetLastError( ) );
        return Error;
    }
}

//=============================================================================
// Function:    radWin32Drive::DestroyDir
//=============================================================================

radDrive::CompletionStatus radWin32Drive::DestroyDir( const char* pName )
{
    rWarningMsg( m_Capabilities & radDriveDirectory,
        "This drive does not support the DestroyDir function." );

    //
    // Build the full filename
    //
    char fullSpec[ radFileFilenameMax + 1 ];
    BuildFileSpec( pName, fullSpec, radFileFilenameMax + 1 );

    if ( ::RemoveDirectory( fullSpec ) )
    {
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = TranslateError( ::GetLastError( ) );
        return Error;
    }
}

//=============================================================================
// Function:    radWin32Drive::DestroyFile
//=============================================================================

radDrive::CompletionStatus radWin32Drive::DestroyFile( const char* filename )
{
    rWarningMsg( m_Capabilities & radDriveWriteable, "This drive does not support the DestroyFile function." );

    //
    // Someday check if the file is open!
    //

    //
    // Build the full filename
    //
    char fullSpec[ radFileFilenameMax + 1 ];
    BuildFileSpec( filename, fullSpec, radFileFilenameMax + 1 );

    if ( ::DeleteFile( fullSpec ) )
    {
        m_LastError = Success;
        return Complete;
    }
    else
    {
        m_LastError = TranslateError( ::GetLastError( ) );
        return Error;
    }
}

//=============================================================================
// Private Member Functions
//=============================================================================

//=============================================================================
// Function:    radWin32Drive::SetMediaInfo
//=============================================================================

DWORD radWin32Drive::SetMediaInfo( void )
{
    //
    // Get volume information.
    //
    const char* realDriveName = m_DriveName;

    char Drive[ 4 ];
    rAssert( strlen( realDriveName ) == 2 );
    strcpy( Drive, realDriveName );
    strcat( Drive, "\\");
    
    DWORD snum;
    if( ::GetVolumeInformation( Drive, m_MediaInfo.m_VolumeName, sizeof( m_MediaInfo.m_VolumeName ),
        &snum, NULL, NULL, NULL, 0 ) )
    {
        //
        // Got media info, so fill in the structure
        //
        m_MediaInfo.m_VolumeName[ sizeof( m_MediaInfo.m_VolumeName) - 1 ] = '\0';
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaPresent; 
        
        ULARGE_INTEGER FreeBytesAvailableToCaller; 
        ULARGE_INTEGER TotalNumberOfBytes; 
        ULARGE_INTEGER TotalNumberOfFreeBytes; 
        
        ::GetDiskFreeSpaceEx( Drive, &FreeBytesAvailableToCaller,          
            &TotalNumberOfBytes, &TotalNumberOfFreeBytes );
        
        if( FreeBytesAvailableToCaller.HighPart != 0 )
        {
            m_MediaInfo.m_FreeSpace = UINT_MAX;
        }
        else
        {
            m_MediaInfo.m_FreeSpace = FreeBytesAvailableToCaller.LowPart;
        }        
    }
    else
    {
        //
        // Don't have media info, so fill structure in with 0s
        //
        snum = 0;
        m_MediaInfo.m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
        m_MediaInfo.m_FreeSpace = 0;
        m_MediaInfo.m_VolumeName[ 0 ] = '\0';
    }

    m_MediaInfo.m_SectorSize = WIN32_DEFAULT_SECTOR_SIZE;

    //
    // No file limit, so set it to the same as free space
    //
    m_MediaInfo.m_FreeFiles = m_MediaInfo.m_FreeSpace / m_MediaInfo.m_SectorSize;

    return snum;
}


//=============================================================================
// Function:    radWin32Drive::BuildFileSpec
//=============================================================================

void radWin32Drive::BuildFileSpec( const char* fileName, char* fullName, unsigned int size )
{
    char fileWithDrive[ radFileFilenameMax + 1 ];
    char* pFile = NULL;

    int len = strlen( m_DriveName );
    strcpy( fileWithDrive, m_DriveName );
    strncpy( fileWithDrive + len, fileName, radFileFilenameMax - len );
    fileWithDrive[ radFileFilenameMax ] ='\0';
    
    ::GetFullPathName( fileWithDrive, size - 1, fullName, &pFile );
    fullName[ size - 1 ] = '\0';
}

//=============================================================================
// Function:    radWin32Drive::TranslateError
//=============================================================================
// Description: This translates the windows error code into our error enums.
//
// Parameters:  Win32 error code
//              
// Returns:     Our error code
//------------------------------------------------------------------------------

radFileError radWin32Drive::TranslateError( DWORD windowsError )
{
    //
    // we shouldn't have success here. If we do we made a mistake earlier.
    //
    switch( windowsError )
    {
    case ERROR_SUCCESS:
        rAssertMsg( false, "radFileSystem: internal error\n" );
        return( Success );

    case ERROR_FILE_NOT_FOUND :
    case ERROR_PATH_NOT_FOUND :
    case ERROR_INVALID_DRIVE :
        return( FileNotFound );

    case ERROR_WRONG_DISK :
    case ERROR_NOT_SAME_DEVICE :
        return( WrongMedia );

    case ERROR_NOT_READY :
        return( ShellOpen );

    case ERROR_BAD_FORMAT :
    case ERROR_SECTOR_NOT_FOUND :
        return( MediaNotFormatted );

    case ERROR_DISK_FULL:
        return ( NoFreeSpace );

    case ERROR_UNRECOGNIZED_VOLUME:
        return ( MediaCorrupt );

    case ERROR_NOT_DOS_DISK:
        return ( MediaInvalid );

    case ERROR_DEVICE_NOT_CONNECTED:
        return( NoMedia );

    default:
        return( HardwareFailure );
    }
}

//=============================================================================
// Function:    radWin32Drive::TranslateDirInfo
//=============================================================================
// Description: Translate the directory info and return an error status. A handle
//              with value INVALID_HANDLE_VALUE means the find_first/next call
//              failed and needs to be checked if something went wrong or of the
//              search just ended.
//
// Parameters:  
//              
// Returns:     
//------------------------------------------------------------------------------

radFileError radWin32Drive::TranslateDirInfo
( 
    IRadDrive::DirectoryInfo*   pDirectoryInfo, 
    const WIN32_FIND_DATA*      pFindData,
    const radFileDirHandle*     pHandle
)
{
    if ( *pHandle == INVALID_HANDLE_VALUE )
    {
        //
        // Either we failed or we're out of games.
        //
        DWORD error = ::GetLastError( );
        if ( error == ERROR_NO_MORE_FILES || error == ERROR_FILE_NOT_FOUND )
        {
            pDirectoryInfo->m_Name[0] = '\0';
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDone;
        }
        else
        {
            return TranslateError( error );
        }
    }
    else
    {
        strncpy( pDirectoryInfo->m_Name, pFindData->cFileName, radFileFilenameMax );
        pDirectoryInfo->m_Name[ radFileFilenameMax ] = '\0';

        if ( pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsDirectory;
        }
        else
        {
            pDirectoryInfo->m_Type = IRadDrive::DirectoryInfo::IsFile;
        }
    }
    return Success;
}
