//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File: radtstobj.hpp
//
// Subsystem:	Testing Object
//
// Description:	
//
//=============================================================================

#ifndef RADTSTOBJ_HPP
#define RADTSTOBJ_HPP

//============================================================================
// Include Files
//============================================================================

#include <radobject.hpp>
#include <radfactory.hpp>
#include <radnamespace.hpp>
#include <radtypeinfo.hpp>

//============================================================================
// Forward Declarations
//============================================================================

struct IScrTstClass1;
struct IScrTstClass2;
struct IScrTstClass3;

struct IScrTstClassEvent;
struct IScrTstClassEventHandler;
//============================================================================
// Functions
//============================================================================
void scrTstClassRegisterFactories( void );

IScrTstClass1 * scrTstClass1Create( radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );

IScrTstClass2 * scrTstClass2Create( radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );

IScrTstClass3 * scrTstClass3Create( radMemoryAllocator = RADMEMORY_ALLOC_DEFAULT );

//============================================================================
// Interface: IScrTstClass1
//============================================================================

struct IScrTstClass1 : public IRefCount
{
    //
    // basic buildin type
    //

    virtual void SetBool( bool bVal ) = 0;

    virtual void SetInt( int nVal ) = 0;
    virtual void SetUInt( unsigned int uVal ) = 0;

    virtual void SetLong( long nVal ) = 0;
    virtual void SetULong( unsigned long uVal ) = 0;

    virtual void SetShort( short nVal ) = 0;
    virtual void SetUShort( unsigned short uVal ) = 0;

    virtual void SetChar( char nVal ) = 0;
    virtual void SetUChar( unsigned char uVal ) = 0;

    virtual void SetShortString( const char * pString ) = 0;
    virtual void SetLongString( const char * pString ) = 0;

    virtual void SetFloat( float fltVal ) = 0;

    virtual void SetRef( IRefCount * pRefCount ) = 0;
    virtual void SetClassInstance( IScrTstClass1 * pTstClass1 ) = 0;

    // Parameter test
    virtual void CallNoParam( ) = 0;
    virtual void CallMultiParam1( bool bVal, int nVal ) = 0;
    virtual void CallMultiParam2( bool bVal, int nVal, float fltVal ) = 0;
};

//============================================================================
// Interface: IScrTstClass2
//            Testing inheritence
//============================================================================
struct IScrTstClass2 : public IScrTstClass1
{
    virtual void CallFromDerivedClass1( ) = 0;
    virtual void CallFromDerivedClass2( ) = 0;
};

//============================================================================
// Interface: IScrTstClass3
//      This class contain functions only exists in different platform
//============================================================================
struct IScrTstClass3 : public IRefCount
{
    // function in all platform
    virtual void CallDefault( ) = 0;
};

struct IScrTstClassEvent : public IRefCount
{
    virtual void InvokeEvent( ) = 0;
    virtual void SetEventHandler( IScrTstClassEventHandler * pEventHandler ) = 0;
};

struct IScrTstClassEventHandler : public IRefCount
{
    virtual void OnEventDo( ) = 0;
    virtual void SetEventScript( const char * pScriptName ) = 0;
};

#endif // RADTSTOBJ_HPP
