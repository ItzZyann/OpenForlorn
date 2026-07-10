#include "FL_MenuLayer.h"
#include "FL_PlayLayer.h"
#include "CCMenuItemSpriteExtra.h"

bool FL_MenuLayer::init() {
	if (!CCLayer::init()) return false;

	setKeypadEnabled(true);

	auto dir = CCDirector::sharedDirector();
	auto winSize = dir->getWinSize();

	auto logo = CCSprite::create("forlorn_logo_menu.png");
	logo->setScale(0.6f);
	logo->setPosition(ccp(
		winSize.width / 2.0f,
		winSize.height - 120.0f
	));
	addChild(logo);

	// play button
	// i dont have button sprite yet
	auto btnMenu = CCMenu::create();
	btnMenu->setPosition(ccp(winSize.width / 2.0f, winSize.height / 2.0f));
	addChild(btnMenu);

	auto play_on = CCLabelBMFont::create("PLAY", "bigFont.fnt");
	play_on->setColor({ 255, 255, 0 });
	auto play_off = CCLabelBMFont::create("PLAY", "bigFont.fnt");

	auto playBtn = CCMenuItemSprite::create(play_off, play_on, this, menu_selector(FL_MenuLayer::onPlay));
	playBtn->setPosition(btnMenu->convertToNodeSpace({
		winSize.width / 2.0f,
		winSize.height / 2.0f - 40.0f
	}));

	// exit btn
	auto exit_on = CCLabelBMFont::create("EXIT", "bigFont.fnt");
	exit_on->setColor({ 255, 0, 0 });
	auto exit_off = CCLabelBMFont::create("EXIT", "bigFont.fnt");

	auto exitBtn = CCMenuItemSprite::create(exit_off, exit_on, this, menu_selector(FL_MenuLayer::onExitGame));
	exitBtn->setPosition(btnMenu->convertToNodeSpace({
		winSize.width / 2.0f,
		winSize.height / 2.0f - 80.0f
	}));

	btnMenu->addChild(playBtn);
	btnMenu->addChild(exitBtn);

	return true;
}

void FL_MenuLayer::keyBackClicked() {
	CCDirector::sharedDirector()
		->end();
}

void FL_MenuLayer::onExitGame(CCObject* pSender) { keyBackClicked(); }
void FL_MenuLayer::onPlay(CCObject* pSender) {
	FL_PlayLayer::Args args;
	args.levelID = 1;

	auto gameScene = FL_PlayLayer::scene(args);
	CCDirector::sharedDirector()
		->replaceScene(CCTransitionFade::create(0.5f, gameScene));
}