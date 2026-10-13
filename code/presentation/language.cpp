//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        language.cpp
//
// Description: contains functions for dealing with language
//
// History:     21/1/2002 + Created -- Ian Gipson
//
//=============================================================================

//========================================
// System Includes
//========================================
#ifdef RAD_PS2
    #include <libscf.h>
#endif

#ifdef RAD_GAMECUBE
    #include <dolphin/os.h>
#endif

#ifdef RAD_XBOX
    #include <xtl.h>
#endif

#ifdef __SWITCH__
    #include <switch.h>
#endif

#ifdef RAD_VITA
    #include <psp2/apputil.h>
    #include <psp2/system_param.h>
#endif

//========================================
// Project Includes
//========================================

#include <presentation/language.h>

//*****************************************************************************
//
// Global Data, Local Data, Local Classes
//
//*****************************************************************************

//*****************************************************************************
//
// Public Member Functions
//
//*****************************************************************************
namespace Language{

//=============================================================================
// Language::GetHardwareLanguage()
//=============================================================================
// Description: returns the currently set language for the console.
//
// Parameters: None.
//
// Return:      Language enum specifying the current language
//
//=============================================================================
Language GetHardwareLanguage()
{
    #ifdef RAD_PS2
    ////////////////////////////////////////////////////////////
    // PS2
    ////////////////////////////////////////////////////////////
    switch ( sceScfGetLanguage() )
    {
        case SCE_DUTCH_LANGUAGE : 
        {
            return DUTCH;
        }
        case SCE_ENGLISH_LANGUAGE : 
        {
            return ENGLISH;
        }
        case SCE_FRENCH_LANGUAGE : 
        {
            return FRENCH;
        }
        case SCE_GERMAN_LANGUAGE : 
        {
            return GERMAN;
        }
        case SCE_ITALIAN_LANGUAGE : 
        {
            return ITALIAN;
        }
        case SCE_JAPANESE_LANGUAGE : 
        {
            return JAPANESE;
        }
        case SCE_PORTUGUESE_LANGUAGE : 
        {
            return PORTUGUESE;
        }
        case SCE_SPANISH_LANGUAGE : 
        {
            return SPANISH;
        }
        default : 
        {
            return UNKNOWN;
        }
    }
    #endif

    #ifdef RAD_XBOX
    ////////////////////////////////////////////////////////////
    // XBOX
    ////////////////////////////////////////////////////////////
    switch ( XGetLanguage() )
    {
        case XC_LANGUAGE_ENGLISH : 
        {
            return ENGLISH;
        }
        case XC_LANGUAGE_FRENCH : 
        {
            return FRENCH;
        }
        case XC_LANGUAGE_GERMAN : 
        {
            return GERMAN;
        }
        case XC_LANGUAGE_ITALIAN : 
        {
            return ITALIAN;
        }
        case XC_LANGUAGE_JAPANESE : 
        {
            return JAPANESE;
        }
        case XC_LANGUAGE_SPANISH : 
        {
            return SPANISH;
        }
        default : 
        {
            return UNKNOWN;
        }
    }
    #endif

    #ifdef RAD_GAMECUBE
    switch ( OSGetLanguage() )
    {
        case OS_LANG_ENGLISH:
        {
            return ENGLISH;
        }
        case OS_LANG_GERMAN:
        {
            return GERMAN;
        }
        case OS_LANG_FRENCH:
        {
            return FRENCH;
        }
        case OS_LANG_SPANISH:
        {
            return SPANISH;
        }
        case OS_LANG_ITALIAN:
        {
            return ITALIAN;
        }
        case OS_LANG_DUTCH:
        {
            return DUTCH;
        }
        default:
        {
            return UNKNOWN;
        }
    }
    #endif

    #ifdef __SWITCH__
    if( R_FAILED( setInitialize() ) )
        return UNKNOWN;

    u64 code;
    SetLanguage lang;
    Result rc = setGetSystemLanguage( &code );
    if( R_SUCCEEDED( rc ) )
        rc = setMakeLanguage( code, &lang );
    setExit();

    if( R_FAILED( rc ) )
        return UNKNOWN;

    switch ( lang )
    {
        case SetLanguage_JA:
        {
            return JAPANESE;
        }
        case SetLanguage_ENUS:
        case SetLanguage_ENGB:
        {
            return ENGLISH;
        }
        case SetLanguage_FR:
        case SetLanguage_FRCA:
        {
            return FRENCH;
        }
        case SetLanguage_DE:
        {
            return GERMAN;
        }
        case SetLanguage_IT:
        {
            return ITALIAN;
        }
        case SetLanguage_ES:
        case SetLanguage_ES419:
        {
            return SPANISH;
        }
        case SetLanguage_NL:
        {
            return DUTCH;
        }
        case SetLanguage_PT:
        case SetLanguage_PTBR:
        {
            return PORTUGUESE;
        }
        default:
        {
            return UNKNOWN;
        }
    }
    #elif RAD_VITA
    SceAppUtilInitParam init = { 0 };
    SceAppUtilBootParam boot = { 0 };
    if( sceAppUtilInit( &init, &boot ) < 0)
        return UNKNOWN;

    SceSystemParamLang lang;
    if( sceAppUtilSystemParamGetInt( SCE_SYSTEM_PARAM_ID_LANG, reinterpret_cast<int*>( &lang ) ) < 0 )
        return UNKNOWN;

    switch ( lang )
    {
        case SCE_SYSTEM_PARAM_LANG_JAPANESE:
        {
            return JAPANESE;
        }
        case SCE_SYSTEM_PARAM_LANG_ENGLISH_US:
        case SCE_SYSTEM_PARAM_LANG_ENGLISH_GB:
        {
            return ENGLISH;
        }
        case SCE_SYSTEM_PARAM_LANG_FRENCH:
        {
            return FRENCH;
        }
        case SCE_SYSTEM_PARAM_LANG_SPANISH:
        {
            return SPANISH;
        }
        case SCE_SYSTEM_PARAM_LANG_GERMAN:
        {
            return GERMAN;
        }
        case SCE_SYSTEM_PARAM_LANG_ITALIAN:
        {
            return ITALIAN;
        }
        case SCE_SYSTEM_PARAM_LANG_DUTCH:
        {
            return DUTCH;
        }
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT:
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
        {
            return PORTUGUESE;
        }
        default:
        {
            return UNKNOWN;
        }
    }
    #elif RAD_WIN32
    ////////////////////////////////////////////////////////////
    // WIN32
    ////////////////////////////////////////////////////////////
    return UNKNOWN;     // to be implemented.
    #endif
}

} //namespace Language