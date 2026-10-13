//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        win32drive.hpp
//
// Subsystem:	Radical Drive System
//
// Description:	This file contains all definitions and classes relevant to
//              a WIN32 physical drive.
//
// Revisions:	
//
//=============================================================================

#ifndef	WIN32DRIVE_HPP
#define WIN32DRIVE_HPP

//=============================================================================
// Include Files
//=============================================================================
#include <windows.h>
#include "../common/drive.hpp"
#include "../common/drivethread.hpp"

//=============================================================================
// Defines
//=============================================================================

//
// Disk read alignment values.
//
#define WIN32_DEFAULT_SECTOR_SIZE  512

//=============================================================================
// Public Functions
//=============================================================================

//
// Every physical drive type must provide a drive factory.
//
void radWin32DriveFactory( radDrive** ppDrive, const char* driveSpec, radMemoryAllocator alloc );

/*
//
// Constructs a cached version of the Win32 drive
//
void radWin32CacheDriveFactory( radDrive** ppDrive,
                                radDrive* pOriginalDrive,
                                radFileSystem* pFileSystem,
                                const char* driveSpec,
                                radMemoryAllocator alloc );
*/

//=============================================================================
// Class Declarations
//=============================================================================

//
// This is a Win32 Drive. It implements the appropriate radDrive members. 
//
class radWin32Drive : public radDrive
{
public:

    //
    // Constructor / destructor.
    //
    radWin32Drive( const char* pdrivespec, radMemoryAllocator alloc );
    virtual ~radWin32Drive( void );

    void Lock( void );
    void Unlock( void );

    unsigned int GetCapabilities( void );

    const char* GetDriveName( void );
 
    CompletionStatus Initialize( void );

//    CompletionStatus OpenCacheFile( const char* fileName, radFileOpenFlags flags, bool writeAccess, bool fileAlreadyLoaded,
//                                              unsigned int* pBaseOffset, void* pHandle, unsigned int* pSize );

    CompletionStatus OpenFile( const char*        fileName, 
                               radFileOpenFlags   flags, 
                               bool               writeAccess, 
                               radFileHandle*     pHandle, 
                               unsigned int*      pSize );

    CompletionStatus CloseFile( radFileHandle handle, const char* fileName );

    CompletionStatus ReadFile( radFileHandle      handle,
                               const char*        fileName,
                               IRadFile::BufferedReadState buffState,
                               unsigned int       position, 
                               void*              pData, 
                               unsigned int       bytesToRead, 
                               unsigned int*      bytesRead, 
                               radMemorySpace     pDataSpace );

    CompletionStatus WriteFile( radFileHandle     handle,
                                const char*       fileName,
                                IRadFile::BufferedReadState buffState,
                                unsigned int      position, 
                                const void*       pData, 
                                unsigned int      bytesToWrite, 
                                unsigned int*     bytesWritten, 
                                unsigned int*     size, 
                                radMemorySpace    pDataSpace );

    CompletionStatus FindFirst( const char*                 searchSpec, 
                                IRadDrive::DirectoryInfo*   pDirectoryInfo, 
                                radFileDirHandle*           pHandle,
                                bool                        firstSearch );

    CompletionStatus FindNext( radFileDirHandle* pHandle, IRadDrive::DirectoryInfo* pDirectoryInfo );

    CompletionStatus FindClose( radFileDirHandle* pHandle );

    CompletionStatus CreateDir( const char* pName );

    CompletionStatus DestroyDir( const char* pName );   

    CompletionStatus DestroyFile( const char* filename );

private:
    DWORD SetMediaInfo( void );
    void BuildFileSpec( const char* fileName, char* fullName, unsigned int size );
    radFileError TranslateError( DWORD windowsError );
    radFileError TranslateDirInfo( IRadDrive::DirectoryInfo*   pDirectoryInfo, 
                                   const WIN32_FIND_DATA*      pFindData,
                                   const radFileDirHandle*     pHandle );

    unsigned int    m_Capabilities;
    UINT            m_OldErrorSetting;
    unsigned int    m_OpenFiles;
    char            m_DriveName[ radFileDrivenameMax + 1 ];
    DWORD           m_SerialNumber;

    //
    // Mutex for critical sections
    //
    IRadThreadMutex*    m_pMutex;
};

#endif // WIN32DRIVE_HPP

