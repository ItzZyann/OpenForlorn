#pragma once
#include "incl.h"

#include <map>
#include <vector>

class FL_UILayerDelegate;

class FL_UILayer : public CCLayer {
public:
    FL_UILayer();
    virtual ~FL_UILayer();

    virtual bool init();
    void setDelegate(FL_UILayerDelegate* delegate);

    virtual void onEnter();
    virtual void onExit();
    virtual void update(float dt);

    virtual void registerWithTouchDispatcher();
    virtual bool ccTouchBegan(CCTouch* touch, CCEvent* event);
    virtual void ccTouchMoved(CCTouch* touch, CCEvent* event);
    virtual void ccTouchEnded(CCTouch* touch, CCEvent* event);
    virtual void ccTouchCancelled(CCTouch* touch, CCEvent* event);

    void togglePauseFromKeyboard();

    CREATE_FUNC(FL_UILayer);

private:
    enum Direction {
        DIR_NONE = 0,
        DIR_LEFT,
        DIR_RIGHT,
        DIR_UP,
        DIR_DOWN,
        DIR_MENU,
        DIR_STANCE,
        DIR_PAUSE_RESUME,
        DIR_PAUSE_RESTART,
        DIR_PAUSE_QUIT,
        DIR_PAUSE_SOUND,
        DIR_PAUSE_UI,
        DIR_PAUSE_TAB_ITEMS,
        DIR_PAUSE_TAB_CHARACTER,
        DIR_PAUSE_TAB_QUESTS
    };

    Direction hitTest(const CCPoint& loc);
    void pressDirection(Direction dir);
    void releaseDirection(Direction dir, bool activate);
    void layoutUI();
    void setGameplayVisible(bool visible);
    void setPauseVisible(bool visible);
    void setPauseTab(int tab);
    void refreshSoundView();
    void refreshUIButton();
    void rebuildHealthView(int maximumHealth);
    void refreshHealthView();

    FL_UILayerDelegate* m_delegate;
    CCSpriteBatchNode* m_uiBatch;

    CCSprite* m_dpadSprite;
    CCSpriteFrame* m_dpadIdleFrame;
    CCSpriteFrame* m_dpadDwnFrame;

    CCSprite* m_jumpSprite;
    CCSpriteFrame* m_jumpIdleFrame;
    CCSpriteFrame* m_jumpDwnFrame;

    CCSprite* m_attackSprite;
    CCSpriteFrame* m_attackIdleFrame;
    CCSpriteFrame* m_attackDwnFrame;

    CCSprite* m_menuSprite;
    CCSpriteFrame* m_menuFrame;

    CCLayerColor* m_pauseOverlay;
    CCSprite* m_pauseBackground;
    CCSprite* m_pauseResumeSprite;
    CCSprite* m_pauseRestartSprite;
    CCSprite* m_pauseQuitSprite;
    CCSprite* m_pauseSoundSprite;
    CCLabelBMFont* m_pauseUIButton;
    CCSprite* m_pauseItemsTab;
    CCSprite* m_pauseCharacterTab;
    CCSprite* m_pauseQuestsTab;
    int m_activePauseTab;
    bool m_pausedVisual;
    bool m_gameplayUIEnabled;
    float m_pauseButtonScale;
    float m_pauseSoundScale;
    float m_pauseUIScale;
    float m_pauseTabScale;
    float m_lastLayoutWidth;
    float m_lastLayoutHeight;

    std::vector<CCSprite*> m_healthSprites;
    CCSpriteFrame* m_heartFrames[5];
    CCLabelTTF* m_healthLabel;
    int m_cachedHealth;
    int m_cachedMaximumHealth;

    std::map<CCTouch*, Direction> m_activeTouches;
};
