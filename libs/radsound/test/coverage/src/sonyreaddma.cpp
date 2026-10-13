//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <stdlib.h>
#include <eekernel.h>
#include <sifdev.h>
#include <sif.h>
#include <stdlib.h>
#include <sifrpc.h>
#include <assert.h>
#include <stdio.h>

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !! This test will crash (TLB exception) in _sceSifCmdIntrHdlr
// !! _sceFsIntrHdlr, or spin in a loop that looks something like this:
// !! 
// !! loop: nop
// !!       nop
// !!       nop
// !!       nop
// !!       branch loop
// !!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!! THIS TEST ALSO FAILS WITH BLOCKING READS ON A THREAD !!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define PAD_CHECK_SIZE ( 256 ) // Verify tail of file transfer buffer
#define TRANSFER_BUFFER_SIZE (1024 * 512 )

// Get Other Data vars
char g_IopToEeDmaTransferBuffer[ TRANSFER_BUFFER_SIZE ] __attribute__((aligned (128)));
sceSifReceiveData g_IopToEeReceiveData __attribute__((aligned (64)));
bool g_IopToEeDmaOutstanding = false;
unsigned int g_NumIopToEeDmas = 0;

// sifDma vars
char g_EeToIopDmaTransferBuffer[ TRANSFER_BUFFER_SIZE ] __attribute__((aligned (128)));
int  g_EeToIopTransactionId = 0;
bool g_EeToIopDmaOutstanding = false;
unsigned int g_NumEeToIopDmas = 0;

// sceRead vars
int g_hFile = -1;
int g_FileSize = 0;
bool g_FileReadOutstanding = false;
char g_FileTransferBuffer[ TRANSFER_BUFFER_SIZE + PAD_CHECK_SIZE ] __attribute__((aligned (128)));
char * g_pPadStart = g_FileTransferBuffer + TRANSFER_BUFFER_SIZE;
unsigned int g_NumReads = 0;

#define FILENAME "host0:stream.rsd"

unsigned int RoundUp( unsigned int size, unsigned int alignment )
{
    if ( alignment != 0 )
    {
        if ( ( size % alignment ) != 0 )
        {
            return size + ( alignment - ( size % alignment ) );
        }
    }
    
    return size;
}

int sonyReadDmaTest( int argc, char * argv[] )
{
    // Fill with known pattern to check for corruption
    
    for( unsigned int i = 0; i < PAD_CHECK_SIZE; i ++ )
    {
        g_pPadStart[ i ] = 0xDB;
    }
    
    // Init remote systems.
    
    ::sceSifInitRpc( 0 );
    ::sceSifInitIopHeap( );
    
    // Open file sychronously, strangely, this is the only way we can
    // get the size (!)

    g_hFile = ::sceOpen( FILENAME, SCE_RDONLY );
    assert( g_hFile >= 0 );
    g_FileSize = sceLseek( g_hFile, 0, SCE_SEEK_END );
    sceClose( g_hFile );
    
    // Close, and reopen in async mode
    
    g_hFile = sceOpen( FILENAME, SCE_RDONLY | SCE_NOWAIT );
    
    //
    // Get some Iop Memory to perform the tests
    //
    
    void * pIopMemory = ::sceSifAllocIopHeap( TRANSFER_BUFFER_SIZE );
    assert( pIopMemory ); 
    
    while( 1 )
    {
        // Do some random Sif Dma's
        
        if ( g_EeToIopDmaOutstanding == false )
        {
            unsigned int bytes = RoundUp( (rand() % TRANSFER_BUFFER_SIZE )+ 1, 64 );

            assert( bytes > 0 );
            assert( bytes <= TRANSFER_BUFFER_SIZE );

            sceSifDmaData dmaData;
            dmaData.data = (unsigned int) g_EeToIopDmaTransferBuffer;
            dmaData.addr = (unsigned int) pIopMemory;   
            dmaData.mode = 0x00;
            dmaData.size = 
            
            g_EeToIopTransactionId = sceSifSetDma( & dmaData, 1 );
            assert( g_EeToIopTransactionId != 0 );   
            g_EeToIopDmaOutstanding = true;                      
        }
        else
        {
            if ( sceSifDmaStat( g_EeToIopTransactionId ) < 0 )
            {
                g_NumEeToIopDmas++;
                g_EeToIopDmaOutstanding = false;
            }
        }
        
        // Do some random GetOtherDatas
        
        if ( g_IopToEeDmaOutstanding )
        {
            if( sceSifCheckStatRpc( (sceSifRpcData* ) & g_IopToEeReceiveData ) == 0 )
            {
                g_NumIopToEeDmas++;
                g_IopToEeDmaOutstanding = false;
            }
            
        }
        else
        {
            int bytes = ( rand() % TRANSFER_BUFFER_SIZE ) + 1;
            
            bytes = RoundUp( bytes, 64 );
            
            assert( bytes <= TRANSFER_BUFFER_SIZE );
            assert( bytes > 0 );            
            
            int ret = ::sceSifGetOtherData
                (
                & g_IopToEeReceiveData,
                (void*) pIopMemory,
                (void*) g_IopToEeDmaTransferBuffer,
                bytes,
                SIF_RPCM_NOWAIT
                );
            
            assert( ret >= 0 );
            
            g_IopToEeDmaOutstanding = true;
            
        }
        
        // Do Some Random File Reads
        
        if ( g_FileReadOutstanding == false )
        {
            int position = rand() % g_FileSize;
            
            int bytes = (rand() % g_FileSize);
            
            if ( (position + bytes) > g_FileSize )
            {
                bytes = ( g_FileSize - position );
            }
            
            if ( bytes > TRANSFER_BUFFER_SIZE )
            {
                bytes = TRANSFER_BUFFER_SIZE;
            }
            
            assert( (position + bytes) <= g_FileSize );
            assert( bytes <= TRANSFER_BUFFER_SIZE );
            
            sceLseek( g_hFile, 0, SCE_SEEK_SET );    
            
            int Executing = 1;
            
            while( Executing )
            {
                sceIoctl( g_hFile, SCE_FS_EXECUTING, &Executing);
            }
            
            sceRead( g_hFile, g_FileTransferBuffer,  bytes );
            
            g_FileReadOutstanding = true;
        }
        else
        {
            int Executing;
            
            sceIoctl( g_hFile, SCE_FS_EXECUTING, &Executing);
            
            if ( ! Executing )
            {
                g_FileReadOutstanding = false;
                g_NumReads++;
            }
        }

        // Check for memory corruption, this sometimes (not always)
        // fails.
        
        for( unsigned int i = 0; i < PAD_CHECK_SIZE; i ++ )
        {
            assert( g_pPadStart[ i ] == (char) 0xDB );
        }
    }
    
    return 0;
}