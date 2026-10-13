//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        gcncontroller.cpp
//
// Subsystem:	Foundation Technologies - Controller System
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies GameCube Nintendo controller
//
// Date:    	Modified by Cary Brisebois to support GBA, Keyboard, Wavebird
//              and Wheels.
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <radobject.hpp>
#include <radcontroller.hpp>
#include <raddebug.hpp>
#include <radstring.hpp>
#include <radobjectlist.hpp>
#include <radtime.hpp>
#include <radmemorymonitor.hpp>
#include "radcontrollerbuffer.hpp"

#include <dolphin/os.h>
#include <dolphin/kbd.h>
#include <dolphin/pad.h>
#include <dolphin/lg.h>
#include <dolphin.h>

//============================================================================
// Globals
//============================================================================

struct GcnInputPoint
{
    char * m_pType;
    char * m_pName;
    unsigned int m_Mask;
};

static char * g_Gcnipt[] =
{
    "Button",       // 0
    "XAxis",        // 1
    "YAxis",        // 2
    "AnalogButton" // 3
};

#define PAD_LEFT_STICK_X    0
#define PAD_LEFT_STICK_Y    1
#define PAD_RIGHT_STICK_X   2
#define PAD_RIGHT_STICK_Y   3
#define PAD_LEFT_TRIGGER    4
#define PAD_RIGHT_TRIGGER   5

static GcnInputPoint g_GcnPoints[] =
{
    { g_Gcnipt[ 0 ], "DPadLeft",        PAD_BUTTON_LEFT },
    { g_Gcnipt[ 0 ], "DPadRight",       PAD_BUTTON_RIGHT },
    { g_Gcnipt[ 0 ], "DPadDown",        PAD_BUTTON_DOWN },
    { g_Gcnipt[ 0 ], "DPadUp",          PAD_BUTTON_UP },
    { g_Gcnipt[ 0 ], "TriggerZ",        PAD_TRIGGER_Z },
    { g_Gcnipt[ 0 ], "TriggerR",        PAD_TRIGGER_R },
    { g_Gcnipt[ 0 ], "TriggerL",        PAD_TRIGGER_L },
    { g_Gcnipt[ 0 ], "A",               PAD_BUTTON_A },
    { g_Gcnipt[ 0 ], "B",               PAD_BUTTON_B },
    { g_Gcnipt[ 0 ], "X",               PAD_BUTTON_X },
    { g_Gcnipt[ 0 ], "Y",               PAD_BUTTON_Y },
    { g_Gcnipt[ 0 ], "Menu",            PAD_BUTTON_MENU },
    { g_Gcnipt[ 1 ], "LeftStickX",      PAD_LEFT_STICK_X },
    { g_Gcnipt[ 2 ], "LeftStickY",      PAD_LEFT_STICK_Y },
    { g_Gcnipt[ 1 ], "RightStickX",     PAD_RIGHT_STICK_X },
    { g_Gcnipt[ 2 ], "RightStickY",     PAD_RIGHT_STICK_Y },
    { g_Gcnipt[ 3 ], "AnalogTriggerL",  PAD_LEFT_TRIGGER },
    { g_Gcnipt[ 3 ], "AnalogTriggerR",  PAD_RIGHT_TRIGGER }
//    { g_Gcnipt[ 3 ], "AnalogA",         sizeof ( u16 ) + sizeof( u8 ) + 6 },  //No longer exist
//    { g_Gcnipt[ 3 ], "AnalogB",         sizeof ( u16 ) + sizeof( u8 ) + 7 }   //No longer exist
};

//KBD_UP                  
//KBD_DOWN                
//KBD_SCAN_MASK           
//KBD_SCAN_NULL           
// Alias
//#define KBD_SCAN_ALT_GR         KBD_SCAN_KANA       // EU only
//#define KBD_SCAN_RCONTROL       KBD_SCAN_KANA       // US only
//#define KBD_SCAN_EU_MISC        KBD_SCAN_KANA_MU    // EU only


static GcnInputPoint g_GcnKbdPoints[] = 
{
    { g_Gcnipt[ 0 ], "Reserved",    0x01 },
    { g_Gcnipt[ 0 ], "Reserved",    0x02 },
    { g_Gcnipt[ 0 ], "Reserved",    0x03 },
    { g_Gcnipt[ 0 ], "Reserved",    0x04 },
    { g_Gcnipt[ 0 ], "Fn",          KBD_SCAN_FN },
    { g_Gcnipt[ 0 ], "Home",        KBD_SCAN_HOME },
    { g_Gcnipt[ 0 ], "End",         KBD_SCAN_END },
    { g_Gcnipt[ 0 ], "PgUp",        KBD_SCAN_PAGE_UP },
    { g_Gcnipt[ 0 ], "PgDn",        KBD_SCAN_PAGE_DOWN },
    { g_Gcnipt[ 0 ], "ScrLk",       KBD_SCAN_SCROLL_LOCK },
    { g_Gcnipt[ 0 ], "Reserved",    0x0B },
    { g_Gcnipt[ 0 ], "Reserved",    0x0C },
    { g_Gcnipt[ 0 ], "Reserved",    0x0D },
    { g_Gcnipt[ 0 ], "Reserved",    0x0E },
    { g_Gcnipt[ 0 ], "Reserved",    0x0F },
    { g_Gcnipt[ 0 ], "A",           KBD_SCAN_A },
    { g_Gcnipt[ 0 ], "B",           KBD_SCAN_B },
    { g_Gcnipt[ 0 ], "C",           KBD_SCAN_C },
    { g_Gcnipt[ 0 ], "D",           KBD_SCAN_D },
    { g_Gcnipt[ 0 ], "E",           KBD_SCAN_E },
    { g_Gcnipt[ 0 ], "F",           KBD_SCAN_F },
    { g_Gcnipt[ 0 ], "G",           KBD_SCAN_G },
    { g_Gcnipt[ 0 ], "H",           KBD_SCAN_H },
    { g_Gcnipt[ 0 ], "I",           KBD_SCAN_I },
    { g_Gcnipt[ 0 ], "J",           KBD_SCAN_J },
    { g_Gcnipt[ 0 ], "K",           KBD_SCAN_K },
    { g_Gcnipt[ 0 ], "L",           KBD_SCAN_L },
    { g_Gcnipt[ 0 ], "M",           KBD_SCAN_M },
    { g_Gcnipt[ 0 ], "N",           KBD_SCAN_N },
    { g_Gcnipt[ 0 ], "O",           KBD_SCAN_O },
    { g_Gcnipt[ 0 ], "P",           KBD_SCAN_P },
    { g_Gcnipt[ 0 ], "Q",           KBD_SCAN_Q },
    { g_Gcnipt[ 0 ], "R",           KBD_SCAN_R },
    { g_Gcnipt[ 0 ], "S",           KBD_SCAN_S },
    { g_Gcnipt[ 0 ], "T",           KBD_SCAN_T },
    { g_Gcnipt[ 0 ], "U",           KBD_SCAN_U },
    { g_Gcnipt[ 0 ], "V",           KBD_SCAN_V },
    { g_Gcnipt[ 0 ], "W",           KBD_SCAN_W },
    { g_Gcnipt[ 0 ], "X",           KBD_SCAN_X },
    { g_Gcnipt[ 0 ], "Y",           KBD_SCAN_Y },
    { g_Gcnipt[ 0 ], "Z",           KBD_SCAN_Z },
    { g_Gcnipt[ 0 ], "1",           KBD_SCAN_1 },
    { g_Gcnipt[ 0 ], "2",           KBD_SCAN_2 },
    { g_Gcnipt[ 0 ], "3",           KBD_SCAN_3 },
    { g_Gcnipt[ 0 ], "4",           KBD_SCAN_4 },
    { g_Gcnipt[ 0 ], "5",           KBD_SCAN_5 },
    { g_Gcnipt[ 0 ], "6",           KBD_SCAN_6 },
    { g_Gcnipt[ 0 ], "7",           KBD_SCAN_7 },
    { g_Gcnipt[ 0 ], "8",           KBD_SCAN_8 },
    { g_Gcnipt[ 0 ], "9",           KBD_SCAN_9 },
    { g_Gcnipt[ 0 ], "-",           KBD_SCAN_MINUS },
    { g_Gcnipt[ 0 ], "=",           KBD_SCAN_EQUALS },
    { g_Gcnipt[ 0 ], "¥",           KBD_SCAN_YEN }, //JP only
    { g_Gcnipt[ 0 ], "[",           KBD_SCAN_OPEN_BLACKET },
    { g_Gcnipt[ 0 ], "]",           KBD_SCAN_CLOSE_BLACKET },
    { g_Gcnipt[ 0 ], ";",           KBD_SCAN_SEMICOLON },
    { g_Gcnipt[ 0 ], "'",           KBD_SCAN_QUOTE },
    { g_Gcnipt[ 0 ], "?",           KBD_SCAN_KANA_MU },  //JP only
    { g_Gcnipt[ 0 ], ",",           KBD_SCAN_COMMA },
    { g_Gcnipt[ 0 ], ".",           KBD_SCAN_PERIOD },
    { g_Gcnipt[ 0 ], "/",           KBD_SCAN_SLASH },
    { g_Gcnipt[ 0 ], "?",           KBD_SCAN_KANA_RO },
    { g_Gcnipt[ 0 ], "F1",          KBD_SCAN_F1 },
    { g_Gcnipt[ 0 ], "F2",          KBD_SCAN_F2 },
    { g_Gcnipt[ 0 ], "F3",          KBD_SCAN_F3 },
    { g_Gcnipt[ 0 ], "F4",          KBD_SCAN_F4 },
    { g_Gcnipt[ 0 ], "F5",          KBD_SCAN_F5 },
    { g_Gcnipt[ 0 ], "F6",          KBD_SCAN_F6 },
    { g_Gcnipt[ 0 ], "F7",          KBD_SCAN_F7 },
    { g_Gcnipt[ 0 ], "F8",          KBD_SCAN_F8 },
    { g_Gcnipt[ 0 ], "F9",          KBD_SCAN_F9 },
    { g_Gcnipt[ 0 ], "F10",         KBD_SCAN_F10 },
    { g_Gcnipt[ 0 ], "F11",         KBD_SCAN_F11 },
    { g_Gcnipt[ 0 ], "F12",         KBD_SCAN_F12 },
    { g_Gcnipt[ 0 ], "Esc",         KBD_SCAN_ESCAPE },
    { g_Gcnipt[ 0 ], "Insert",      KBD_SCAN_INSERT },
    { g_Gcnipt[ 0 ], "Delete",      KBD_SCAN_DELETE },
    { g_Gcnipt[ 0 ], "`",           KBD_SCAN_BACK_QUOTE },
    { g_Gcnipt[ 0 ], "Backspace",   KBD_SCAN_BACK_SPACE },
    { g_Gcnipt[ 0 ], "Tab",         KBD_SCAN_TAB },
    { g_Gcnipt[ 0 ], "\\",          KBD_SCAN_BACKSLASH },
    { g_Gcnipt[ 0 ], "CapsLock",    KBD_SCAN_CAPS_LOCK },
    { g_Gcnipt[ 0 ], "LShift",      KBD_SCAN_LSHIFT },
    { g_Gcnipt[ 0 ], "RShift",      KBD_SCAN_RSHIFT },
    { g_Gcnipt[ 0 ], "Ctrl",        KBD_SCAN_CONTROL },
    { g_Gcnipt[ 0 ], "Alt",         KBD_SCAN_ALT },
    { g_Gcnipt[ 0 ], "?",           KBD_SCAN_NONCONVERT },  //JP only
    { g_Gcnipt[ 0 ], " ",           KBD_SCAN_SPACE },
    { g_Gcnipt[ 0 ], "?",           KBD_SCAN_CONVERT }, //JP only
    { g_Gcnipt[ 0 ], "KANA",        KBD_SCAN_KANA },  //This one is special.
    { g_Gcnipt[ 0 ], "Left",        KBD_SCAN_LEFT },
    { g_Gcnipt[ 0 ], "Down",        KBD_SCAN_DOWN },
    { g_Gcnipt[ 0 ], "Down",        KBD_SCAN_DOWN },
    { g_Gcnipt[ 0 ], "Up",          KBD_SCAN_UP },
    { g_Gcnipt[ 0 ], "Right",       KBD_SCAN_RIGHT },
    { g_Gcnipt[ 0 ], ">",           KBD_SCAN_ANGLE_BRACKETS },  //Not US
    { g_Gcnipt[ 0 ], "Enter",       KBD_SCAN_ENTER }
};

#define LG_WHEEL            0
#define LG_GAS              1
#define LG_BRAKE            2
#define LG_COMBINED         3
#define LG_LEFT_TRIGGER     4
#define LG_RIGHT_TRIGGER    5

static GcnInputPoint g_LGPoints[] =
{
    { g_Gcnipt[ 0 ], "DPadLeft",        LG_BUTTON_LEFT },
    { g_Gcnipt[ 0 ], "DPadRight",       LG_BUTTON_RIGHT },
    { g_Gcnipt[ 0 ], "DPadDown",        LG_BUTTON_DOWN },
    { g_Gcnipt[ 0 ], "DPadUp",          LG_BUTTON_UP },
    { g_Gcnipt[ 0 ], "TriggerZ",        LG_TRIGGER_Z },
    { g_Gcnipt[ 0 ], "TriggerR",        LG_TRIGGER_R },
    { g_Gcnipt[ 0 ], "TriggerL",        LG_TRIGGER_L },
    { g_Gcnipt[ 0 ], "A",               LG_BUTTON_A },
    { g_Gcnipt[ 0 ], "B",               LG_BUTTON_B },
    { g_Gcnipt[ 0 ], "X",               LG_BUTTON_X },
    { g_Gcnipt[ 0 ], "Y",               LG_BUTTON_Y },
    { g_Gcnipt[ 0 ], "Menu",            LG_BUTTON_START },
    { g_Gcnipt[ 2 ], "Wheel",           LG_WHEEL },
    { g_Gcnipt[ 3 ], "Gas",             LG_GAS },
    { g_Gcnipt[ 3 ], "Brake",           LG_BRAKE },
    { g_Gcnipt[ 1 ], "Combined",        LG_COMBINED },
    { g_Gcnipt[ 3 ], "AnalogTriggerL",  LG_LEFT_TRIGGER },
    { g_Gcnipt[ 3 ], "AnalogTriggerR",  LG_RIGHT_TRIGGER }
};

enum InputType
{
    CONTROLLER,
    KEYBOARD,
    STEERING_WHEEL
};

struct ControllerBufferPacket
{
	unsigned int m_time;
    u8 m_kbd[KBD_READ_MAX];
	PADStatus m_pad;
    LGPosition m_steering;
    u8 m_InputType;  //The PadStatus struct has a spare u8 leftover.
};

struct KbdStatus
{
    u8 m_scan[KBD_READ_MAX];
    s32 m_err;
};

PADStatus g_PadStatus[ SI_MAX_CHAN ];
KbdStatus g_KbdStatus[ SI_MAX_CHAN ];
LGPosition g_LGStatus[ SI_MAX_CHAN ];

u32 g_WheelHandles[SI_MAX_CHAN] = {LG_INVALID_DEVICE, LG_INVALID_DEVICE, LG_INVALID_DEVICE, LG_INVALID_DEVICE};

                
static class radControllerSystemGcn* g_pTheCS = NULL;

static radMemoryAllocator g_CtlrAllocator = RADMEMORY_ALLOC_DEFAULT;
#define PAD_STICK_MAX_RADIUS 56.0f
#define PAD_SUB_STICK_MAX_RADIUS 44.0f

bool g_isLGInitialized = false;

//============================================================================
// Internal Interfaces
//============================================================================
typedef int PADHandle;

struct IRadControllerInputPointGcn
    :
    public IRadControllerInputPoint
{
	virtual void iInitialize( PADStatus* pData ) = 0;
	virtual void iInitialize( KbdStatus* pData ) = 0;
	virtual void iInitialize( LGPosition* pData ) = 0;
    virtual void iVirtualTimeReMapped( unsigned int virtualTime ) = 0;
    virtual void iVirtualTimeChanged( unsigned int virtualTime, PADStatus* pData ) = 0;
    virtual void iVirtualTimeChanged( unsigned int virtualTime, KbdStatus* pData ) = 0;
    virtual void iVirtualTimeChanged( unsigned int virtualTime, LGPosition* pData ) = 0;
};

struct IRadControllerGcn
    :
    public IRadController
{
    virtual void iPoll( unsigned int virtualTime ) = 0;
    virtual void iVirtualTimeReMapped( unsigned int virtualTime ) = 0;
    virtual void iVirtualTimeChanged( unsigned int virtualTime ) = 0;
    virtual void iSetBufferTime( unsigned int milliseconds, unsigned int pollingRate ) = 0;
	virtual void iSetConnected( PADHandle hController ) = 0;
};

//============================================================================
// Component: radControllerOutputPointGcn
//============================================================================
class radControllerOutputPointGcn
    :
    public IRadControllerOutputPoint,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerOutputPointGcn" )

    //========================================================================
    // radControllerOutputPointGcn::radControllerOutputPointGcn
    //========================================================================
    radControllerOutputPointGcn( const char * pName )
        :
        radRefCount( 0 ),
        m_pName( pName ),
        m_Gain( 0.0f )
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerOutputPointGcn" );
    }

    //========================================================================
    // radControllerOutputPointGcn::~radControllerOutputPointGcn
    //========================================================================
    ~radControllerOutputPointGcn( void )
    {
    }

    //========================================================================
    // radControllerOutputPointGcn::GetName
    //========================================================================
    virtual const char * GetName( void )
    {
        return m_pName;
    }

    //========================================================================
    // radControllerOutputPointGcn::GetType
    //========================================================================
    virtual const char * GetType( void )
    {
        return "Analog";
    }

    //========================================================================
    // radControllerOutputPointGcn::GetGain
    //========================================================================
    virtual float GetGain( void )
    {
        return m_Gain;
    }

    //========================================================================
    // radControllerOutputPointGcn::SetGain
    //========================================================================
    virtual void SetGain( float value )
    {
        if ( value < 0.0f )
        {
            value = 0.0f;
        }
        else if ( value > 1.0f )
        {
            value = 1.0f;
        }

        m_Gain = value;
    }

    //========================================================================
    // radControllerOutputPointGcn Data Members
    //========================================================================
    const char * m_pName;
    float m_Gain;
};

class radControllerOutputPointGcnLG 
    :
    public IRadControllerOutputPoint,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerOutputPointGcnLG" )

    enum Command { IDLE, START, STOP };

    //========================================================================
    // radControllerOutputPointGcnLG::radControllerOutputPointGcnLG
    //========================================================================
    radControllerOutputPointGcnLG( const char * pName, u8 type )
        :
        radRefCount( 0 ),
        m_pName( pName ),
        m_uEffectID( -1 ),
        m_bDirty( false ),
        m_eCommand( IDLE )
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerOutputPointGcnLG" );

        m_Effect.type = type;
    }

    //========================================================================
    // radControllerOutputPointGcnLG::~radControllerOutputPointGcnLG
    //========================================================================
    ~radControllerOutputPointGcnLG( void )
    {
    }

    //========================================================================
    // radControllerOutputPointGcnLG::GetName
    //========================================================================
    const char * GetName( void )
    {
        return m_pName;
    }

    //========================================================================
    // radControllerOutputPointGcnLG::GetType
    //========================================================================
    const char * GetType( void )
    {
        return "ForceEffect";
    }
    //========================================================================
    // radControllerOutputPointGcnLG::GetGain
    //========================================================================
    float GetGain( void )
    {
        rAssert( false );
        return 0;
    }

    //========================================================================
    // radControllerOutputPointGcnLG::SetGain
    //========================================================================
    void SetGain( float value )
    {
        rAssert( false );
    }

    //========================================================================
    // radControllerOutputPointGcnLG::GetEffect
    //========================================================================
    const LGForceEffect* GetEffect() const
    {
        return &m_Effect;
    }

    //========================================================================
    // radControllerOutputPointGcnLG::UpdateEffect
    //========================================================================
    void UpdateEffect( const LGForceEffect* effect )
    {
        rAssert( effect->type == m_Effect.type );
        
        m_Effect.duration = effect->duration;
        m_Effect.startDelay = effect->startDelay;
        m_Effect.p = effect->p;

        m_bDirty = true;
    }

    //========================================================================
    // radControllerOutputPointGcnLG::Start
    //========================================================================
    void Start()
    {
        m_eCommand = START;
    }

    //========================================================================
    // radControllerOutputPointGcnLG::Stop
    //========================================================================
    void Stop() 
    {
        m_eCommand = STOP;
    }

    //========================================================================
    // radControllerOutputPointGcnLG::Update
    //========================================================================
    void Update( u32 device )
    {
        if ( m_uEffectID == -1 && m_bDirty )
        {
            //Let's download the effect to the wheel
            LGDownloadForceEffect( device, (unsigned long*)(&m_uEffectID), &m_Effect );
            m_bDirty = false;
        } 
        else if ( m_bDirty )
        {
            LGStartForceEffect( m_uEffectID );  //This will reset the effect.
            LGUpdateForceEffect( m_uEffectID, &m_Effect );
            m_bDirty = false;
        }

        switch ( m_eCommand )
        {
        case START:
            {
                LGStartForceEffect( m_uEffectID );
                break;
            }
        case STOP:
            {
                LGStopForceEffect( m_uEffectID );
                break;
            }
        default:
            {
                //Must be idle
                break;
            }
        }

        m_eCommand = IDLE;
    }

    //========================================================================
    // radControllerOutputPointGcnLG Data Members
    //========================================================================
    const char * m_pName;
    int m_uEffectID;

    LGForceEffect m_Effect;
    bool m_bDirty;
    
    Command m_eCommand;
};


//**********************************************//
//												//
// Component: radControllerInputPointGcn		//
//												//
//**********************************************//
class radControllerInputPointGcn
    :
    public IRadControllerInputPointGcn,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerInputPointGcn" )

	//========================================================================
    // radControllerInputPointGcn::iIVirtualTimeReMapped
    //========================================================================
    virtual void iVirtualTimeReMapped( unsigned int virtualTime )
    {
        //
        // The client has done a re-sync to game time, all we can do is
        // set our changed-state time to "now".
        //
        m_TimeInState = 0;
        m_TimeOfStateChange = virtualTime;
	}

	//========================================================================
    // radControllerInputPointGcn::CalculateNewValue
    //========================================================================
	float CalculateNewValue( PADStatus * pData )
	{
        //
        // Calculate the current value of the input point according to the
        // data structure passed in.  We get initialized with the offset
        // into this data array to get our data.  Knowing this and our type
        // we can determine our new floating point value.
        //

        float newValue = 0.0f;

       	if ( pData != NULL )
        {
            if ( m_pType == g_Gcnipt[ 0 ] ) // Button
            {
				newValue = ( (unsigned int)(pData->button) & m_Offset) ? 1.0f : 0.0f;
				//sprintf( outbuf, "button name = [%s], button value = [%f]\n", GetName(), newValue);
			}

			else if ( (m_pType == g_Gcnipt[ 1 ]) || (m_pType == g_Gcnipt[ 2 ]) ) // Stick or SubStick
			{
				s8 sShort = 0;
				
				switch(m_Offset)
				{
				case PAD_LEFT_STICK_X:
//					sShort = pData->stickY;
					newValue = ( ( (float)pData->stickX / PAD_STICK_MAX_RADIUS ) + 1.0f ) / 2.0f;
					break;
				
				case PAD_LEFT_STICK_Y:
//					sShort = pData->stickY;
					newValue = ( ( (float)pData->stickY / PAD_STICK_MAX_RADIUS ) + 1.0f ) / 2.0f;
					break;

				case PAD_RIGHT_STICK_X:
//					sShort = pData->substickX;
					newValue = ( ( (float)pData->substickX / PAD_SUB_STICK_MAX_RADIUS ) + 1.0f ) / 2.0f;
					break;

				case PAD_RIGHT_STICK_Y:
//					sShort = pData->substickY;
					newValue = ( ( (float)pData->substickY / PAD_SUB_STICK_MAX_RADIUS ) + 1.0f ) / 2.0f;
					break;	
					
				default:
					rAssert("Should not get here\n");
					break;
				}
					
				//Since these values are getting clamped via the PADClampCircle function, they range from
				//-PAD_SUB_STICK_MAX_RADIUS to PAD_SUB_STICK_MAX_RADIUS
				
//				newValue = ( (float)sShort + 74.0f ) / ( 74.0f * 2.0f ) ;	// -128 --> 127		

				//sprintf( outbuf, "button name = [%s], button value = [%f]\n", GetName(), newValue);
			}
            else if ( m_pType == g_Gcnipt[ 3 ] ) // Trigger Button
            {
                u8 uShort = 0;
                if (m_Offset == PAD_LEFT_TRIGGER)
                {
                	uShort = pData->triggerLeft;
                }
                else if (m_Offset == PAD_RIGHT_TRIGGER)
                {
                	uShort = pData->triggerRight;
                } 
               
               	newValue = (float)uShort;
				newValue /= 150.0;
				//sprintf( outbuf, "button name = [%s], button value = [%f]\n", GetName(), newValue);
            }
            else
            {
                rAssert( 0 );
            }
            
            // rDebugString( outbuf );
		}

		return newValue;
	}

	//========================================================================
    // radControllerInputPointGcn::CalculateNewValue
    //========================================================================
	float CalculateNewValue( KbdStatus* pData )
	{
        //
        // Calculate the current value of the input point according to the
        // data structure passed in.  We get initialized with the offset
        // into this data array to get our data.  Knowing this and our type
        // we can determine our new floating point value.
        //

        float newValue = 0.0f;

       	if ( pData != NULL )
        {
            if ( m_pType == g_Gcnipt[ 0 ] ) // Button
            {
                unsigned int i;
                for ( i = 0; i < KBD_READ_MAX; ++i )
                {
                    u8 code = pData->m_scan[i];

                    if ( code  & KBD_SCAN_MASK == m_Offset )
                    {
                        newValue = (code & KBD_DOWN) ? 1.0f : 0.0f;
                    }
                }
			}
            else
            {
                rAssert( 0 );
            }
            
            // rDebugString( outbuf );
		}

		return newValue;
	}

	//========================================================================
    // radControllerInputPointGcn::CalculateNewValue
    //========================================================================
	float CalculateNewValue( LGPosition * pData )
	{
        //
        // Calculate the current value of the input point according to the
        // data structure passed in.  We get initialized with the offset
        // into this data array to get our data.  Knowing this and our type
        // we can determine our new floating point value.
        //

        float newValue = 0.0f;

       	if ( pData != NULL )
        {
            if ( m_pType == g_Gcnipt[ 0 ] ) // Button
            {
				newValue = ( (unsigned int)(pData->button) & m_Offset) ? 1.0f : 0.0f;
				//sprintf( outbuf, "button name = [%s], button value = [%f]\n", GetName(), newValue);
			}

			else if ( (m_pType == g_Gcnipt[ 1 ]) || (m_pType == g_Gcnipt[ 2 ]) ) //Wheel or combined brake/gas
			{
				s8 sShort = 0;
				
				switch(m_Offset)
				{
				case LG_WHEEL:
					sShort = pData->wheel;
					break;
				
				case LG_COMBINED:
					sShort = pData->combined;
					break;	
					
				default:
					rAssert("Should not get here\n");
					break;
				}
					
				newValue = ((float)sShort + 128.0f) / 255.0f;	// -128 --> 127				

				//sprintf( outbuf, "button name = [%s], button value = [%f]\n", GetName(), newValue);
			}
            else if ( m_pType == g_Gcnipt[ 3 ] ) // Trigger Button
            {
                u8 uShort = 0;
                switch (m_Offset)
                {
                case LG_GAS:
                    uShort = pData->accelerator;
                    break;
                case LG_BRAKE:
                    uShort = pData->brake;
                    break;
                case LG_LEFT_TRIGGER:
                    uShort = pData->triggerLeft;
                    break;
                case LG_RIGHT_TRIGGER:
                    uShort = pData->triggerRight;
                    break;
                }
               
               	newValue = static_cast<float>(uShort);
				newValue /= 255.0f;
				//sprintf( outbuf, "button name = [%s], button value = [%f]\n", GetName(), newValue);
            }
            else
            {
                rAssert( 0 );
            }
            
            // rDebugString( outbuf );
		}

		return newValue;
	}

    //========================================================================
    // radControllerInputPointGcn::iInitialize
    //========================================================================
	virtual void iInitialize( PADStatus* pData )
	{
		//
		// Take on an intial value
		//
		m_Value = CalculateNewValue( pData );
	}

	//========================================================================
    // radControllerInputPointGcn::iInitialize
    //========================================================================
	virtual void iInitialize( KbdStatus* pData )
	{
		//
		// Take on an intial value
		//
		m_Value = CalculateNewValue( pData );
	}

    //========================================================================
    // radControllerInputPointGcn::iInitialize
    //========================================================================
	virtual void iInitialize( LGPosition* pData )
	{
		//
		// Take on an intial value
		//
		m_Value = CalculateNewValue( pData );
	}

    //========================================================================
    // radControllerInputPointGcn::VirtualTimeChanged
    //========================================================================
    void VirtualTimeChanged( unsigned int virtualTime, float newValue )
    {
        //
        // Check tolerance
        //

        if ( ( newValue != m_Value ) && ( fabs( newValue - m_Value ) >= m_Tolerance ) )
        {
            //
            // The input point has changed and we are in tolerance.
            //

            m_Value = newValue;

            m_TimeOfStateChange = virtualTime;
            m_TimeInState = 0; // Just changed

            //
            // Notify callbacks
            //

            AddRef( ); // Don't want to self destruct while we're calling out

            IRadWeakCallbackWrapper * pIWcr;

            m_xIOl_Callbacks->Reset( );

            if ( pIWcr = reinterpret_cast< IRadWeakCallbackWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
            {
                IRadControllerInputPointCallback * pCallback = ( IRadControllerInputPointCallback* ) pIWcr->GetWeakInterface( );
                unsigned int userData = (unsigned int) pIWcr->GetUserData( );

                pCallback->OnControllerInputPointChange( userData, m_Value );           
            }

            Release( );
        }
        else
        {
            //
            // This input point has not changed value, or failed the tolerance
            // test, so count time in state.
            //

            m_TimeInState = virtualTime - m_TimeOfStateChange;
        }
    }
    //========================================================================
    // radControllerInputPointGcn::iIVirtualTimeChanged
    //========================================================================
    virtual void iVirtualTimeChanged( unsigned int virtualTime, PADStatus* pData )
    {
        rAssert( pData != NULL );

        float newValue = m_Value;
        if ( pData->err == PAD_ERR_NONE )
        {
            newValue = CalculateNewValue( pData );
        }
	        
        VirtualTimeChanged( virtualTime, newValue );		
    }


    //========================================================================
    // radControllerInputPointGcn::iIVirtualTimeChanged
    //========================================================================
    virtual void iVirtualTimeChanged( unsigned int virtualTime, KbdStatus* pData )
    {
        rAssert( pData != NULL );

		float newValue = CalculateNewValue( pData );
	        
        VirtualTimeChanged( virtualTime, newValue );		
    }

    //========================================================================
    // radControllerInputPointGcn::iIVirtualTimeChanged
    //========================================================================
    virtual void iVirtualTimeChanged( unsigned int virtualTime, LGPosition* pData )
    {
        rAssert( pData != NULL );

		float newValue = CalculateNewValue( pData );
	        
        VirtualTimeChanged( virtualTime, newValue );		
    }

    //========================================================================
    // radControllerInputPointGcn::GetName
    //========================================================================
    virtual const char * GetName( void )
    {
        //
        // This points to a string in the global controller definition array
        // (see top of file)
        //

        return m_pName;
    }
    
    //========================================================================
    // radControllerInputPointGcn::GetType
    //========================================================================
    virtual const char * GetType( void )
    {
        //
        // This points to a string in the global controller definition array
        // (see top of file)
        //

        return m_pType;      
    }

    //========================================================================
    // radControllerInputPointGcn::SetTolerance
    //========================================================================
    virtual void  SetTolerance( float percentage )
    {
        //
        // Set tolerance in range.
        //

        rAssert( percentage >= 0.0f && percentage <= 1.0f );

        if ( percentage < 0.0f )
        {
            percentage = 0.0f;
        }
        else if ( percentage > 1.0f )
        {
            percentage = 1.0f;
        }

        m_Tolerance = percentage;
    }

    //========================================================================
    // radControllerInputPointGcn::GetTolerance
    //========================================================================
    virtual float GetTolerance( void )
    {
        //
        // Simply return the tolerance
        // 

        return m_Tolerance;
    }

    //========================================================================
    // radControllerInputPointGcn::RegisterControllerInputPointCallback
    //========================================================================
    virtual void RegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback,
        unsigned int userData = 0
    )
    {
        //
        // Wrap the weak interface we a callback wrapper so we can store it
        // in our object list.
        //

        rAssert( pCallback != NULL );

        ref< IRadWeakCallbackWrapper > xIWcr;

        radWeakCallbackWrapperCreate( &xIWcr, g_CtlrAllocator ); 

        rAssert( xIWcr != NULL );

        if ( xIWcr != NULL )
        {
            xIWcr->SetWeakInterface( pCallback );
            xIWcr->SetUserData( (void*) userData );
        }

        m_xIOl_Callbacks->AddObject( xIWcr );
    }

    //========================================================================
    // radControllerInputPointGcn::UnRegisterControllerInputPointCallback
    //========================================================================
    virtual void UnRegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback
    )
    {
        //
        // Simply look for the callback in the list and delete it, if found.
        //
        rAssert( pCallback != NULL );

        IRadWeakCallbackWrapper * pIWcr;

        m_xIOl_Callbacks->Reset( );

        if ( pIWcr = reinterpret_cast< IRadWeakCallbackWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
        {
            if ( pIWcr->GetWeakInterface( ) == pCallback )
            {
                m_xIOl_Callbacks->RemoveObject( pIWcr );
                return;
            }
        }

        rAssertMsg( false, "Controller Input Point Callback Not Registered." );
    }

    //========================================================================
    // radControllerInputPointGcn::GetCurrentValue
    //========================================================================
    virtual float GetCurrentValue( unsigned int * pTime = NULL )
    {
        if ( pTime != NULL )
        {
            *pTime = m_TimeInState;
        }                
        
        //
        // we map it to the client's range here, this solves a bunch of
        // problems with the client remapping in mid-game.
        //

        return (( m_MaxRange - m_MinRange ) * m_Value ) + m_MinRange;
    }

    //========================================================================
    // radControllerInputPointGcn::SetRange
    //========================================================================
    virtual void  SetRange( float min, float max )
    {
        //
        // Note that max range CAN be less than min range, if this is what
        // the client wants.
        //

        m_MinRange = min;
        m_MaxRange = max;
    }

    //========================================================================
    // radControllerInputPointGcn::GetRange
    //========================================================================
    virtual void GetRange( float * pMin, float * pMax )
    {
        //
        // Either param can be null, but not both!
        //

        rAssert( pMin != NULL || pMax != NULL );

        if ( pMin != NULL )
        {
            *pMin = m_MinRange;
        }

        if ( pMax != NULL )
        {
            *pMax = m_MaxRange;
        }
	}

    //========================================================================
    // radControllerInputPointGcn::radControllerInputPointGcn
    //========================================================================
    radControllerInputPointGcn( const char * pType, const char * pName, unsigned int offset )
        :
        radRefCount( 0 ),
        m_Value( 0.0f ),
        m_MinRange( 0.0f ),
        m_MaxRange( 1.0f ),
        m_Tolerance( 0.0f ),
        m_TimeInState( 0 ),
        m_TimeOfStateChange( 0 ),
        m_pType( pType ),
        m_pName( pName ),
        m_Offset( offset )
    {
        
        //
        // Object list to store our callbacks
        //

        ::radObjectListCreate( & m_xIOl_Callbacks, g_CtlrAllocator );
        rAssert( m_xIOl_Callbacks != NULL );
    }

    //========================================================================
    // radControllerInputPointGcn::~radControllerInputPointGcn
    //========================================================================
    ~radControllerInputPointGcn( void )
    {
        rAssertMsg( m_xIOl_Callbacks->GetSize() == 0, "Sombody forgot to UnRegister an input point callback" );
    }

    //========================================================================
    // radControllerInputPointGcn Data Members
    //========================================================================
    float m_Value;
    float m_MinRange;
    float m_MaxRange;
    float m_Tolerance;

    unsigned int m_TimeInState;
    unsigned int m_TimeOfStateChange;

    const char * m_pType;
    const char * m_pName;

    unsigned int m_Offset;

    ref< IRadObjectList > m_xIOl_Callbacks;
};


//**********************************************//
//												//	
// Component: radControllerGcn					//
//												//
//**********************************************//
class radControllerGcn
    :
    public IRadControllerGcn,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerGcn" )

	//========================================================================
    // radControllerGcn::iPoll
    //========================================================================
    virtual void iPoll( unsigned int virtualTime )
    {
    	//
        // Query the hardware for current state and store it in the
        // controller buffer, it will be pulled out by virtual time
        // changing.
        //

        if ( GetRefCount( ) > 1 )
        {
            if ( m_hController != -1 )
            {			
				if ( m_SIType == SI_GC_CONTROLLER || m_SIType == SI_GC_WAVEBIRD )
                {
                	if ( g_PadStatus[ m_hController ].err == PAD_ERR_NO_CONTROLLER )
                	{
	                    if( m_hController == 0 )
	                    {
	                	    PADReset( PAD_CHAN0_BIT );
	                    } 
	                    else if( m_hController == 1 )
	                    {
	                	    PADReset( PAD_CHAN1_BIT );
	                    }
	                    else if( m_hController == 2 )
	                    {
	                	    PADReset( PAD_CHAN2_BIT );
	                    } 
	                    else if( m_hController == 3 )
	                    {
	                	    PADReset( PAD_CHAN3_BIT );
                        }
                        else
                        {
                            rAssert( false );
                        }
                	}
                	else
                	{
	    			    ControllerBufferPacket packet;
					    packet.m_time = virtualTime;
					    packet.m_pad = g_PadStatus[ m_hController ];
	                    packet.m_InputType = CONTROLLER;
											    
					    //
					    // Store the data packet in buffer
					    //
					    //m_xIControllerBuffer->EnqueuePacket( &pads[m_hController] );
					    m_xIControllerBuffer->EnqueuePacket( &packet );
					    
					    //
					    // Send our output point data to the device here
					    //
					    IRadControllerOutputPoint * pICop  = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetAt( 0 ) );
	                
					    float newGain  = pICop->GetGain( );
					    u32 command;	// PAD_MOTOR_STOP, PAD_MOTOR_RUMBLE, PAD_MOTOR_STOP_HARD     
		                           
			            if( newGain  > 0.0f )                    
				        {
						    command = PAD_MOTOR_RUMBLE;					
					    }
					    else if( newGain == 0.0f )
					    {
						    command = PAD_MOTOR_STOP;
					    }
					    else if( newGain < 0.0f )
					    {
						    command = PAD_MOTOR_STOP_HARD;
					    }
		    
					    PADControlMotor( m_hController, command );
					}
                }
                else if ( m_SIType == SI_GC_KEYBOARD )
                {
                    //Keyboard
                    if ( g_KbdStatus[m_hController].m_err == KBD_ERR_NO_KEYBOARD )
                    {
                        KBDResetAsync( m_hController, NULL );
                    }
                    else
                    {
                        ControllerBufferPacket packet;
				        packet.m_time = virtualTime;
                        unsigned int i;
                        for ( i = 0; i < KBD_READ_MAX; ++i )
                        {
    				        packet.m_kbd[i] = g_KbdStatus[ m_hController ].m_scan[i];
                        }
                
                        packet.m_InputType = KEYBOARD;
										        
				        //
				        // Store the data packet in buffer
				        //
				        m_xIControllerBuffer->EnqueuePacket( &packet );
                    }
                }
                else if ( m_SIType == SI_GC_STEERING )
                {
    			    ControllerBufferPacket packet;
				    packet.m_time = virtualTime;
				    packet.m_steering = g_LGStatus[ m_hController ];
                    packet.m_InputType = STEERING_WHEEL;
										    
				    //
				    // Store the data packet in buffer
				    //
				    m_xIControllerBuffer->EnqueuePacket( &packet );
				    
				    //
				    // Send our output point data to the device here
				    //
		            m_xIOl_OutputPoints->Reset( );

                    radControllerOutputPointGcnLG* pICop;

                    while( pICop = reinterpret_cast< radControllerOutputPointGcnLG * >( m_xIOl_OutputPoints->GetNext( ) ) )
                    {
                        //Pass in the device we're updating.
                        pICop->Update( g_WheelHandles[m_hController] );
		            }
                }
                else
                {
                    //Unsupported.
                    rAssert( false);
                }
   			}                  
		}
    }

    //========================================================================
    // radControllerGcn::iVirtualTimeReMapped
    //========================================================================
    virtual void iVirtualTimeReMapped( unsigned int virtualTime )
    {
        m_xIControllerBuffer->Flush( );

        IRadControllerInputPointGcn * pICip;

        m_xIOl_InputPoints->Reset( );

        while ( pICip = reinterpret_cast< IRadControllerInputPointGcn * >( m_xIOl_InputPoints->GetNext( ) ) )
        {
            pICip->iVirtualTimeReMapped( virtualTime );
        }
    }

    //========================================================================
    // radControllerGcn::iVirtualTimeChanged
    //========================================================================
    virtual void iVirtualTimeChanged( unsigned int virtualTime )
    {
        if( GetRefCount( ) > 1 )
        {
            unsigned int time = 0;

            //
            // Packets are stamped with virtual time.
            //

			//
            // While packets and packets happened in the past, let each
            // input point handle its own data from the structure.
            //
            
            while ( m_xIControllerBuffer->PeekNextPacket( &time, sizeof( time ) ) )
            {
	            unsigned int packetAge = virtualTime - time;

	            //
                // Virtual time wraps every 50 dates, so test if the packet
                // is less than 25 virtual days (half an int) old.
                //

                if ( packetAge >= 0x7FFFFFFF )
                {
                    //
                    // Packet is in the future, so we are done.
                    //

                    break;
                }

                //
                // Pull this packet from the buffering system
                //
				ControllerBufferPacket packetstatus;
								
                bool packet = m_xIControllerBuffer->DequeuePacket( &packetstatus );
                
                rAssertMsg( packet == true, "InternalError" );

                //
                // If we got the packet, which we better have, update all of our input points
                //
				if ( packet == true )
                {

					m_xIOl_InputPoints->Reset( );

                    IRadControllerInputPointGcn * pIGcnipc;

                    while ( pIGcnipc = reinterpret_cast< IRadControllerInputPointGcn * >( m_xIOl_InputPoints->GetNext( ) ) )
                    {                                  
                        if ( packetstatus.m_InputType == CONTROLLER )
                        {
            				PADStatus padstatus;

                            padstatus = packetstatus.m_pad;

					        pIGcnipc->iVirtualTimeChanged
					        (
						        time, 
						        &padstatus
					        );
                        }
                        else if ( packetstatus.m_InputType == KEYBOARD)
                        {
                            KbdStatus kbdStatus;

                            unsigned int i;
                            for ( i = 0; i < KBD_READ_MAX; ++i )
                            {
                                kbdStatus.m_scan[i] = packetstatus.m_kbd[i];
                            }

                            kbdStatus.m_err = packetstatus.m_pad.err;

						    pIGcnipc->iVirtualTimeChanged
						    (
							    time, 
							    &kbdStatus
						    );
                        }
                        else if ( packetstatus.m_InputType == STEERING_WHEEL)
                        {
                            LGPosition lgStatus;

                            lgStatus = packetstatus.m_steering;

						    pIGcnipc->iVirtualTimeChanged
						    (
							    time, 
							    &lgStatus
						    );
                        }
                        else
                        {
                            rAssert( false );
                        }
                    }
                }
                else
                {
                    //
                    // We should never get here, but break the while loop
                    // just incase.
                    //
	                break;
                }
            }
        }
    }

    //========================================================================
    // radControllerGcn::iSetBufferTime
    //========================================================================
    virtual void iSetBufferTime
    (
        unsigned int milliseconds,
        unsigned int pollingRate
    )
    {
        //
        // This will flush the queue
        //

        m_xIControllerBuffer->SetQueueSize( ( milliseconds / pollingRate ) + 2 );
    }

    //========================================================================
    // radControllerGcn::iSetConnection
    //========================================================================
    virtual void iSetConnected( PADHandle hController )
    {
        //
        // The controller system will tell us if our handle has "gone bad"
        // or if we we're re-opened at the same location.
        //
        m_hController = hController;
    }

    //========================================================================
    // radControllerGcn::IsConnection
    //========================================================================
    virtual bool IsConnected( void )
    {
        //
        // A -1 handle is the flag for being disconnected.
        //
		return (m_hController != -1);
    }

    //========================================================================
    // radControllerGcn::GetType
    //========================================================================
    virtual const char * GetType( void )
    {
        return m_Type;
    }
    
    //========================================================================
    // radControllerGcn::GetClassification
    //========================================================================
    virtual const char * GetClassification( void )
    {
        return m_Classification;
    }

    //========================================================================
    // radControllerGcn::GetNumberOfInputPointsOfType
    //========================================================================
    virtual unsigned int GetNumberOfInputPointsOfType
    (
        const char * pType
    )
    {
        //
        // Count up the number of input points of this time in the input
        // point list
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        m_xIOl_InputPoints->Reset( );

        IRadControllerInputPoint * pICip;

        while ( pICip = reinterpret_cast< IRadControllerInputPointGcn * >( m_xIOl_InputPoints->GetNext( ) ) )
        {
            if ( strcmp( pICip->GetType( ), pType ) == 0 )
            {
                count++;
            }        
        }

        return count;
    }

    //========================================================================
    // radControllerGcn::GetNumberOfOutputPointsOfType
    //========================================================================
    unsigned int GetNumberOfOutputPointsOfType( const char * pType )
    {
        //
        // Count up the number of Output points of this time in the Output
        // point list
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        m_xIOl_OutputPoints->Reset( );

        IRadControllerOutputPoint * pICop;

        while ( pICop = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
        {
            if ( strcmp( pICop->GetType( ), pType ) == 0 )
            {
                count++;
            }        
        }

        return count;
    }

    //========================================================================
    // radControllerGcn::GetInputPointByTypeAndIndex
    //========================================================================
    virtual IRadControllerInputPoint * GetInputPointByTypeAndIndex
    (
        const char * pType,
        unsigned int index
    )
    {
        //
        // Just loop through all of the input points counting each one of
        // that time
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        m_xIOl_InputPoints->Reset( );

        IRadControllerInputPoint * pICip;

        while ( pICip = reinterpret_cast< IRadControllerInputPointGcn * >( m_xIOl_InputPoints->GetNext( ) ) )
        {
            if ( strcmp( pICip->GetType( ), pType ) == 0 )
            {
                if ( count == index )
                {
                    return pICip;
                }

                count++;
            }
        }
        
        return NULL;
    }

    //========================================================================
    // radControllerGcn::GetOutputPointByTypeAndIndex
    //========================================================================
    IRadControllerOutputPoint * GetOutputPointByTypeAndIndex
    (  
        const char * pType,
        unsigned int index
    ) 
    {
        //
        // Just loop through all of the Output points counting each one of
        // that time
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        m_xIOl_OutputPoints->Reset( );

        IRadControllerOutputPoint * pICop;

        while ( pICop = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
        {
            if ( strcmp( pICop->GetType( ), pType ) == 0 )
            {
                if ( count == index )
                {
                    return pICop;
                }

                count++;
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerGcn::GetInputPointByName
    //========================================================================
    virtual IRadControllerInputPoint * GetInputPointByName
    (
        const char * pName
    )
    {
        //
        // Just loop through all of the input points comparing each ones
        // name to the name passed in.
        //

        rAssert( pName != NULL );

        m_xIOl_InputPoints->Reset( );

		IRadControllerInputPoint * pICip;

        while ( pICip = reinterpret_cast< IRadControllerInputPointGcn * >( m_xIOl_InputPoints->GetNext( ) ) )
        {
            if ( strcmp( pName, pICip->GetName( ) ) == 0 )
            {
                return pICip;
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerGcn::GetOutputPointByName
    //========================================================================
    IRadControllerOutputPoint * GetOutputPointByName
    (
        const char * pName
    )
    {
        //
        // Just loop through all of the Output points comparing each ones
        // name to the name passed in.
        //

        rAssert( pName != NULL );

        m_xIOl_OutputPoints->Reset( );

        IRadControllerOutputPoint * pICop;

        while ( pICop = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
        {
            if ( strcmp( pName, pICop->GetName( ) ) == 0 )
            {
                return pICop;
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerGcn::GetLocation
    //========================================================================
    virtual const char * GetLocation( void )
    {
        //
        // Just return the location string;
        //

        return m_xIString_Location->GetChars( );
    }

    //========================================================================
    // radControllerGcn::GetNumberOfInputPoints
    //========================================================================
    virtual unsigned int GetNumberOfInputPoints( void )
    {
        return m_xIOl_InputPoints->GetSize( );
    }

    //========================================================================
    // radControllerGcn::GetInputPointByIndex
    //========================================================================
    virtual IRadControllerInputPoint * GetInputPointByIndex( unsigned int index )
    {
        return reinterpret_cast< IRadControllerInputPointGcn * >( m_xIOl_InputPoints->GetAt( index ) );
    }

    //========================================================================
    // radControllerGcn::GetNumberOfInputPoints
    //========================================================================
    virtual unsigned int GetNumberOfOutputPoints( void )
    {
        return m_xIOl_OutputPoints->GetSize( );
    }

    //========================================================================
    // radControllerGcn::GetOutputPointByIndex
    //========================================================================
    virtual IRadControllerOutputPoint * GetOutputPointByIndex( unsigned int index )
    {
        return reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetAt( index ) );
    }

    //========================================================================
    // radControllerGcn::radControllerGcn
    //========================================================================
	radControllerGcn
    (
        unsigned int thisAllocator,
		PADHandle  	 hController,
        unsigned int virtualTime,
        unsigned int bufferTime,
        unsigned int pollingRate
    )
        :
        radRefCount( 0 ),
        m_hController( hController )
    {
		
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerGcn" );
		//
        // Get a controller buffering system, this is a shared component
        // used by other implementations on other platforms
        //
        radControllerBufferCreate( &m_xIControllerBuffer, g_CtlrAllocator );
        
        //
        // Get an object list to store our input points
        //
        ::radObjectListCreate( & m_xIOl_InputPoints, g_CtlrAllocator );
        ::radObjectListCreate( & m_xIOl_OutputPoints, g_CtlrAllocator );
        
		//
        // Get a string to store our location
        //
        ::radStringCreate( & m_xIString_Location, g_CtlrAllocator );

        //Find out what the fontroller type is..
        //Types we are looking for are:
        //
        //SI_GBA          
        //SI_GC_CONTROLLER
        //SI_GC_WAVEBIRD
        //SI_GC_RECEIVER
        //SI_GC_KEYBOARD  
        //SI_GC_STEERING  
        m_SIType = SIProbe( m_hController );
        if ( m_SIType == SI_GC_KEYBOARD )
        {
            m_SIType = 0x00010000;
        }

        while ( m_SIType == SI_ERROR_BUSY )
        {
            m_SIType = SIProbe( m_hController );
            if ( m_SIType == SI_GC_KEYBOARD )
            {
                m_SIType = 0x00010000;
            }

        }

        if ( m_SIType == SI_ERROR_UNKNOWN )
        {
            //Let's pretend it's a controller in case someone is testing for this.
            m_SIType = SI_GC_CONTROLLER;
        }

        //This is for setting the input points.
        GcnInputPoint* points = NULL;
        unsigned int pointSize;

        switch ( m_SIType )
        {
        case SI_GC_CONTROLLER:
            {
                m_Type = "GcnStandard";
                m_Classification = "Joystick";
                points = g_GcnPoints;
                pointSize = sizeof( g_GcnPoints );
                rDebugPrintf( "STANDARD CONTROLLER connected on channel %d\n", m_hController );
            }   
            break;
        case SI_GC_WAVEBIRD:
        case SI_GC_RECEIVER:
            {
                m_Type = "GcnStandard";
                m_Classification = "Joystick";
                points = g_GcnPoints;
                pointSize = sizeof( g_GcnPoints );
                rDebugPrintf( "WAVEBIRD CONTROLLER connected on channel %d\n", m_hController );

                m_SIType = SI_GC_WAVEBIRD;
            }   
            break;
        case SI_GC_KEYBOARD:
            {
                u8 kbdType = KBDGetType( m_hController );

                if ( kbdType == KBD_ID_ASCII )
                {
                    m_Type = "ASCII";
                }
                else
                {
                    m_Type = "GcnStandard";
                }

                m_Classification = "Keyboard";
                points = g_GcnKbdPoints;
                pointSize = sizeof( g_GcnKbdPoints );
                rDebugPrintf( "%s KEYBOARD connected on channel %d\n", m_Type, m_hController );
            }
            break;
        case SI_GC_STEERING:
            {
                m_Type = "GcnStandard";
                m_Classification = "Wheel";
                points = g_LGPoints;
                pointSize = sizeof( g_LGPoints );
                rDebugPrintf( "WHEEL connected on channel %d\n", m_hController );
            }   
            break;
        default:
            //Don't support this...  yet
            rAssert( false );
        }

        //
        // Initialize the controller buffer to store game pad sized packets.
        // The queue size will get set later.
        m_xIControllerBuffer->Initialize( 1, sizeof(ControllerBufferPacket) /* buffer size */);

        //
        // Create our location name based on our port and slot
        //
        m_xIString_Location->SetSize(9);
        m_xIString_Location->Append( "Channel" );
		m_xIString_Location->Append( (unsigned int) m_hController );
               
        //
        // Create all of our intput points, this is always the same for every
        // gcn controller.  See static array above.
        //
        for ( unsigned int input = 0; input < ( pointSize / sizeof( GcnInputPoint ) ); input++ )
        {
            IRefCount * pObject = static_cast< IRefCount * >(
                new( g_CtlrAllocator ) radControllerInputPointGcn( points[ input ].m_pType,  points[ input ].m_pName, points[ input ].m_Mask )
                                                            );
            m_xIOl_InputPoints->AddObject( pObject );
        }

		//
		// Initialize the input point values
		//
        if ( m_SIType == SI_GC_CONTROLLER || m_SIType == SI_GC_WAVEBIRD )
        {		
		    PADStatus status[ SI_MAX_CHAN ];
		    PADRead( (PADStatus*) & status );
		    PADClamp( (PADStatus*) & status );
		    
		    ref< radControllerInputPointGcn > pInputPoint;
		    
		    m_xIOl_InputPoints->Reset( );

            while( pInputPoint = reinterpret_cast< radControllerInputPointGcn * >( m_xIOl_InputPoints->GetNext( ) ) )
            {
			    pInputPoint->iInitialize( & status[ m_hController ] );
		    }

		    //
		    // Create the output points
		    //
            if ( m_xIOl_OutputPoints != NULL )
            {
                radControllerOutputPointGcn * pMotor = new( g_CtlrAllocator ) radControllerOutputPointGcn( "Motor" );

                m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pMotor ) );
            }
        }
        else if ( m_SIType == SI_GC_KEYBOARD )
        {
            KbdStatus status;
            status.m_err = KBDRead( m_hController, status.m_scan );
            KBDResetAsync(m_hController, NULL);

		    ref< radControllerInputPointGcn > pInputPoint;
		    
		    m_xIOl_InputPoints->Reset( );

            while( pInputPoint = reinterpret_cast< radControllerInputPointGcn * >( m_xIOl_InputPoints->GetNext( ) ) )
            {
			    pInputPoint->iInitialize( & status );
		    }
        }
        else if ( m_SIType == SI_GC_STEERING )
        {
            int openStatus = LGOpen( m_hController, &g_WheelHandles[m_hController] );
            if ( openStatus == LG_SUCCESS )
            {
                LGPosition status[ SI_MAX_CHAN ];
		        LGRead( (LGPosition*) & status );
		        
		        ref< radControllerInputPointGcn > pInputPoint;
		        
		        m_xIOl_InputPoints->Reset( );

                while( pInputPoint = reinterpret_cast< radControllerInputPointGcn * >( m_xIOl_InputPoints->GetNext( ) ) )
                {
			        pInputPoint->iInitialize( & status[ m_hController ] );
		        }

		        //
		        // Create the output points
		        //
                if ( m_xIOl_OutputPoints != NULL )
                {
                    radControllerOutputPointGcnLG * pSpring = new( g_CtlrAllocator ) radControllerOutputPointGcnLG( "CenterSpring", LG_TYPE_SPRING );

                    m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pSpring ) );

                    radControllerOutputPointGcnLG * pDamper = new( g_CtlrAllocator ) radControllerOutputPointGcnLG( "BaseDamper", LG_TYPE_DAMPER );

                    m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pDamper ) );

                    radControllerOutputPointGcnLG * pConstant = new( g_CtlrAllocator ) radControllerOutputPointGcnLG( "Constant", LG_TYPE_CONSTANT );

                    m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pConstant ) );

                    radControllerOutputPointGcnLG * pRumble = new( g_CtlrAllocator ) radControllerOutputPointGcnLG( "Rumble", LG_TYPE_TRIANGLE );

                    m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pRumble ) );

                    radControllerOutputPointGcnLG * pHeavyRumble = new( g_CtlrAllocator ) radControllerOutputPointGcnLG( "HeavyRumble", LG_TYPE_SQUARE );

                    m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pHeavyRumble ) );
                }
            }
        }
        else
        {
            rAssert( false );
        }

        //
        // Set everything to a know state using our regular runtime functions.
        // Note that the controller might get created during gameplay after
        // the controller system has been run for a while.
        // 

        iSetBufferTime( bufferTime, pollingRate );
        iVirtualTimeReMapped( virtualTime );
    }

    //========================================================================
    // radControllerGcn::~radControllerGcn
    //========================================================================
    ~radControllerGcn( void )
    {
        if ( m_hController != -1 )
        {
            m_hController = -1;
			
        }
    }

    //========================================================================
    // radControllerGcn Data Members
    //========================================================================
   	PADHandle	m_hController;
       
    ref< IRadObjectList >             m_xIOl_InputPoints;
    ref< IRadObjectList >             m_xIOl_OutputPoints;

    ref< IRadString >                 m_xIString_Location;
    ref< IRadControllerBuffer >       m_xIControllerBuffer;

    const char*                       m_Type;
    const char*                       m_Classification;
    u32                               m_SIType;

};


//**********************************************//
//												//
// Component: radControllerSystemGcn			//
//												//
//**********************************************//
class radControllerSystemGcn
    :
    public IRadControllerSystem,
    public IRadTimerCallback,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerSystemGcn" )

    //=========================================================================
	// radControllerSystemGcn::GCNGetDevices
	//=========================================================================
	//

	bool GCNGetDevices( unsigned int* chanMask )
	{
		*chanMask = 0;
		
		for (int chan = 0; chan < SI_MAX_CHAN; chan++)
		{
            if ( g_PadStatus[ chan ].err == PAD_ERR_NONE )
            {
                //One of the devices is allright.
                *chanMask |= 1 << chan;
            }
            else
            {
                s32 err = KBDRead( chan, g_KbdStatus[chan].m_scan );

                if ( err == KBD_ERR_READY )
                {
                    *chanMask |= 1 << chan;
                }
                else 
                {
                    if ( err == KBD_ERR_NO_KEYBOARD &&
                         g_PadStatus[ chan ].err == PAD_ERR_NO_CONTROLLER )
                         
                    {
                    	//Is this a steering wheel?
                    	err = SIProbe( chan );
                    	while ( err == SI_ERROR_BUSY )
                    	{ 
                    		err = SIProbe( chan );

                    	}

                        if( err == SI_GC_KEYBOARD )
                        {
                            err = 0x00010000;
                        }

                    	if ( err == SI_GC_STEERING )
                    	{
		                    *chanMask |= 1 << chan;
                    	}
                    	else if ( err == SI_GC_CONTROLLER ||
                    		      err == SI_GC_RECEIVER ||
                    		      err == SI_GC_WAVEBIRD )
	                    {
	                        if( chan == 0 )
	                        {
	                	        PADReset( PAD_CHAN0_BIT );
	                        } 
	                        else if( chan == 1 )
	                        {
	                	        PADReset( PAD_CHAN1_BIT );
	                        }
	                        else if( chan == 2 )
	                        {
	                	        PADReset( PAD_CHAN2_BIT );
	                        } 
	                        else if( chan == 3 )
	                        {
	                	        PADReset( PAD_CHAN3_BIT );
	                        }
	                    }
/*
	                    else if ( err == SI_GC_KEYBOARD )
	                    {
	                        KBDReset( chan );
	                    }
*/
                        else
                        {
                            // unknown device, assuming no device attached
                            //
                            g_PadStatus[ chan ].err = PAD_ERR_NO_CONTROLLER;
                        }
	                }
                    else
                    {
                        //Some other error.
                        return false;
                    }
                }
            }
		}
	
        return true;
	}

	//=========================================================================
    // radControllerSystemGcn::CheckDeviceConnectionStatus
    //=========================================================================
    void CheckDeviceConnectionStatus( void )
    {
		//
        // Check if devices have been inserted or removed
        //
		unsigned int deviceMap = 0;
        
        bool result = GCNGetDevices( &deviceMap );

        if (!result)
        {
            return;
        }
		
        if (deviceMap != m_DeviceMap)  
        {           
            unsigned int changes = deviceMap ^ m_DeviceMap;

            m_DeviceMap = deviceMap;
			
		    for ( unsigned int i = 0; i < SI_MAX_CHAN; i ++ )
            {
                unsigned int deviceMask = 1 << i;

                if ( changes & deviceMask )
                {
                    //
                    // Find the controller in question (may not exist though)
                    //
                    ref< IRadController > xIController;
                    ref< IRadControllerGcn > xIGcnController;

                    char location[ 255 ];

					PADHandle hController;

                    sprintf( location, "Channel%d", i );

                    xIController = GetControllerAtLocation( location );

                    if ( xIController != NULL )
                    {
                        xIGcnController = (IRadControllerGcn*) xIController.m_pInterface;
                        rAssert( xIGcnController != NULL );
                    }

                    if ( deviceMap & deviceMask )
                    {
                        hController = i;

						//
                        // Here a device has been inserted, so open it
                        //
                        if ( xIGcnController != NULL )
                        {
                            //
                            // Here the device is already constructed, just set
                            // it to the connected state.
                            //
                            xIGcnController->iSetConnected( hController );
                        }
                        else
                        {
                            //
                            // Here the controller at this location has not yet been 
                            // constructed, so construct a new controller
	                        //
							unsigned int virtualTime = 0;
                            unsigned int pollingRate = 10;
      
							virtualTime = radTimeGetMilliseconds() + m_VirtualTimeAdjust;
                             
                            if ( m_xITimer != NULL )
                            {
								pollingRate = m_xITimer->GetTimeout( );
                            }

                            xIController = new ( g_CtlrAllocator ) radControllerGcn
                            (
                                g_CtlrAllocator,
                                hController,
                                virtualTime,
                                m_EventBufferTime,
                                pollingRate
                            );

                            m_xIOl_Controllers->AddObject( xIController );
                        }                            
                    }
                    else
                    {
						//
                        // Here a device has been removed
                        //
                        if ( xIGcnController != NULL )
                        {
							xIGcnController->iSetConnected( hController );

                            if ( strcmp(xIGcnController->GetType(), "Wheel") == 0 )
                            {
                                LGClose( g_WheelHandles[hController] );
                            }
                            //We need to remove this from the set as the next thing to 
                            //plug in could be a new type of controller.
                            m_xIOl_Controllers->RemoveObject( xIController );
                        }

                        hController = -1;
                    }

                    rAssert( xIController != NULL );

                    AddRef( ); // Don't want to self destruct while we are calling out

                    IRadWeakInterfaceWrapper * pIWir;

                    m_xIOl_Callbacks->Reset( );

                    while( pIWir = reinterpret_cast< IRadWeakInterfaceWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
                    {

                        IRadControllerConnectionChangeCallback * pCallback = (IRadControllerConnectionChangeCallback *) pIWir->GetWeakInterface( );
                        pCallback->OnControllerConnectionStatusChange( xIController );
                    }

                    Release( );                                                                                       
                }
            }
        }
    }
    
    
    //========================================================================
    // radControllerSystemGcn:OnTimerDone
    //========================================================================
    virtual void OnTimerDone( unsigned int elapsedtime, void* pUserData )
    {
    	unsigned int currentTime = radTimeGetMilliseconds( );
        //
        // Check for device connections, removals
        //
    
        PADRead( g_PadStatus );
        PADClampCircle( g_PadStatus );
        LGRead( g_LGStatus );
         
        CheckDeviceConnectionStatus( );
       
        //
        // Now, update all of our controllers
        //

        m_xIOl_Controllers->Reset( );

        IRadControllerGcn * pIGcn;

        while ( pIGcn = reinterpret_cast< IRadControllerGcn * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            //
            // The controller stamps packets with virtual time, so
            // we pass in virtual time.
            //
            pIGcn->iPoll( currentTime + m_VirtualTimeAdjust );
        }

        //
        // If the client is not driving us, we drive ourselves from this timer
        //
        if ( m_UsingVirtualTime == false )
        {
            SetVirtualTime( currentTime );
        }
    }

    //========================================================================
    // radControllerSystemGcn::GetNumberOfControlelrs
    //========================================================================
    virtual unsigned int GetNumberOfControllers( void )
    {
        return m_xIOl_Controllers->GetSize( );
    }

    //========================================================================
    // radControllerSystemGcn::GetControllerByIndex
    //========================================================================
    virtual IRadController * GetControllerByIndex( unsigned int myindex )
    {
        return reinterpret_cast< IRadControllerGcn * >( m_xIOl_Controllers->GetAt( myindex ) );
    }

    //========================================================================
    // radControllerSystemGcn::GetControllerAtLocation
    //========================================================================
    virtual IRadController * GetControllerAtLocation
    (
        const char * pLocation
    )
    {
        //
        // Just loop through all of the controllers asking it for its location
        // if we find a match, return it.
        //

        rAssert( pLocation != NULL );

        m_xIOl_Controllers->Reset( );

        IRadController * pIC;

        while ( pIC = reinterpret_cast< IRadControllerGcn * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            if ( strcmp( pIC->GetLocation(), pLocation ) == 0 )
            {
                return pIC;
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerSystemGcn::SetBufferTime
    //========================================================================
    virtual void SetBufferTime( unsigned int milliseconds )
    {
        if ( milliseconds == 0 )
        {
            //
            // We are always buffering behind the scenes, so we set the
            // buffering time to one 60Hz frame even if the client thinks
            // we are just maintaining state.
            //

            m_UsingVirtualTime = false;

            MapVirtualTime( 0, 0 );

            milliseconds = 8;
        }
        else
        {
            m_UsingVirtualTime = true;
        }

        m_EventBufferTime = milliseconds;
        
        unsigned int pollingRate = 16;

		if( m_xITimer != NULL )
		{
        	pollingRate = m_xITimer->GetTimeout( );
		}
		
		if( m_xIOl_Controllers != NULL )
		{
        	m_xIOl_Controllers->Reset( );

	        IRadControllerGcn * pIGcn;

        	while ( pIGcn = reinterpret_cast< IRadControllerGcn * >( m_xIOl_Controllers->GetNext( ) ) )
        	{
            	pIGcn->iSetBufferTime( milliseconds, pollingRate );
        	}
        }
    }

    //========================================================================
    // radControllerSystemGcn::MapVirtualTime
    //========================================================================
    virtual void MapVirtualTime
    (
        unsigned int systemTicks,
        unsigned int virtualTicks
    )
    {
        //
        // Here the client has requested a (re)mapping of virtual time to
        // timer manager time.
        //

        //
        // Recalculate the adjustment factor.  This number represents the
        // number to add to timer manager ticks to get game ticks.
        //
        m_VirtualTimeAdjust = virtualTicks - systemTicks;

        //
        // Inform each controller that virtual time has been remapped.
        //

        m_xIOl_Controllers->Reset( );

        IRadControllerGcn * pIGcn;

        while ( pIGcn = reinterpret_cast< IRadControllerGcn * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            pIGcn->iVirtualTimeReMapped( radTimeGetMilliseconds() + m_VirtualTimeAdjust );
        }       
    }

    //========================================================================
    // radControllerSystemGcn::SetVirtualTime
    //========================================================================
    virtual void SetVirtualTime( unsigned int virtualTime )
    {
        //
        // Inform each controller that virtual time has changed.
        //

        m_xIOl_Controllers->Reset( );

        IRadControllerGcn * pIGcn;

        while ( pIGcn = reinterpret_cast< IRadControllerGcn * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            pIGcn->iVirtualTimeChanged( virtualTime );
        }
    }

    //========================================================================
    // radControllerSystemGcn::SetCaptureRate
    //========================================================================
    virtual void SetCaptureRate( unsigned int ms )
    {
        //
        // This is a very important message.
        // If the capture rate is too fast,
        // the values of the input points will appear to 
        // drift between valid and invalid readings.  I
        // don't know if this is a hardware limitation or what.
        // Just don't try to read the input points too often.
        //
        rWarningMsg( ms >= 16, "GCNCONTROLLER WARNING: Capture Rate <= 16ms causes bad values\n" );

        m_xITimer->SetTimeout( ms );

        //
        // Reset event queue size given new filling rate
        //

        SetBufferTime( m_EventBufferTime );
    }

    //========================================================================
    // radControllerSystemGcn::RegisterConnectionChangeCallback
    //========================================================================
    virtual void RegisterConnectionChangeCallback
    (
        IRadControllerConnectionChangeCallback * pCallback
    )
    {
        //
        // Store the callback in a weak interface wrapper so we can store
        // it in an object list.
        //

        rAssert( pCallback != NULL );

        ref< IRadWeakInterfaceWrapper > xIWir;

        ::radWeakInterfaceWrapperCreate( & xIWir, g_CtlrAllocator );

        xIWir->SetWeakInterface( pCallback );

        m_xIOl_Callbacks->AddObject( xIWir );
    }

    //========================================================================
    // radControllerSystemGcn::UnRegisterConnectionChagneCallback
    //========================================================================
    virtual void UnRegisterConnectionChangeCallback
    (
        IRadControllerConnectionChangeCallback * pCallback
    )
    {
        //
        // Look for the callback in the list and remove it if found.
        //

        rAssert( pCallback != NULL );

        IRadWeakInterfaceWrapper * pIWir;

        m_xIOl_Callbacks->Reset( );

        while ( pIWir = reinterpret_cast< IRadWeakInterfaceWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
        {
            if ( pIWir->GetWeakInterface() == pCallback )
            {
                m_xIOl_Callbacks->RemoveObject( pIWir  );
                return;
            }
        }
        
        rAssertMsg( false, "Controller connection change callback not registered." );            
    }

    //========================================================================
    // radControllerSystemGcn::Service
    //========================================================================
    void Service( void )
    {
        m_xITimerList->Service(  );   
    }

    //========================================================================
    // radControllerSystemGcn::radControllerSystemGcn
    //========================================================================
	radControllerSystemGcn
    (
        IRadControllerConnectionChangeCallback* pConnectionChangeCallback,
        radMemoryAllocator thisAllocator
    )
        :
        m_UsingVirtualTime( false ),
        m_VirtualTimeAdjust( 0 ),
        m_EventBufferTime( 0 ),
        m_DefaultConnectionChangeCallback( NULL ),
        m_DeviceMap( 0 )
    {
        //
        // Set our singleton to "this"
        //
        rAssert( g_pTheCS == NULL );
        g_pTheCS = this;
        
        g_CtlrAllocator = thisAllocator;

		//
		// Initialize the GameCube Nintendo peripheral control software 
		// This is the default call and will create 4 game controllers
		// Must include these 4 functions to work properly

		VIInit();
        KBDInit();
		PADInit();
        LGInit();

        g_isLGInitialized = true;

		//
        // Create a timer to poll with, this will be changed to a thread if
        // people start complaining about latencies.
        //

        //
        // Create a timer list to drive the processing of controllers
        //
    
        radTimeCreateList( &m_xITimerList, 1, g_CtlrAllocator );

        //
        // This initial timer period is overridden by 
        // changes to the capture rate
        //
        m_xITimerList->CreateTimer( & m_xITimer, 16, this );

        //
        // Object list to hold controllers
        //
        ::radObjectListCreate( & m_xIOl_Controllers, g_CtlrAllocator );
        rAssert( m_xIOl_Controllers != NULL );

        //
        // Object list to hold callbacks
        //
        ::radObjectListCreate( & m_xIOl_Callbacks, g_CtlrAllocator );
        rAssert( m_xIOl_Callbacks != NULL );

        //
        // Register the default connection state callback
        //
        m_DefaultConnectionChangeCallback = pConnectionChangeCallback;
        if( pConnectionChangeCallback )
        {
            m_DefaultConnectionChangeCallback = pConnectionChangeCallback;
            RegisterConnectionChangeCallback( pConnectionChangeCallback );
        }

        //
        // If there is no connection change callback, wait synchronously for the connection
        //
        rWarningMsg( pConnectionChangeCallback != NULL, "Spinning while initializing controllers synchronously." );
        if( pConnectionChangeCallback == NULL )
        {
            while( m_xIOl_Controllers->GetSize( ) == 0 )
            {
                CheckDeviceConnectionStatus( );
            }
        }

        //
        // Set everything to known state
        //        
        SetCaptureRate( 16 );   // This needs to be at least 16 ms
        MapVirtualTime( 0, 0 );
        SetBufferTime( 0 );
    }
        
    //========================================================================
    // radControllerSystemGcn::~radControllerSystemGcn
    //========================================================================
    ~radControllerSystemGcn( void )
    {
        if( m_DefaultConnectionChangeCallback != NULL )
        {
            UnRegisterConnectionChangeCallback( m_DefaultConnectionChangeCallback );
            m_DefaultConnectionChangeCallback = NULL;
        }
        
        //
        // Make sure the client(s) unregistered all of their callbacks.
        //
        rAssertMsg( m_xIOl_Callbacks->GetSize() == 0, "Somebody forgot to unregister a controller connection change callback" );

        g_CtlrAllocator = RADMEMORY_ALLOC_DEFAULT;

        LGShutdown();

        //
        // Set the singleton back to null.
        //
        rAssert( g_pTheCS == this );
        g_pTheCS = NULL;
    }


    //========================================================================
    // Data Members
    //========================================================================
    bool		    m_UsingVirtualTime;
    unsigned int    m_VirtualTimeAdjust;
    unsigned int    m_EventBufferTime;
    unsigned int    m_DeviceMap;

    IRadControllerConnectionChangeCallback* m_DefaultConnectionChangeCallback;

	ref< IRadObjectList >     m_xIOl_Callbacks;

    ref< IRadObjectList >     m_xIOl_Controllers;
    ref< IRadTimer >          m_xITimer;
    ref< IRadTimerList >      m_xITimerList;
};


//============================================================================
// Function:    radControllerInitialize
// Paramters:   pConnectionChangeCallback - a callback that is called when the
//                  connection status of controllers changes.  It is called
//                  as soon as a controller is found and properly initialized.
//============================================================================
void radControllerInitialize
(
    IRadControllerConnectionChangeCallback* pConnectionChangeCallback,
    radMemoryAllocator alloc
)
{
    rAssert( g_pTheCS == NULL );

    new ( alloc ) radControllerSystemGcn( pConnectionChangeCallback, alloc );
}

//============================================================================
// Function: radControllerTerminate
//============================================================================
//
// Use this function to release the interface to the controller system object.
//
void radControllerTerminate( void )
{
    radRelease( g_pTheCS, NULL );
}

//============================================================================
// Function: radControllerSystemGet
//============================================================================
//
// Use this function to obtain an interface to the controller system object.
//
IRadControllerSystem* radControllerSystemGet( void )
{
    rAssert( g_pTheCS != NULL );

    return( g_pTheCS );
}

//============================================================================
// Function: radControllerSystemGet
//============================================================================
//
// Use this function to drive the processing of the controller system
//
void radControllerSystemService( void )
{
    if( g_pTheCS != NULL )
    {
        g_pTheCS->Service( );
    }
	
}

