//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"

#include <radtypeinfo.hpp>

#if defined (RAD_WIN32) || defined (RAD_XBOX)
#include "win32/win32typeinfovfcall.h"
#endif

#ifdef RAD_PS2
#include "ps2/ps2typeinfovfcall.h"
#endif

#ifdef RAD_GAMECUBE
#include "gcn/gcntypeinfovfcall.h"
#endif

//============================================================================
// Component: IRadTypeInfoMethod
//============================================================================

//========================================================================
// IRadTypeInfoMethod::Invoke
//========================================================================
unsigned int IRadTypeInfoMethod::Invoke
(
    void * pI,
    void * pParams,
    unsigned int numParams
) 
{
    if ( pI == NULL )
    {
       rDebugString( "IRadTypeInfoMethod: Invoke() called on NULL object!\n" ); 
    }
    else
    {
        if ( numParams != m_lstParamsPtr->GetSize( ) )
        {
            rDebugString( "IRadTypeInfoMethod: [" );
            rDebugString( GetName( ) );
            rDebugString( "] was called incorrectly!\n" );
        }
        else
        {
            //
            //  ---- RAD_WIN32 or RAD_XBOX ----------------------------------------------------
            // 
                 
            #if defined (RAD_WIN32) || defined (RAD_XBOX)

                return InvokeVf( pI, m_VTableOffset, pParams, numParams );

            #endif
           
            //
            //  ---- RAD_PS2 ----------------------------------------------------
            //
           
            #ifdef RAD_PS2        
           
                unsigned int pIntParams[ 7 ];
                float        pFloatParams[ 8 ];

                int          ip = 0;
                int          fp = 0;

                for( unsigned int i = 0; i < numParams; i ++ )
                {
                    if ( GetParamInfoAt( i )->GetIndLvl( ) > 0 || GetParamInfoAt( i )->GetHashedType( ) != ParserConst::TOK_FLOAT )
                    {
                        rAssert( ip < 7 );                   
                        pIntParams[ ip++ ] = ((unsigned int*)pParams)[i];                            
                    }
                    else
                    {
                        rAssert( fp < 8 );
                        pFloatParams[ fp++ ] = ((float*)pParams)[ i ];
                    }
                }

                int ret = InvokeVf( pI, m_VTableOffset + 1, pIntParams, pFloatParams ); 
            
                if ( GetReturnInfo( )->GetHashedType( ) == ParserConst::TOK_BOOL )
                {
                    return ret & 0x000000FF;
                }
            
                return ret;
            
            #endif
                
            //
            //  ---- RAD_GAMECUBE ------------------------------------------------------
            //                    
            
            #ifdef RAD_GAMECUBE            
           
                unsigned int pIntParams[ 7 ];
                float        pFloatParams[ 8 ];

                int          ip = 0;
                int          fp = 0;

                for( unsigned int i = 0; i < numParams; i ++ )
                {
                    if ( GetParamInfoAt( i )->GetIndLvl( ) > 0 || GetParamInfoAt( i )->GetHashedType( ) != ParserConst::TOK_FLOAT )
                    {
                        rAssert( ip < 7 );

                        pIntParams[ ip++ ] = ((unsigned int*)pParams)[ i ];
                    }
                    else
                    {
                        rAssert( fp < 8 );
                        pFloatParams[ fp++ ] = ((float*)pParams)[ i ];
                    }
                }
            
                int ret = InvokeVf( pI, m_VTableOffset + 1, pIntParams, pFloatParams );

                return ret;
            
            #endif          
       }
   }
   
   return 0; 
}


