#include "FL_MenuLayer.h"
#include "FL_PlayLayer.h"

#include <string>

namespace {
const char* const kLevelFiles[] = {
    "Level001.plist",
    "Level002.plist",
    "Level003.plist",
    "Level004.plist",
    "Level005.plist",
    "LevelCave.plist"
};

CCMenuItemLabel* createLevelItem(const char* fileName, CCObject* target, SEL_MenuHandler selector, int tag) {
    CCLabelBMFont* normal = CCLabelBMFont::create(fileName, "bigFont.fnt");
    normal->setScale(0.48f);
    CCMenuItemLabel* item = CCMenuItemLabel::create(normal, target, selector);
    item->setTag(tag);
    return item;
}
}

bool FL_MenuLayer::init() {
    if (!CCLayer::init()) return false;

    setKeypadEnabled(true);

    const CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    // Scale UI offsets relative to the 960x520 design size so the layout
    // looks identical on Windows and Mac regardless of actual window size.
    const float scaleX = winSize.width  / 960.0f;
    const float scaleY = winSize.height / 520.0f;

    // Render the cave level as a non-interactive animated menu background.
    FL_PlayLayer::Args previewArgs;
    previewArgs.levelFile = "LevelCave.plist";
    previewArgs.previewMode = true;
    previewArgs.initialZoom = 1.5f;

    FL_PlayLayer* cavePreview = FL_PlayLayer::create(previewArgs);
    if (cavePreview) {
        addChild(cavePreview, -1000000);
        cavePreview->attachFixedBackground(this, -2000000);
    }

    CCSprite* logo = CCSprite::create("forlorn_logo_menu.png");
    if (logo) {
        logo->setScale(0.48f);
        logo->setPosition(ccp(winSize.width / 2.0f, winSize.height - 75.0f * scaleY));
        addChild(logo);
    }

    CCLabelBMFont* title = CCLabelBMFont::create("SELECT LEVEL", "bigFont.fnt");
    title->setScale(0.55f);
    title->setColor(ccc3(255, 220, 80));
    title->setPosition(ccp(winSize.width / 2.0f, winSize.height - 135.0f * scaleY));
    addChild(title);

    CCMenu* menu = CCMenu::create();
    menu->setPosition(CCPointZero);
    addChild(menu);

    const float startY  = winSize.height - 185.0f * scaleY;
    const float spacing = 38.0f * scaleY;
    for (int index = 0; index < 6; ++index) {
        CCMenuItemLabel* item = createLevelItem(
            kLevelFiles[index], this, menu_selector(FL_MenuLayer::onSelectLevel), index
        );
        item->setPosition(ccp(winSize.width / 2.0f, startY - spacing * index));
        menu->addChild(item);
    }

    CCLabelBMFont* exitLabel = CCLabelBMFont::create("EXIT", "bigFont.fnt");
    exitLabel->setScale(0.48f);
    exitLabel->setColor(ccc3(255, 90, 90));
    CCMenuItemLabel* exitItem = CCMenuItemLabel::create(
        exitLabel, this, menu_selector(FL_MenuLayer::onExitGame)
    );
    exitItem->setPosition(ccp(winSize.width / 2.0f, startY - spacing * 6 - 8.0f * scaleY));
    menu->addChild(exitItem);

    return true;
}

void FL_MenuLayer::keyBackClicked() {
    CCDirector::sharedDirector()->end();
}

void FL_MenuLayer::onExitGame(CCObject*) {
    keyBackClicked();
}

void FL_MenuLayer::onSelectLevel(CCObject* sender) {
    CCNode* node = dynamic_cast<CCNode*>(sender);
    if (!node) return;

    const int index = node->getTag();
    if (index < 0 || index >= 6) return;

    FL_PlayLayer::Args args;
    args.levelFile = kLevelFiles[index];

    CCScene* gameScene = FL_PlayLayer::scene(args);
    if (gameScene) {
        CCDirector::sharedDirector()->replaceScene(
            CCTransitionFade::create(0.35f, gameScene)
        );
    }
}
