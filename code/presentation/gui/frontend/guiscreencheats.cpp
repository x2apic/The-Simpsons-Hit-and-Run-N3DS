#include <presentation/gui/frontend/guiscreencheats.h>
#include <presentation/gui/guimenu.h>
#include <cheats/cheatinputsystem.h>
#include <cheats/portsettings.h>
#include <events/eventmanager.h>
#include <memory/srrmemory.h>

#include <raddebug.hpp>
#include <Screen.h>
#include <Page.h>
#include <Group.h>
#include <Text.h>
#include <Sprite.h>

enum eCheatKind
{
    CHEAT_KIND_UNLOCK,
    CHEAT_KIND_ONE_SHOT,
    CHEAT_KIND_TOGGLE
};

struct CheatEntry
{
    eCheatID    id;
    const char* name;
    eCheatKind  kind;
};

static const CheatEntry s_Cheats[] =
{
    { CHEAT_ID_UNLOCK_EVERYTHING,           "UNLOCK ALL",         CHEAT_KIND_UNLOCK   },
    { CHEAT_ID_UNLOCK_CARDS,                "UNLOCK CARDS",       CHEAT_KIND_UNLOCK   },
    { CHEAT_ID_UNLOCK_SKINS,                "UNLOCK OUTFITS",     CHEAT_KIND_UNLOCK   },
    { CHEAT_ID_UNLOCK_MISSIONS,             "UNLOCK MISSIONS",    CHEAT_KIND_UNLOCK   },
    { CHEAT_ID_UNLOCK_MOVIES,               "UNLOCK MOVIES",      CHEAT_KIND_UNLOCK   },
    { CHEAT_ID_UNLOCK_VEHICLES,             "UNLOCK CARS",        CHEAT_KIND_UNLOCK   },
    { CHEAT_ID_EXTRA_COINS,                 "+100 COINS",         CHEAT_KIND_ONE_SHOT },
    { CHEAT_ID_INVINCIBLE_CAR,              "INVINCIBLE CAR",     CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_NO_TOP_SPEED,                "NO SPEED LIMIT",     CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_HIGH_ACCELERATION,           "FAST ACCELERATION",  CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_CAR_JUMP_ON_HORN,            "HORN JUMP",          CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_ONE_TAP_TRAFFIC_DEATH,       "ONE-HIT TRAFFIC",    CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_FULL_DAMAGE_TO_CAR,          "WRECKED CAR LOOK",   CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_REDBRICK,                    "RED BRICK CARS",     CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_KICK_TOGGLES_CHARACTER_MODEL,"KICK SWAPS MODEL",   CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_UNLOCK_CAMERAS,              "EXTRA CAMERAS",      CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_SHOW_SPEEDOMETER,            "SPEEDOMETER",        CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_SHOW_AVATAR_POSITION,        "SHOW POSITION",      CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_PLAY_CREDITS_DIALOG,         "CREDITS DIALOGUE",   CHEAT_KIND_TOGGLE   },
    { CHEAT_ID_EXTRA_TIME,                  "5X MISSION TIME",    CHEAT_KIND_TOGGLE   },
};
static const int NUM_CHEAT_ENTRIES = sizeof( s_Cheats ) / sizeof( s_Cheats[ 0 ] );

static void SetAllStrings( Scrooby::Text* text, const char* string )
{
    if( text == NULL )
    {
        return;
    }
    int n = text->GetNumOfStrings();
    if( n < 1 ) n = 1;
    for( int i = 0; i < n; i++ )
    {
        text->SetString( i, string );
    }
}

static int CentreY( Scrooby::BoundedDrawable* d )
{
    int x0, y0, x1, y1;
    d->GetBoundingBox( x0, y0, x1, y1 );
    return ( y0 + y1 ) / 2;
}

CGuiScreenCheats::CGuiScreenCheats
(
    Scrooby::Screen* pScreen,
    CGuiEntity* pParent,
    bool isInGame
)
:   CGuiScreen( pScreen, pParent, GUI_SCREEN_ID_DISPLAY ),
    m_pMenu( NULL ),
    m_mapValue( NULL ),
    m_cheatValue( NULL ),
    m_applyText( NULL ),
    m_cheatIndex( 0 ),
    m_isInGame( isInGame ),
    m_changed( false )
{
MEMTRACK_PUSH_GROUP( "CGuiScreenCheats" );
    Scrooby::Page* pPage = m_pScroobyScreen != NULL ? m_pScroobyScreen->GetPage( "Display" ) : NULL;
    rAssert( pPage != NULL );

    m_pMenu = new CGuiMenu( this, NUM_MENU_ITEMS );
    rAssert( m_pMenu != NULL );

    Scrooby::Group* resRow   = pPage != NULL ? pPage->GetGroup( "Resolution" )  : NULL;
    Scrooby::Group* timerRow = pPage != NULL ? pPage->GetGroup( "DisplayMode" ) : NULL;
    Scrooby::Group* cheatRow = pPage != NULL ? pPage->GetGroup( "ColourDepth" ) : NULL;
    Scrooby::Group* gammaRow = pPage != NULL ? pPage->GetGroup( "Gamma" )       : NULL;
    Scrooby::Group* menu     = pPage != NULL ? pPage->GetGroup( "Menu" )        : NULL;

    Scrooby::Text* timerLabel = timerRow != NULL ? timerRow->GetText( "DisplayMode" )       : NULL;
    Scrooby::Text* timerValue = timerRow != NULL ? timerRow->GetText( "DisplayMode_Value" ) : NULL;
    Scrooby::Text* mapLabel   = resRow   != NULL ? resRow->GetText( "Resolution" )          : NULL;
    m_mapValue                = resRow   != NULL ? resRow->GetText( "Resolution_Value" )    : NULL;
    Scrooby::Text* cheatLabel = cheatRow != NULL ? cheatRow->GetText( "ColourDepth" )       : NULL;
    m_cheatValue              = cheatRow != NULL ? cheatRow->GetText( "ColourDepth_Value" ) : NULL;
    m_applyText               = menu     != NULL ? menu->GetText( "ApplyChanges" )          : NULL;

    if( timerLabel != NULL && timerValue != NULL && mapLabel != NULL && m_mapValue != NULL &&
        cheatLabel != NULL && m_cheatValue != NULL && m_applyText != NULL )
    {
        {
            const int timerY     = CentreY( timerLabel );
            const int pitch      = CentreY( cheatLabel ) - timerY;
            const int labelToVal = CentreY( cheatLabel ) - CentreY( m_cheatValue );
            timerRow->Translate( 0, pitch );
            resRow->Translate( 0, timerY - CentreY( mapLabel ) );
            const int cheatMove = ( timerY - pitch ) - CentreY( cheatLabel );
            cheatRow->Translate( 0, cheatMove );
            const int cheatValueY = CentreY( m_cheatValue ) + cheatMove;
            menu->Translate( 0, ( cheatValueY - labelToVal - labelToVal / 2 ) - CentreY( m_applyText ) );
        }

        timerLabel->SetString( 0, "NO TIMERS" );
        timerValue->SetString( 0, "OFF" );
        timerValue->SetString( 1, "ON" );
        m_pMenu->AddMenuItem( timerLabel,
                              timerValue,
                              NULL,
                              NULL,
                              timerRow->GetSprite( "DisplayMode_ArrowL" ),
                              timerRow->GetSprite( "DisplayMode_ArrowR" ),
                              SELECTION_ENABLED | VALUES_WRAPPED | TEXT_OUTLINE_ENABLED );

        SetAllStrings( mapLabel, "MAP" );
        m_pMenu->AddMenuItem( mapLabel,
                              m_mapValue,
                              NULL,
                              NULL,
                              resRow->GetSprite( "Resolution_ArrowL" ),
                              resRow->GetSprite( "Resolution_ArrowR" ),
                              SELECTION_ENABLED | VALUES_WRAPPED | TEXT_OUTLINE_ENABLED );

        SetAllStrings( cheatLabel, "CHEAT" );
        m_pMenu->AddMenuItem( cheatLabel,
                              m_cheatValue,
                              NULL,
                              NULL,
                              cheatRow->GetSprite( "ColourDepth_ArrowL" ),
                              cheatRow->GetSprite( "ColourDepth_ArrowR" ),
                              SELECTION_ENABLED | VALUES_WRAPPED | TEXT_OUTLINE_ENABLED );

        m_pMenu->AddMenuItem( m_applyText );

        RefreshMapLabel();
        RefreshCheatLabels();
    }
    else
    {
        if( resRow != NULL ) resRow->SetVisible( false );
        m_mapValue = NULL;
        m_cheatValue = NULL;
        m_applyText = NULL;
    }

    if( gammaRow != NULL ) gammaRow->SetVisible( false );
MEMTRACK_POP_GROUP( "CGuiScreenCheats" );
}

CGuiScreenCheats::~CGuiScreenCheats()
{
    if( m_pMenu != NULL )
    {
        delete m_pMenu;
        m_pMenu = NULL;
    }
}

void CGuiScreenCheats::HandleMessage
(
	eGuiMessage message,
	unsigned int param1,
	unsigned int param2
)
{
    if( m_state == GUI_WINDOW_STATE_RUNNING && m_pMenu != NULL &&
        m_pMenu->GetNumItems() == NUM_MENU_ITEMS )
    {
        switch( message )
        {
            case GUI_MSG_CONTROLLER_START:
            {
                if( m_isInGame )
                {
                    m_pParent->HandleMessage( GUI_MSG_UNPAUSE_INGAME );
                }
                break;
            }
            case GUI_MSG_CONTROLLER_LEFT:
            case GUI_MSG_CONTROLLER_RIGHT:
            {
                if( m_pMenu->GetSelection() == MENU_ITEM_CHEAT )
                {
                    StepCheat( message == GUI_MSG_CONTROLLER_RIGHT ? 1 : -1 );
                }
                else if( m_pMenu->GetSelection() == MENU_ITEM_MAP )
                {
                    PortSettings::SetMapOnTop( !PortSettings::MapOnTop() );
                    m_changed = true;
                    RefreshMapLabel();
                }
                break;
            }
            case GUI_MSG_MENU_SELECTION_CHANGED:
            {
                this->SetButtonVisible( BUTTON_ICON_ACCEPT, param1 == MENU_ITEM_APPLY );
                break;
            }
            case GUI_MSG_MENU_SELECTION_VALUE_CHANGED:
            {
                if( param1 == MENU_ITEM_NO_TIMERS )
                {
                    PortSettings::SetNoTimers( param2 == 1 );
                    m_changed = true;
                }
                break;
            }
            case GUI_MSG_MENU_SELECTION_MADE:
            {
                if( param1 == MENU_ITEM_APPLY )
                {
                    ApplyCheat();
                }
                break;
            }
            default:
            {
                break;
            }
        }

        m_pMenu->HandleMessage( message, param1, param2 );
    }

	CGuiScreen::HandleMessage( message, param1, param2 );
}

void CGuiScreenCheats::StepCheat( int direction )
{
    m_cheatIndex = ( m_cheatIndex + direction + NUM_CHEAT_ENTRIES ) % NUM_CHEAT_ENTRIES;
    RefreshCheatLabels();
}

void CGuiScreenCheats::ApplyCheat()
{
    const CheatEntry& cheat = s_Cheats[ m_cheatIndex ];
    CheatInputSystem* cheats = GetCheatInputSystem();

    switch( cheat.kind )
    {
        case CHEAT_KIND_UNLOCK:
        {
            cheats->SetCheatEnabled( cheat.id, true );

            if( cheat.id == CHEAT_ID_UNLOCK_EVERYTHING )
            {
                for( int id = CHEAT_ID_UNLOCK_BEGIN; id < CHEAT_ID_UNLOCK_EVERYTHING; id++ )
                {
                    cheats->SetCheatEnabled( static_cast<eCheatID>( id ), true );
                }
            }
            break;
        }
        case CHEAT_KIND_ONE_SHOT:
        {
            cheats->SetCheatEnabled( cheat.id, true );
            break;
        }
        case CHEAT_KIND_TOGGLE:
        {
            cheats->SetCheatEnabled( cheat.id, !cheats->IsCheatEnabled( cheat.id ) );
            break;
        }
    }

    GetEventManager()->TriggerEvent( EVENT_FE_CHEAT_SUCCESS );
    RefreshCheatLabels();
}

void CGuiScreenCheats::RefreshMapLabel()
{
    SetAllStrings( m_mapValue, PortSettings::MapOnTop() ? "TOP SCREEN" : "BOTTOM SCREEN" );
}

void CGuiScreenCheats::RefreshCheatLabels()
{
    if( m_cheatValue == NULL || m_applyText == NULL )
    {
        return;
    }

    const CheatEntry& cheat = s_Cheats[ m_cheatIndex ];
    const bool on = GetCheatInputSystem()->IsCheatEnabled( cheat.id );

    SetAllStrings( m_cheatValue, cheat.name );

    const char* action = "APPLY";
    switch( cheat.kind )
    {
        case CHEAT_KIND_UNLOCK:   action = on ? "UNLOCKED" : "APPLY";    break;
        case CHEAT_KIND_ONE_SHOT: action = "APPLY";                      break;
        case CHEAT_KIND_TOGGLE:   action = on ? "TURN OFF" : "TURN ON";  break;
    }
    SetAllStrings( m_applyText, action );
}

void CGuiScreenCheats::InitIntro()
{
    if( m_pMenu != NULL && m_pMenu->GetNumItems() == NUM_MENU_ITEMS )
    {
        m_pMenu->SetSelectionValue( MENU_ITEM_NO_TIMERS, PortSettings::NoTimers() ? 1 : 0 );
        this->SetButtonVisible( BUTTON_ICON_ACCEPT, m_pMenu->GetSelection() == MENU_ITEM_APPLY );
    }

    RefreshMapLabel();
    RefreshCheatLabels();
}

void CGuiScreenCheats::InitRunning()
{
}

void CGuiScreenCheats::InitOutro()
{
    if( m_changed )
    {
        PortSettings::Save();
        m_changed = false;
    }
}
