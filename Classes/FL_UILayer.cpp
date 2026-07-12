#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#include "FL_UILayer.h"
#include "FL_UILayerDelegate.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
    CCSprite* spriteFromFrame(CCSpriteFrameCache* cache, const char* name) {
        CCSpriteFrame* frame = cache->spriteFrameByName(name);
        return frame ? CCSprite::createWithSpriteFrame(frame) : NULL;
    }

    float minimumValue(float a, float b) {
        return a < b ? a : b;
    }

    float maximumValue(float a, float b) {
        return a > b ? a : b;
    }

    float clampValue(float value, float minimum, float maximum) {
        return maximumValue(minimum, minimumValue(maximum, value));
    }

    void animateScale(CCNode* node, float scale, float duration) {
        if (!node) return;
        node->stopAllActions();
        node->runAction(CCScaleTo::create(duration, scale));
    }

    void setPressedVisual(CCSprite* sprite, bool pressed, float baseScale) {
        if (!sprite) return;
        sprite->setOpacity(pressed ? 210 : 255);
        animateScale(sprite, pressed ? baseScale * 0.92f : baseScale, 0.07f);
    }

}

FL_UILayer::FL_UILayer()
    : m_delegate(NULL)
    , m_uiBatch(NULL)
    , m_dpadSprite(NULL)
    , m_dpadIdleFrame(NULL)
    , m_dpadDwnFrame(NULL)
    , m_jumpSprite(NULL)
    , m_jumpIdleFrame(NULL)
    , m_jumpDwnFrame(NULL)
    , m_attackSprite(NULL)
    , m_attackIdleFrame(NULL)
    , m_attackDwnFrame(NULL)
    , m_menuSprite(NULL)
    , m_menuFrame(NULL)
    , m_pauseOverlay(NULL)
    , m_pauseBackground(NULL)
    , m_pauseResumeSprite(NULL)
    , m_pauseRestartSprite(NULL)
    , m_pauseQuitSprite(NULL)
    , m_pauseSoundSprite(NULL)
    , m_pauseUIButton(NULL)
    , m_pauseItemsTab(NULL)
    , m_pauseCharacterTab(NULL)
    , m_pauseQuestsTab(NULL)
    , m_activePauseTab(1)
    , m_pausedVisual(false)
    , m_gameplayUIEnabled(true)
    , m_pauseButtonScale(1.0f)
    , m_pauseSoundScale(0.6f)
    , m_pauseUIScale(1.0f)
    , m_pauseTabScale(1.0f)
    , m_lastLayoutWidth(0.0f)
    , m_lastLayoutHeight(0.0f)
    , m_healthLabel(NULL)
    , m_cachedHealth(-1)
    , m_cachedMaximumHealth(-1) {
    for (int index = 0; index < 5; ++index) m_heartFrames[index] = NULL;
}

FL_UILayer::~FL_UILayer() {
    CC_SAFE_RELEASE_NULL(m_dpadIdleFrame);
    CC_SAFE_RELEASE_NULL(m_dpadDwnFrame);
    CC_SAFE_RELEASE_NULL(m_jumpIdleFrame);
    CC_SAFE_RELEASE_NULL(m_jumpDwnFrame);
    CC_SAFE_RELEASE_NULL(m_attackIdleFrame);
    CC_SAFE_RELEASE_NULL(m_attackDwnFrame);
    CC_SAFE_RELEASE_NULL(m_menuFrame);
    for (int index = 0; index < 5; ++index) CC_SAFE_RELEASE_NULL(m_heartFrames[index]);
}

bool FL_UILayer::init() {
    if (!CCLayer::init()) return false;

    const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    CCSpriteFrameCache* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
    cache->addSpriteFramesWithFile("UISheet-hd.plist");

    m_dpadIdleFrame = cache->spriteFrameByName("Dpad_Btn.png");
    m_dpadDwnFrame = cache->spriteFrameByName("Dpad_Btn_Dwn.png");
    m_attackIdleFrame = cache->spriteFrameByName("Attack_Btn.png");
    m_attackDwnFrame = cache->spriteFrameByName("Attack_Btn_Dwn.png");
    m_jumpIdleFrame = cache->spriteFrameByName("Jump_Btn.png");
    m_jumpDwnFrame = cache->spriteFrameByName("Jump_Btn_Dwn.png");
    m_menuFrame = cache->spriteFrameByName("menuIcon.png");
    m_heartFrames[0] = cache->spriteFrameByName("FLHeart_00_001.png");
    m_heartFrames[1] = cache->spriteFrameByName("FLHeart_01_001.png");
    m_heartFrames[2] = cache->spriteFrameByName("FLHeart_02_001.png");
    m_heartFrames[3] = cache->spriteFrameByName("FLHeart_03_001.png");
    m_heartFrames[4] = cache->spriteFrameByName("FLHeart_04_001.png");

    if (!m_dpadIdleFrame || !m_dpadDwnFrame ||
        !m_attackIdleFrame || !m_attackDwnFrame ||
        !m_jumpIdleFrame || !m_jumpDwnFrame) {
        return false;
    }

    m_dpadIdleFrame->retain();
    m_dpadDwnFrame->retain();
    m_attackIdleFrame->retain();
    m_attackDwnFrame->retain();
    m_jumpIdleFrame->retain();
    m_jumpDwnFrame->retain();
    if (m_menuFrame) m_menuFrame->retain();
    for (int index = 0; index < 5; ++index) {
        if (m_heartFrames[index]) m_heartFrames[index]->retain();
    }

    m_dpadSprite = CCSprite::createWithSpriteFrame(m_dpadIdleFrame);
    m_attackSprite = CCSprite::createWithSpriteFrame(m_attackIdleFrame);
    m_jumpSprite = CCSprite::createWithSpriteFrame(m_jumpIdleFrame);

    addChild(m_dpadSprite, 100);
    addChild(m_attackSprite, 100);
    addChild(m_jumpSprite, 100);

    if (m_menuFrame) {
        m_menuSprite = CCSprite::createWithSpriteFrame(m_menuFrame);
        m_menuSprite->setAnchorPoint(ccp(0.0f, 1.0f));
        addChild(m_menuSprite, 200);
    }

    m_healthLabel = CCLabelTTF::create("", "Arial", 16.0f);
    m_healthLabel->setAnchorPoint(ccp(0.0f, 1.0f));
    m_healthLabel->setVisible(m_heartFrames[4] == NULL);
    addChild(m_healthLabel, 210);

    m_pauseOverlay = CCLayerColor::create(ccc4(0, 0, 0, 200), winSize.width, winSize.height);
    m_pauseOverlay->setVisible(false);
    addChild(m_pauseOverlay, 1000);

    m_pauseBackground = spriteFromFrame(cache, "interface_bg.png");
    if (m_pauseBackground) {
        m_pauseBackground->setAnchorPoint(ccp(0.5f, 1.0f));
        m_pauseOverlay->addChild(m_pauseBackground, 1);
    }

    m_pauseResumeSprite = spriteFromFrame(cache, "Play_Btn.png");
    m_pauseRestartSprite = spriteFromFrame(cache, "Replay_Btn.png");
    m_pauseQuitSprite = spriteFromFrame(cache, "Menu_Btn.png");
    m_pauseSoundSprite = spriteFromFrame(cache, "Sound_Btn.png");

    if (m_pauseResumeSprite) m_pauseOverlay->addChild(m_pauseResumeSprite, 3);
    if (m_pauseRestartSprite) m_pauseOverlay->addChild(m_pauseRestartSprite, 3);
    if (m_pauseQuitSprite) m_pauseOverlay->addChild(m_pauseQuitSprite, 3);
    if (m_pauseSoundSprite) m_pauseOverlay->addChild(m_pauseSoundSprite, 3);

    m_pauseUIButton = CCLabelBMFont::create("UI", "bigFont.fnt");
    if (m_pauseUIButton) {
        m_pauseUIButton->setAnchorPoint(ccp(0.5f, 0.5f));
        m_pauseOverlay->addChild(m_pauseUIButton, 3);
    }

    m_pauseItemsTab = spriteFromFrame(cache, "interface_tab_items_selected.png");
    m_pauseCharacterTab = spriteFromFrame(cache, "interface_tab_character_deselected.png");
    m_pauseQuestsTab = spriteFromFrame(cache, "interface_tab_quests_deselected.png");

    if (m_pauseItemsTab) {
        m_pauseItemsTab->setAnchorPoint(ccp(0.0f, 0.0f));
        m_pauseOverlay->addChild(m_pauseItemsTab, 2);
    }
    if (m_pauseCharacterTab) {
        m_pauseCharacterTab->setAnchorPoint(ccp(0.5f, 0.0f));
        m_pauseOverlay->addChild(m_pauseCharacterTab, 2);
    }
    if (m_pauseQuestsTab) {
        m_pauseQuestsTab->setAnchorPoint(ccp(1.0f, 0.0f));
        m_pauseOverlay->addChild(m_pauseQuestsTab, 2);
    }

    rebuildHealthView(12);
    layoutUI();
    refreshSoundView();
    refreshUIButton();
    setTouchEnabled(true);
    return true;
}

void FL_UILayer::setDelegate(FL_UILayerDelegate* delegate) {
    m_delegate = delegate;
    m_cachedHealth = -1;
    m_cachedMaximumHealth = -1;
    refreshHealthView();
    refreshSoundView();
}

void FL_UILayer::onEnter() {
    CCLayer::onEnter();
    layoutUI();
    scheduleUpdate();
}

void FL_UILayer::onExit() {
    unscheduleUpdate();
    CCLayer::onExit();
}

void FL_UILayer::update(float) {
    const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    if (std::fabs(winSize.width - m_lastLayoutWidth) > 0.5f ||
        std::fabs(winSize.height - m_lastLayoutHeight) > 0.5f) {
        layoutUI();
    }
    refreshHealthView();
}

void FL_UILayer::registerWithTouchDispatcher() {
    CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, 0, true);
}

void FL_UILayer::layoutUI() {
    const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
    m_lastLayoutWidth = winSize.width;
    m_lastLayoutHeight = winSize.height;

    const float edge = clampValue(minimumValue(winSize.width, winSize.height) * 0.02f, 8.0f, 18.0f);
    const float controlGap = clampValue(winSize.width * 0.025f, 10.0f, 20.0f);

    if (m_dpadSprite) {
        m_dpadSprite->setScale(1.0f);
        m_dpadSprite->setPosition(ccp(
            edge + m_dpadSprite->getContentSize().width * 0.5f,
            edge + m_dpadSprite->getContentSize().height * 0.5f
        ));
    }

    if (m_jumpSprite) {
        m_jumpSprite->setScale(1.0f);
        m_jumpSprite->setPosition(ccp(
            winSize.width - edge - m_jumpSprite->getContentSize().width * 0.5f,
            edge + m_jumpSprite->getContentSize().height * 0.5f
        ));
    }

    if (m_attackSprite && m_jumpSprite) {
        m_attackSprite->setScale(1.0f);
        m_attackSprite->setPosition(ccp(
            m_jumpSprite->getPositionX()
                - m_jumpSprite->getContentSize().width * 0.5f
                - controlGap
                - m_attackSprite->getContentSize().width * 0.5f,
            edge + m_attackSprite->getContentSize().height * 0.5f
        ));
    }

    if (m_menuSprite) {
        m_menuSprite->setPosition(ccp(edge, winSize.height - edge));
    }

    const float heartScale = 0.8f;
    float healthStartX = edge;
    if (m_menuSprite) {
        healthStartX += m_menuSprite->getContentSize().width * m_menuSprite->getScaleX() + 8.0f;
    }

    float heartWidth = 52.0f * heartScale;
    float heartHeight = 40.0f * heartScale;
    if (!m_healthSprites.empty() && m_healthSprites[0]) {
        heartWidth = m_healthSprites[0]->getContentSize().width * heartScale;
        heartHeight = m_healthSprites[0]->getContentSize().height * heartScale;
    }

    const float heartGap = 2.0f;
    const float rowGap = 3.0f;
    const float healthAvailableWidth = maximumValue(
        heartWidth,
        winSize.width - edge - healthStartX
    );
    const int fittingColumns = std::max(
        1,
        static_cast<int>((healthAvailableWidth + heartGap) / (heartWidth + heartGap))
    );
    const int healthColumns = std::min(5, fittingColumns);

    for (unsigned int index = 0; index < m_healthSprites.size(); ++index) {
        CCSprite* heart = m_healthSprites[index];
        if (!heart) continue;
        const int column = static_cast<int>(index) % healthColumns;
        const int row = static_cast<int>(index) / healthColumns;
        heart->setAnchorPoint(ccp(0.5f, 0.5f));
        heart->setScale(heartScale);
        heart->setPosition(ccp(
            healthStartX + heartWidth * 0.5f + column * (heartWidth + heartGap),
            winSize.height - edge - heartHeight * 0.5f - row * (heartHeight + rowGap)
        ));
    }

    if (m_healthLabel) {
        m_healthLabel->setPosition(ccp(healthStartX, winSize.height - edge));
    }

    if (!m_pauseOverlay) return;
    m_pauseOverlay->setContentSize(winSize);

    CCSprite* pauseButtons[3] = {
        m_pauseResumeSprite,
        m_pauseRestartSprite,
        m_pauseQuitSprite
    };

    float buttonNativeWidth = 0.0f;
    float buttonNativeHeight = 0.0f;
    int buttonCount = 0;
    for (int index = 0; index < 3; ++index) {
        if (!pauseButtons[index]) continue;
        buttonNativeWidth += pauseButtons[index]->getContentSize().width;
        buttonNativeHeight = maximumValue(buttonNativeHeight, pauseButtons[index]->getContentSize().height);
        ++buttonCount;
    }

    const float nativeButtonGap = 24.0f;
    if (buttonCount > 1) buttonNativeWidth += nativeButtonGap * (buttonCount - 1);

    m_pauseSoundScale = 0.55f;
    float soundWidth = 0.0f;
    float soundHeight = 0.0f;
    if (m_pauseSoundSprite) {
        soundWidth = m_pauseSoundSprite->getContentSize().width * m_pauseSoundScale;
        soundHeight = m_pauseSoundSprite->getContentSize().height * m_pauseSoundScale;
    }

    m_pauseUIScale = 1.0f;
    float uiWidth = 0.0f;
    float uiHeight = 0.0f;
    if (m_pauseUIButton) {
        const CCSize uiSize = m_pauseUIButton->getContentSize();
        if (uiSize.height > 0.0f) {
            m_pauseUIScale = minimumValue(1.0f, 34.0f / uiSize.height);
        }
        uiWidth = uiSize.width * m_pauseUIScale;
        uiHeight = uiSize.height * m_pauseUIScale;
    }

    const float sideSpacing = 10.0f;
    const float centerWidth = maximumValue(
        80.0f,
        winSize.width - edge * 2.0f - soundWidth - uiWidth - sideSpacing * 2.0f
    );
    m_pauseButtonScale = buttonNativeWidth > 0.0f
        ? clampValue(centerWidth / buttonNativeWidth, 0.32f, 0.9f)
        : 1.0f;

    float bottomRowHeight = maximumValue(
        buttonNativeHeight * m_pauseButtonScale,
        maximumValue(soundHeight, uiHeight)
    );

    const float backgroundNativeWidth = m_pauseBackground
        ? m_pauseBackground->getContentSize().width
        : 934.0f;
    const float backgroundNativeHeight = m_pauseBackground
        ? m_pauseBackground->getContentSize().height
        : 467.0f;
    const float tabNativeHeight = 60.0f;
    const float tabOverlap = 9.0f;
    const float topGroupNativeHeight = backgroundNativeHeight + tabNativeHeight - tabOverlap;
    const float verticalGap = clampValue(winSize.height * 0.012f, 5.0f, 10.0f);

    float bottomRowTop = edge + bottomRowHeight;
    float topAvailableHeight = maximumValue(
        1.0f,
        winSize.height - edge - bottomRowTop - verticalGap
    );
    const float widthScale = backgroundNativeWidth > 0.0f
        ? (winSize.width - edge * 2.0f) / backgroundNativeWidth
        : 1.0f;
    const float heightScale = topGroupNativeHeight > 0.0f
        ? topAvailableHeight / topGroupNativeHeight
        : 1.0f;
    m_pauseTabScale = clampValue(minimumValue(1.0f, minimumValue(widthScale, heightScale)), 0.22f, 1.0f);

    const float minimumBottomScale = clampValue(
        (winSize.height - edge * 2.0f - verticalGap - topGroupNativeHeight * m_pauseTabScale)
            / maximumValue(1.0f, buttonNativeHeight),
        0.25f,
        m_pauseButtonScale
    );
    if (minimumBottomScale < m_pauseButtonScale) {
        m_pauseButtonScale = minimumBottomScale;
        bottomRowHeight = maximumValue(
            buttonNativeHeight * m_pauseButtonScale,
            maximumValue(soundHeight, uiHeight)
        );
        bottomRowTop = edge + bottomRowHeight;
        topAvailableHeight = maximumValue(
            1.0f,
            winSize.height - edge - bottomRowTop - verticalGap
        );
        m_pauseTabScale = clampValue(
            minimumValue(1.0f, minimumValue(widthScale, topAvailableHeight / topGroupNativeHeight)),
            0.22f,
            1.0f
        );
    }

    const float buttonY = edge + bottomRowHeight * 0.5f;
    if (m_pauseSoundSprite) {
        m_pauseSoundSprite->setScale(m_pauseSoundScale);
        m_pauseSoundSprite->setPosition(ccp(edge + soundWidth * 0.5f, buttonY));
    }

    if (m_pauseUIButton) {
        m_pauseUIButton->setScale(m_pauseUIScale);
        m_pauseUIButton->setPosition(ccp(
            winSize.width - edge - uiWidth * 0.5f,
            buttonY
        ));
    }

    const float scaledButtonsWidth = buttonNativeWidth * m_pauseButtonScale;
    float buttonX = winSize.width * 0.5f - scaledButtonsWidth * 0.5f;
    for (int index = 0; index < 3; ++index) {
        CCSprite* button = pauseButtons[index];
        if (!button) continue;
        button->setScale(m_pauseButtonScale);
        const float width = button->getContentSize().width * m_pauseButtonScale;
        buttonX += width * 0.5f;
        button->setPosition(ccp(buttonX, buttonY));
        buttonX += width * 0.5f + nativeButtonGap * m_pauseButtonScale;
    }

    const float backgroundBottom = edge + bottomRowHeight + verticalGap;
    const float backgroundTop = backgroundBottom + backgroundNativeHeight * m_pauseTabScale;
    const float backgroundWidth = backgroundNativeWidth * m_pauseTabScale;

    if (m_pauseBackground) {
        m_pauseBackground->setScale(m_pauseTabScale);
        m_pauseBackground->setPosition(ccp(winSize.width * 0.5f, backgroundTop));
    }

    const float backgroundLeft = winSize.width * 0.5f - backgroundWidth * 0.5f;
    const float backgroundRight = winSize.width * 0.5f + backgroundWidth * 0.5f;
    const float baseTabY = backgroundTop - tabOverlap * m_pauseTabScale;
    const float itemsYOffset = m_activePauseTab == 1 ? 0.0f : 6.0f * m_pauseTabScale;
    const float characterYOffset = m_activePauseTab == 2 ? 0.0f : 6.0f * m_pauseTabScale;
    const float questsYOffset = m_activePauseTab == 3 ? 0.0f : 5.0f * m_pauseTabScale;

    if (m_pauseItemsTab) {
        m_pauseItemsTab->setScale(m_pauseTabScale);
        m_pauseItemsTab->setPosition(ccp(
            backgroundLeft + 10.0f * m_pauseTabScale,
            baseTabY - itemsYOffset
        ));
    }
    if (m_pauseCharacterTab) {
        m_pauseCharacterTab->setScale(m_pauseTabScale);
        m_pauseCharacterTab->setPosition(ccp(
            winSize.width * 0.5f,
            baseTabY - characterYOffset
        ));
    }
    if (m_pauseQuestsTab) {
        m_pauseQuestsTab->setScale(m_pauseTabScale);
        m_pauseQuestsTab->setPosition(ccp(
            backgroundRight - 10.0f * m_pauseTabScale,
            baseTabY - questsYOffset
        ));
    }
}

void FL_UILayer::setGameplayVisible(bool visible) {
    const bool controlsVisible = visible && m_gameplayUIEnabled;
    if (m_dpadSprite) m_dpadSprite->setVisible(controlsVisible);
    if (m_attackSprite) m_attackSprite->setVisible(controlsVisible);
    if (m_jumpSprite) m_jumpSprite->setVisible(controlsVisible);
    if (m_menuSprite) m_menuSprite->setVisible(controlsVisible);

    for (std::vector<CCSprite*>::iterator it = m_healthSprites.begin();
        it != m_healthSprites.end(); ++it) {
        if (*it) (*it)->setVisible(controlsVisible);
    }

    if (m_healthLabel) {
        m_healthLabel->setVisible(controlsVisible && m_heartFrames[4] == NULL);
    }
}

void FL_UILayer::setPauseVisible(bool visible) {
    m_pausedVisual = visible;
    if (visible) layoutUI();
    if (m_pauseOverlay) m_pauseOverlay->setVisible(visible);
    setGameplayVisible(!visible);
    if (!visible) {
        m_cachedHealth = -1;
        refreshHealthView();
    }
    refreshSoundView();
    refreshUIButton();
}

void FL_UILayer::setPauseTab(int tab) {
    if (tab < 1 || tab > 3) return;
    m_activePauseTab = tab;

    CCSpriteFrameCache* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
    CCSpriteFrame* itemsFrame = cache->spriteFrameByName(
        tab == 1 ? "interface_tab_items_selected.png" : "interface_tab_items_deselected.png"
    );
    CCSpriteFrame* characterFrame = cache->spriteFrameByName(
        tab == 2 ? "interface_tab_character_selected.png" : "interface_tab_character_deselected.png"
    );
    CCSpriteFrame* questsFrame = cache->spriteFrameByName(
        tab == 3 ? "interface_tab_quests_selected.png" : "interface_tab_quests_deselected.png"
    );

    if (m_pauseItemsTab && itemsFrame) m_pauseItemsTab->setDisplayFrame(itemsFrame);
    if (m_pauseCharacterTab && characterFrame) m_pauseCharacterTab->setDisplayFrame(characterFrame);
    if (m_pauseQuestsTab && questsFrame) m_pauseQuestsTab->setDisplayFrame(questsFrame);

    layoutUI();
}

void FL_UILayer::refreshSoundView() {
    if (!m_pauseSoundSprite) return;
    const bool enabled = !m_delegate || m_delegate->uiSoundEnabled();
    m_pauseSoundSprite->setOpacity(enabled ? 255 : 105);
}

void FL_UILayer::refreshUIButton() {
    if (!m_pauseUIButton) return;
    m_pauseUIButton->setString("UI");
    m_pauseUIButton->setOpacity(m_gameplayUIEnabled ? 255 : 120);
}

void FL_UILayer::rebuildHealthView(int maximumHealth) {
    for (std::vector<CCSprite*>::iterator it = m_healthSprites.begin();
        it != m_healthSprites.end(); ++it) {
        if (*it) (*it)->removeFromParentAndCleanup(true);
    }
    m_healthSprites.clear();

    if (!m_heartFrames[4]) {
        if (m_healthLabel) m_healthLabel->setVisible(!m_pausedVisual && m_gameplayUIEnabled);
        return;
    }

    const int heartCount = std::max(1, (maximumHealth + 3) / 4);
    for (int index = 0; index < heartCount; ++index) {
        CCSprite* heart = CCSprite::createWithSpriteFrame(m_heartFrames[4]);
        if (!heart) continue;
        heart->setAnchorPoint(ccp(0.5f, 0.5f));
        heart->setScale(0.8f);
        heart->setOpacity(255);
        addChild(heart, 200);
        m_healthSprites.push_back(heart);
    }
    layoutUI();
}

void FL_UILayer::refreshHealthView() {
    const int maximumHealth = m_delegate ? std::max(1, m_delegate->uiMaximumHealth()) : 12;
    const int health = m_delegate
        ? std::max(0, std::min(maximumHealth, m_delegate->uiCurrentHealth()))
        : maximumHealth;

    if (maximumHealth != m_cachedMaximumHealth) {
        rebuildHealthView(maximumHealth);
        m_cachedMaximumHealth = maximumHealth;
        m_cachedHealth = -1;
    }
    if (health == m_cachedHealth && !m_pausedVisual) return;

    for (unsigned int index = 0; index < m_healthSprites.size(); ++index) {
        const int value = std::max(0, std::min(4, health - static_cast<int>(index) * 4));
        CCSprite* heart = m_healthSprites[index];
        if (!heart) continue;
        if (m_heartFrames[value]) heart->setDisplayFrame(m_heartFrames[value]);
        heart->setOpacity(255);
        heart->setVisible(!m_pausedVisual && m_gameplayUIEnabled);
    }

    if (m_healthLabel) {
        char text[32];
#if defined(_MSC_VER) && _MSC_VER < 1900
        _snprintf(text, sizeof(text), "HP %d/%d", health, maximumHealth);
        text[sizeof(text) - 1] = '\0';
#else
        std::snprintf(text, sizeof(text), "HP %d/%d", health, maximumHealth);
#endif
        m_healthLabel->setString(text);
        m_healthLabel->setVisible(
            !m_pausedVisual && m_gameplayUIEnabled && m_heartFrames[4] == NULL
        );
    }

    m_cachedHealth = health;
}

FL_UILayer::Direction FL_UILayer::hitTest(const CCPoint& loc) {
    if (m_pausedVisual) {
        if (m_pauseResumeSprite && m_pauseResumeSprite->boundingBox().containsPoint(loc)) return DIR_PAUSE_RESUME;
        if (m_pauseRestartSprite && m_pauseRestartSprite->boundingBox().containsPoint(loc)) return DIR_PAUSE_RESTART;
        if (m_pauseQuitSprite && m_pauseQuitSprite->boundingBox().containsPoint(loc)) return DIR_PAUSE_QUIT;
        if (m_pauseSoundSprite && m_pauseSoundSprite->boundingBox().containsPoint(loc)) return DIR_PAUSE_SOUND;
        if (m_pauseUIButton && m_pauseUIButton->boundingBox().containsPoint(loc)) return DIR_PAUSE_UI;
        if (m_pauseItemsTab && m_pauseItemsTab->boundingBox().containsPoint(loc)) return DIR_PAUSE_TAB_ITEMS;
        if (m_pauseCharacterTab && m_pauseCharacterTab->boundingBox().containsPoint(loc)) return DIR_PAUSE_TAB_CHARACTER;
        if (m_pauseQuestsTab && m_pauseQuestsTab->boundingBox().containsPoint(loc)) return DIR_PAUSE_TAB_QUESTS;
        return DIR_NONE;
    }

    if (!m_gameplayUIEnabled) return DIR_NONE;

    if (m_menuSprite && m_menuSprite->isVisible() && m_menuSprite->boundingBox().containsPoint(loc)) return DIR_MENU;

    if (m_dpadSprite && m_dpadSprite->isVisible()) {
        const CCSize dpadSize = m_dpadSprite->getContentSize();
        const CCPoint dpadPos = m_dpadSprite->getPosition();
        const CCRect dpadRect(
            dpadPos.x - dpadSize.width * 0.5f,
            dpadPos.y - dpadSize.height * 0.5f,
            dpadSize.width,
            dpadSize.height
        );
        if (dpadRect.containsPoint(loc)) return loc.x < dpadPos.x ? DIR_LEFT : DIR_RIGHT;
    }

    if (m_jumpSprite && m_jumpSprite->isVisible() && m_jumpSprite->boundingBox().containsPoint(loc)) return DIR_UP;
    if (m_attackSprite && m_attackSprite->isVisible() && m_attackSprite->boundingBox().containsPoint(loc)) return DIR_DOWN;

    // Pseudocode ControlLayer sends pushButton:7 when the touch is not on any
    // gameplay button.  Player::pushButton: handles button 7 by switchStance.
    return DIR_STANCE;
}

void FL_UILayer::pressDirection(Direction dir) {
    switch (dir) {
    case DIR_LEFT:
        m_dpadSprite->setDisplayFrame(m_dpadDwnFrame);
        m_dpadSprite->setFlipX(false);
        if (m_delegate) m_delegate->uiLeftPressed();
        break;
    case DIR_RIGHT:
        m_dpadSprite->setDisplayFrame(m_dpadDwnFrame);
        m_dpadSprite->setFlipX(true);
        if (m_delegate) m_delegate->uiRightPressed();
        break;
    case DIR_UP:
        m_jumpSprite->setDisplayFrame(m_jumpDwnFrame);
        if (m_delegate) m_delegate->uiUpPressed();
        break;
    case DIR_DOWN:
        m_attackSprite->setDisplayFrame(m_attackDwnFrame);
        if (m_delegate) m_delegate->uiDownPressed();
        break;
    case DIR_MENU:
        setPressedVisual(m_menuSprite, true, 1.0f);
        setPauseVisible(true);
        if (m_delegate) m_delegate->uiMenuPressed();
        break;
    case DIR_STANCE:
        if (m_delegate) m_delegate->uiStancePressed();
        break;
    case DIR_PAUSE_RESUME:
        setPressedVisual(m_pauseResumeSprite, true, m_pauseButtonScale);
        break;
    case DIR_PAUSE_RESTART:
        setPressedVisual(m_pauseRestartSprite, true, m_pauseButtonScale);
        break;
    case DIR_PAUSE_QUIT:
        setPressedVisual(m_pauseQuitSprite, true, m_pauseButtonScale);
        break;
    case DIR_PAUSE_SOUND:
        setPressedVisual(m_pauseSoundSprite, true, m_pauseSoundScale);
        break;
    case DIR_PAUSE_UI:
        animateScale(m_pauseUIButton, m_pauseUIScale * 0.92f, 0.07f);
        break;
    case DIR_PAUSE_TAB_ITEMS:
    case DIR_PAUSE_TAB_CHARACTER:
    case DIR_PAUSE_TAB_QUESTS:
        break;
    default:
        break;
    }
}

void FL_UILayer::releaseDirection(Direction dir, bool activate) {
    switch (dir) {
    case DIR_LEFT:
        m_dpadSprite->setDisplayFrame(m_dpadIdleFrame);
        m_dpadSprite->setFlipX(false);
        if (m_delegate) m_delegate->uiLeftReleased();
        break;
    case DIR_RIGHT:
        m_dpadSprite->setDisplayFrame(m_dpadIdleFrame);
        m_dpadSprite->setFlipX(false);
        if (m_delegate) m_delegate->uiRightReleased();
        break;
    case DIR_UP:
        m_jumpSprite->setDisplayFrame(m_jumpIdleFrame);
        if (m_delegate) m_delegate->uiUpReleased();
        break;
    case DIR_DOWN:
        m_attackSprite->setDisplayFrame(m_attackIdleFrame);
        if (m_delegate) m_delegate->uiDownReleased();
        break;
    case DIR_MENU:
        setPressedVisual(m_menuSprite, false, 1.0f);
        break;
    case DIR_STANCE:
        break;
    case DIR_PAUSE_RESUME:
        setPressedVisual(m_pauseResumeSprite, false, m_pauseButtonScale);
        if (activate) {
            setPauseVisible(false);
            if (m_delegate) m_delegate->uiMenuPressed();
        }
        break;
    case DIR_PAUSE_RESTART:
        setPressedVisual(m_pauseRestartSprite, false, m_pauseButtonScale);
        if (activate && m_delegate) m_delegate->uiPauseRestartPressed();
        break;
    case DIR_PAUSE_QUIT:
        setPressedVisual(m_pauseQuitSprite, false, m_pauseButtonScale);
        if (activate && m_delegate) m_delegate->uiPauseQuitPressed();
        break;
    case DIR_PAUSE_SOUND:
        setPressedVisual(m_pauseSoundSprite, false, m_pauseSoundScale);
        if (activate && m_delegate) m_delegate->uiPauseSoundPressed();
        refreshSoundView();
        break;
    case DIR_PAUSE_UI:
        animateScale(m_pauseUIButton, m_pauseUIScale, 0.07f);
        if (activate) {
            m_gameplayUIEnabled = !m_gameplayUIEnabled;
            setGameplayVisible(!m_pausedVisual);
            refreshUIButton();
        }
        break;
    case DIR_PAUSE_TAB_ITEMS:
        if (activate) setPauseTab(1);
        break;
    case DIR_PAUSE_TAB_CHARACTER:
        if (activate) setPauseTab(2);
        break;
    case DIR_PAUSE_TAB_QUESTS:
        if (activate) setPauseTab(3);
        break;
    default:
        break;
    }
}

bool FL_UILayer::ccTouchBegan(CCTouch* touch, CCEvent*) {
    const CCPoint loc = convertTouchToNodeSpace(touch);
    const Direction dir = hitTest(loc);
    if (dir == DIR_NONE) return false;

    pressDirection(dir);
    m_activeTouches[touch] = dir;
    return true;
}

void FL_UILayer::ccTouchMoved(CCTouch* touch, CCEvent*) {
    std::map<CCTouch*, Direction>::iterator it = m_activeTouches.find(touch);
    if (it == m_activeTouches.end()) return;
    if (it->second != DIR_LEFT && it->second != DIR_RIGHT) return;

    const CCPoint loc = convertTouchToNodeSpace(touch);
    const Direction newDir = hitTest(loc);
    if (newDir != it->second && (newDir == DIR_LEFT || newDir == DIR_RIGHT)) {
        releaseDirection(it->second, false);
        pressDirection(newDir);
        it->second = newDir;
    }
}

void FL_UILayer::ccTouchEnded(CCTouch* touch, CCEvent*) {
    std::map<CCTouch*, Direction>::iterator it = m_activeTouches.find(touch);
    if (it == m_activeTouches.end()) return;

    const Direction direction = it->second;
    const Direction releasedOver = hitTest(convertTouchToNodeSpace(touch));
    m_activeTouches.erase(it);
    releaseDirection(direction, releasedOver == direction);
}

void FL_UILayer::ccTouchCancelled(CCTouch* touch, CCEvent*) {
    std::map<CCTouch*, Direction>::iterator it = m_activeTouches.find(touch);
    if (it == m_activeTouches.end()) return;
    const Direction direction = it->second;
    m_activeTouches.erase(it);
    releaseDirection(direction, false);
}
