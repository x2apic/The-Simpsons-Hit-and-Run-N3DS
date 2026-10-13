//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        BaseDamper.cpp
//
// Description: Implement BaseDamper
//
// History:     10/21/2002 + Created -- Cary Brisebois
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
#include <input/basedamper.h>
#include <input/wheeldefines.h>

//******************************************************************************
//
// Global Data, Local Data, Local Classes
//
//******************************************************************************

//******************************************************************************
//
// Public Member Functions
//
//******************************************************************************

//==============================================================================
// BaseDamper::BaseDamper
//==============================================================================
// Description: Constructor.
//
// Parameters:	None.
//
// Return:      N/A.
//
//==============================================================================
BaseDamper::BaseDamper()
{
    //Setup the respective force effect structures.
#ifdef WIN32
    m_conditon.lOffset              = 0;
    m_conditon.lDeadBand            = 0;
    m_conditon.lPositiveCoefficient = 127;
    m_conditon.lNegativeCoefficient = 127; 
    m_conditon.dwPositiveSaturation = 127;
    m_conditon.dwNegativeSaturation = 127;

    mForceEffect.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    mForceEffect.dwDuration              = INFINITE;
    mForceEffect.dwGain                  = DI_FFNOMINALMAX;
    mForceEffect.dwTriggerButton         = DIEB_NOTRIGGER;
    mForceEffect.cAxes                   = 1;
    mForceEffect.rgdwAxes                = m_rgdwAxes;
    mForceEffect.rglDirection            = m_rglDirection;
    mForceEffect.lpEnvelope              = NULL;
    mForceEffect.cbTypeSpecificParams    = sizeof(DICONDITION);
    mForceEffect.lpvTypeSpecificParams   = &m_conditon;
    mForceEffect.dwStartDelay            = 0;
#else
    mForceEffect.type                = SDL_HAPTIC_DAMPER;
    mForceEffect.condition.direction = SDL_HapticDirection{};
    mForceEffect.condition.length    = SDL_HAPTIC_INFINITY;
    mForceEffect.condition.delay     = 0;
    mForceEffect.condition.button    = 0;
    mForceEffect.condition.interval  = 0;
    SetCenterPoint(0, 0);
    SetDamperStrength(127);
    SetDamperCoefficient(127);
#endif
}

//==============================================================================
// BaseDamper::~BaseDamper
//==============================================================================
// Description: Destructor.
//
// Parameters:	None.
//
// Return:      N/A.
//
//==============================================================================
BaseDamper::~BaseDamper()
{
}

//=============================================================================
// BaseDamper::OnInit
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      void 
//
//=============================================================================
void BaseDamper::OnInit()
{
}

//=============================================================================
// BaseDamper::SetCenterPoint
//=============================================================================
// Description: Comment
//
// Parameters:  ( s8 point, u8 deadband  )
//
// Return:      void 
//
//=============================================================================
void BaseDamper::SetCenterPoint( s8 point, u8 deadband  )
{
#ifdef WIN32
    m_conditon.lOffset                            = point;
    m_conditon.lDeadBand                          = deadband;
#else
    mForceEffect.condition.center[0]   = point;
    mForceEffect.condition.center[1]   = point;
    mForceEffect.condition.center[2]   = point;
    mForceEffect.condition.deadband[0] = deadband;
    mForceEffect.condition.deadband[1] = deadband;
    mForceEffect.condition.deadband[2] = deadband;
#endif

    mEffectDirty = true;
}

//=============================================================================
// BaseDamper::SetDamperStrength
//=============================================================================
// Description: Comment
//
// Parameters:  ( u8 strength )
//
// Return:      void 
//
//=============================================================================
void BaseDamper::SetDamperStrength( u16 strength )
{
#ifdef WIN32
    m_conditon.dwPositiveSaturation               = strength;
    m_conditon.dwNegativeSaturation               = strength;
#else
    mForceEffect.condition.right_sat[0] = strength;
    mForceEffect.condition.right_sat[1] = strength;
    mForceEffect.condition.right_sat[2] = strength;
    mForceEffect.condition.left_sat[0] = strength;
    mForceEffect.condition.left_sat[1] = strength;
    mForceEffect.condition.left_sat[2] = strength;
#endif
    mEffectDirty = true;
}

//=============================================================================
// BaseDamper::SetDamperCoefficient
//=============================================================================
// Description: Comment
//
// Parameters:  ( s16 coeff )
//
// Return:      void 
//
//=============================================================================
void BaseDamper::SetDamperCoefficient( s16 coeff )
{
#ifdef WIN32
    m_conditon.lPositiveCoefficient               = coeff;
    m_conditon.lNegativeCoefficient               = coeff; 
#else
    mForceEffect.condition.right_coeff[0] = coeff;
    mForceEffect.condition.right_coeff[1] = coeff;
    mForceEffect.condition.right_coeff[2] = coeff;
    mForceEffect.condition.left_coeff[0] = coeff;
    mForceEffect.condition.left_coeff[1] = coeff;
    mForceEffect.condition.left_coeff[2] = coeff;
#endif

    mEffectDirty = true;
}

//******************************************************************************
//
// Private Member Functions
//
//******************************************************************************
