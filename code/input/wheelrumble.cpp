//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        wheelrumble.cpp
//
// Description: Implement WheelRumble
//
// History:     6/25/2003 + Created -- Cary Brisebois
//
//=============================================================================

//========================================
// System Includes
//========================================
// Foundation Tech
#include <raddebug.hpp>


//========================================
// Project Includes
//========================================
#include <input/wheelrumble.h>
#include <input/wheeldefines.h>
#include <input/inputmanager.h>

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

//=============================================================================
// WheelRumble::WheelRumble
//=============================================================================
// Description: Constructor.
//
// Parameters:  None.
//
// Return:      N/A.
//
//=============================================================================
WheelRumble::WheelRumble()
{
#ifdef WIN32
    m_diPeriodic.dwMagnitude             = 0;
    m_diPeriodic.lOffset                 = 0;
    m_diPeriodic.dwPhase                 = 0;
    m_diPeriodic.dwPeriod                = 80;

    m_diEnvelope.dwSize                  = sizeof(DIENVELOPE);
    m_diEnvelope.dwAttackLevel           = 0;
    m_diEnvelope.dwAttackTime            = (DWORD)(0.5 * DI_SECONDS);
    m_diEnvelope.dwFadeLevel             = 0;
    m_diEnvelope.dwFadeTime              = (DWORD)(1 * DI_SECONDS);

    mForceEffect.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    mForceEffect.dwDuration              = INFINITE;
    mForceEffect.dwSamplePeriod          = 0;
    mForceEffect.dwGain                  = DI_FFNOMINALMAX;
    mForceEffect.dwTriggerButton         = DIEB_NOTRIGGER;
    mForceEffect.dwTriggerRepeatInterval = 0;
    mForceEffect.cAxes                   = 1;
    mForceEffect.rgdwAxes                = m_rgdwAxes;
    mForceEffect.rglDirection            = m_rglDirection;
    mForceEffect.lpEnvelope              = &m_diEnvelope;
    mForceEffect.cbTypeSpecificParams    = sizeof(DIPERIODIC);
    mForceEffect.lpvTypeSpecificParams   = &m_diPeriodic;
    mForceEffect.dwStartDelay            = 0;
#else
    mForceEffect.type                          = SDL_HAPTIC_TRIANGLE;
    mForceEffect.periodic.direction            = SDL_HapticDirection{SDL_HAPTIC_POLAR, {90}};
    mForceEffect.periodic.length               = 500;
    mForceEffect.periodic.delay                = 0;

    mForceEffect.periodic.button               = 0;
    mForceEffect.periodic.interval             = 0;

    mForceEffect.periodic.period               = 80;
    mForceEffect.periodic.magnitude            = 0;
    mForceEffect.periodic.offset               = 0;
    mForceEffect.periodic.phase                = 0;

    mForceEffect.periodic.attack_length = 0;
    mForceEffect.periodic.attack_level  = 0;
    mForceEffect.periodic.fade_length   = 0;
    mForceEffect.periodic.fade_level    = 0;
#endif
}

//=============================================================================
// WheelRumble::~WheelRumble
//=============================================================================
// Description: Destructor.
//
// Parameters:  None.
//
// Return:      N/A.
//
//=============================================================================
WheelRumble::~WheelRumble()
{
}

//=============================================================================
// WheelRumble::OnInit
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void WheelRumble::OnInit()
{
#ifdef WIN32
    m_diPeriodic.dwMagnitude = 0;
#else
    mForceEffect.periodic.magnitude = 0;
#endif
}

//=============================================================================
// WheelRumble::SetMagDir
//=============================================================================
// Description: Comment
//
// Parameters:  ( u8 mag, u16 dir )
//
// Return:      void 
//
//=============================================================================
void WheelRumble::SetMagDir( u16 mag, u16 dir )
{
#ifdef WIN32
    LONG rglDirection[2]      = { dir, 0 };
    m_diPeriodic.dwMagnitude = mag;
    mForceEffect.rglDirection = rglDirection;
#else
    mForceEffect.periodic.magnitude = mag;
    mForceEffect.periodic.direction = SDL_HapticDirection{SDL_HAPTIC_POLAR, {dir}};
#endif

    mEffectDirty = true;
}

//=============================================================================
// WheelRumble::SetPPO
//=============================================================================
// Description: Comment
//
// Parameters:  ( u16 per, u16 phas, s16 offset )
//
// Return:      void 
//
//=============================================================================
void WheelRumble::SetPPO( u16 per, u16 phas, s16 offset )
{
#ifdef WIN32
    m_diPeriodic.dwPeriod                = per;
    m_diPeriodic.dwPhase                 = phas;
    m_diPeriodic.lOffset                 = offset;
#else
    mForceEffect.periodic.period = per;
    mForceEffect.periodic.phase = phas;
    mForceEffect.periodic.offset = offset;
#endif

    mEffectDirty = true;
}

//=============================================================================
// WheelRumble::SetRumbleType
//=============================================================================
// Description: Comment
//
// Parameters:  ( u8 type )
//
// Return:      void 
//
//=============================================================================
void WheelRumble::SetRumbleType( u8 type ) 
{
#ifdef WIN32
    
#else
    mForceEffect.type = type;
#endif
     
};

#ifdef WIN32
void WheelRumble::Update(unsigned timeins)
{
    m_currentTime += timeins;
    if( m_currentTime > m_effectTime )
    {
        SetMagDir(0,0);
        m_currentTime = 0;
    }
    ForceEffect::Update();
}
#endif

//*****************************************************************************
//
// Private Member Functions
//
//*****************************************************************************
