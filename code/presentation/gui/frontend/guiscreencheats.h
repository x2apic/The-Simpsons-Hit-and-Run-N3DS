#ifndef GUISCREENCHEATS_H
#define GUISCREENCHEATS_H

#include <presentation/gui/guiscreen.h>

class CGuiMenu;

namespace Scrooby
{
    class Text;
}

class CGuiScreenCheats : public CGuiScreen
{
public:
    CGuiScreenCheats( Scrooby::Screen* pScreen, CGuiEntity* pParent, bool isInGame );
    virtual ~CGuiScreenCheats();

	virtual void HandleMessage( eGuiMessage message,
			                    unsigned int param1 = 0,
								unsigned int param2 = 0 );

    virtual CGuiMenu* HasMenu() { return m_pMenu; }

protected:
    void InitIntro();
	void InitRunning();
	void InitOutro();

private:
    enum eMenuItem
    {
        MENU_ITEM_NO_TIMERS,
        MENU_ITEM_MAP,
        MENU_ITEM_CHEAT,
        MENU_ITEM_APPLY,

        NUM_MENU_ITEMS
    };

    void StepCheat( int direction );
    void ApplyCheat();
    void RefreshCheatLabels();
    void RefreshMapLabel();

    CGuiMenu* m_pMenu;
    Scrooby::Text* m_mapValue;
    Scrooby::Text* m_cheatValue;
    Scrooby::Text* m_applyText;
    int m_cheatIndex;
    bool m_isInGame;
    bool m_changed;
};
#endif
