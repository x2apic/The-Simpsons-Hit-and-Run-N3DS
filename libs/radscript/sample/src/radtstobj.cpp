//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================




//=============================================================================
// Include Files
//=============================================================================
#include "radtstobj.hpp"

#include <raddebug.hpp>
#include <radstring.hpp>
#include <radfactory.hpp>
#include <stdlib.h>
#include <radobject.hpp>
#include <radscript.hpp>
#include <typeinfo>

#ifdef RADSCRIPT_DEBUG
static char dc[] = "scrTstClass";
#endif

extern ref< IRadNameSpace > g_xIRadNameSpace;

struct scrTstClass1;
struct scrTstClass2;

//=============================================================================
// Public Functions
//=============================================================================
static char szBuffer[1024];

//=============================================================================
// Interface : scrTstClass1
//=============================================================================
struct scrTstClass1 :
    public IScrTstClass1,
    public radRefCount
{
    IMPLEMENT_REFCOUNTED( "scrTstClass1" )

    scrTstClass1( ) :
        radRefCount( 0 )
    {
    }

    ~scrTstClass1( )
    {
    }

    void SetBool( bool bVal )
    {
        sprintf( szBuffer, "\tResult : SetBool ( %s )\n", bVal ? "true" : "false" );
        rReleaseString( szBuffer );
    }

    void SetInt( int nVal )
    {
        sprintf( szBuffer, "\tResult : SetInt ( %i )\n", nVal );
        rReleaseString( szBuffer );
    }

    void SetUInt( unsigned int uVal )
    {
        sprintf( szBuffer, "\tResult : SetUInt ( %u )\n", uVal );
        rReleaseString( szBuffer );
    }

    void SetLong( long nVal )
    {
        sprintf( szBuffer, "\tResult : SetLong ( %i )\n", nVal );
        rReleaseString( szBuffer );
    }

    void SetULong( unsigned long uVal )
    {
        sprintf( szBuffer, "\tResult : SetULong ( %u )\n", uVal );
        rReleaseString( szBuffer );
    }

    void SetShort( short nVal )
    {
        sprintf( szBuffer, "\tResult : SetShort ( %i )\n", nVal );
        rReleaseString( szBuffer );
    }

    void SetUShort( unsigned short uVal )
    {
        sprintf( szBuffer, "\tResult : SetUShort ( %u )\n", uVal );
        rReleaseString( szBuffer );
    }

    void SetChar( char nVal )
    {
        sprintf( szBuffer, "\tResult : SetChar ( %i )\n", nVal );
        rReleaseString( szBuffer );
    }

    void SetUChar( unsigned char uVal )
    {
        sprintf( szBuffer, "\tResult : SetUChar ( %u )\n", uVal );
        rReleaseString( szBuffer );
    }

    void SetShortString( const char * pString )
    {
        sprintf( szBuffer, "\tResult : SetShortString ( \"%s\" )\n", pString );
        rReleaseString( szBuffer );
    }

    void SetLongString( const char * pString )
    {
        sprintf( szBuffer, "\tResult : SetLongString ( \"%s\" )\n", pString );
        rReleaseString( szBuffer );
    }

    void SetFloat( float fltVal )
    {
        sprintf( szBuffer, "\tResult : SetFloat ( %f )\n", fltVal );
        rReleaseString( szBuffer );
    }

    void SetRef( IRefCount * pRefCount )
    {
        sprintf( szBuffer, "\tResult : SetRef ( ref:%s )\n", typeid( pRefCount ).name( ) );
        rReleaseString( szBuffer );
    }

    void SetClassInstance( IScrTstClass1 * pTstClass1 )
    {
        sprintf( szBuffer, "\tResult : SetClassInstance ( ref:%s )\n", typeid( pTstClass1 ).name( ) );
        rReleaseString( szBuffer );
    }

    // Parameter test
    void CallNoParam( )
    {
        rReleaseString( "\tResult : CallNoParam( )\n" );
    }

    void CallMultiParam1( bool bVal, int nVal )
    {
        sprintf(
                szBuffer,
                "\tResult : CallMultiParam1( %s %i )\n", 
                bVal ? "true" : "false",
                nVal
               );
        rReleaseString( szBuffer );
    }

    void CallMultiParam2( bool bVal, int nVal, float fltVal )
    {
        sprintf(
                szBuffer,
                "\tResult : CallMultiParam1( %s %i %f )\n", 
                bVal ? "true" : "false",
                nVal,
                fltVal
               );
        rReleaseString( szBuffer );
    }

};


struct scrTstClass2 :
    public IScrTstClass2,
    public scrTstClass1
{
    IMPLEMENT_REFCOUNTED( "scrTstClass1" )

    scrTstClass2( ) :
        scrTstClass1( )
    {
    }

    ~scrTstClass2( )
    {
    }

    void SetBool( bool bVal )
    {
        scrTstClass1::SetBool( bVal );
    }

    void SetInt( int nVal )
    {
        scrTstClass1::SetInt( nVal );
    }

    void SetUInt( unsigned int uVal )
    {
        scrTstClass1::SetUInt( uVal );
    }

    void SetLong( long nVal )
    {
        scrTstClass1::SetLong( nVal );
    }

    void SetULong( unsigned long uVal )
    {
        scrTstClass1::SetULong( uVal );
    }

    void SetShort( short nVal )
    {
        scrTstClass1::SetShort( nVal );
    }

    void SetUShort( unsigned short uVal )
    {
        scrTstClass1::SetUShort( uVal );
    }

    void SetChar( char nVal )
    {
        scrTstClass1::SetChar( nVal );
    }

    void SetUChar( unsigned char uVal )
    {
        scrTstClass1::SetUChar( uVal );
    }

    void SetShortString( const char * pString )
    {
        scrTstClass1::SetShortString( pString );
    }

    void SetLongString( const char * pString )
    {
        scrTstClass1::SetLongString( pString );
    }

    void SetFloat( float fltVal )
    {
        scrTstClass1::SetFloat( fltVal );
    }

    void SetRef( IRefCount * pRefCount )
    {
        scrTstClass1::SetRef( pRefCount );
    }

    void SetClassInstance( IScrTstClass1 * pTstClass1 )
    {
        scrTstClass1::SetClassInstance( pTstClass1 );
    }

    // Parameter test
    void CallNoParam( )
    {
        scrTstClass1::CallNoParam( );
    }

    void CallMultiParam1( bool bVal, int nVal )
    {
        scrTstClass1::CallMultiParam1( bVal, nVal );
    }

    void CallMultiParam2( bool bVal, int nVal, float fltVal )
    {
        scrTstClass1::CallMultiParam2( bVal, nVal, fltVal );
    }

    void CallFromDerivedClass1( )
    {
        sprintf(
                szBuffer,
                "\tResult : CallFromDerivedClass1( )\n"
               );
        rReleaseString( szBuffer );
    }

    void CallFromDerivedClass2( )
    {
        sprintf(
                szBuffer,
                "\tResult : CallFromDerivedClass2( )\n"
               );
        rReleaseString( szBuffer );
    }
};

//============================================================================
// Interface: scrTstClass3
//      This class contain functions only exists in different platform
//      It should test both CPP parser and script very throughly
//============================================================================
struct scrTstClass3 :
    public IScrTstClass3,
    public radRefCount
{
    IMPLEMENT_REFCOUNTED( "scrTstClass3" )

    scrTstClass3( ) :
        radRefCount( 0 )
    {

    }

    ~scrTstClass3( )
    {
    }

//
// all platform
//

    // function in all platform
    virtual void CallDefault( )
    {
        rReleaseString( "\tResult : CallDefault( ).\n" );
    }


};

struct scrTstClassEvent :
    public IScrTstClassEvent,
    public radRefCount
{
    IMPLEMENT_REFCOUNTED( "scrTstClassEvent" )

    scrTstClassEvent( ) :
        radRefCount( 0 ),
        m_pEventHandler( NULL )
    {

    }

    ~scrTstClassEvent( )
    {
    }

    virtual void InvokeEvent( )
    {
        if ( m_pEventHandler )
        {
            m_pEventHandler->OnEventDo( );
        }
    }

    virtual void SetEventHandler( IScrTstClassEventHandler * pEventHandler )
    {
        m_pEventHandler = pEventHandler;
    }

    ref< IScrTstClassEventHandler > m_pEventHandler;
};

struct scrTstClassEventHandler :
    public IScrTstClassEventHandler,
    public radRefCount
{
    IMPLEMENT_REFCOUNTED( "scrTstClassEventHandler" )

    scrTstClassEventHandler( ) :
        radRefCount( 0 )
    {
        IRadString * pString = NULL;

        radStringCreate( & pString );
        rAssert( pString );
        m_strScriptName = pString;
        pString->Release( );
    }

    ~scrTstClassEventHandler( )
    {
    }

    virtual void OnEventDo( )
    {
        ::radScriptLoadAndRunScriptSync( m_strScriptName->GetChars( ), g_xIRadNameSpace, RADMEMORY_ALLOC_TEMP );
    }

    virtual void SetEventScript( const char * pScriptName )
    {
        m_strScriptName->Copy( pScriptName );
    }

    ref< IRadString > m_strScriptName;
};

IScrTstClass1 * scrTstClass1Create( radMemoryAllocator allocator )
{
    return new ( allocator ) scrTstClass1;
}

IScrTstClass2 * scrTstClass2Create( radMemoryAllocator allocator )
{
    return new ( allocator ) scrTstClass2;
}

IScrTstClass3 * scrTstClass3Create( radMemoryAllocator allocator )
{
    return new ( allocator ) scrTstClass3;
}

IScrTstClassEventHandler * scrTstClassEventHandlerCreate( radMemoryAllocator allocator )
{
    return new ( allocator ) scrTstClassEventHandler;
}

IScrTstClassEvent * scrTstClassEventCreate( radMemoryAllocator allocator )
{
    return new ( allocator ) scrTstClassEvent;
}

void scrTstClassRegisterFactories( void )
{
    radFactoryRegister( "scrTstClass1", (radFactoryProc*) scrTstClass1Create );
    radFactoryRegister( "scrTstClass2", (radFactoryProc*) scrTstClass2Create );
    radFactoryRegister( "scrTstClass3", (radFactoryProc*) scrTstClass3Create );
    radFactoryRegister( "scrTstClassEventHandler", (radFactoryProc*) scrTstClassEventHandlerCreate );
    radFactoryRegister( "scrTstClassEvent", (radFactoryProc*) scrTstClassEventCreate );
}
