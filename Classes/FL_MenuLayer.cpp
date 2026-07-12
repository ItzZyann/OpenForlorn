#include "FL_MenuLayer.h"
#include "FL_PlayLayer.h"
#include "FL_LevelScene.h"
#include "SimpleAudioEngine.h"
#include "CCMenuItemSpriteExtra.h"

CCScene* FL_MenuLayer::scene() {
	CCScene* scene = CCScene::create();
	FL_MenuLayer* layer = FL_MenuLayer::create();
	scene->addChild(layer, 1);
	return scene;
}

bool FL_MenuLayer::init() {
	if (!CCLayer::init()) return false;

	setKeypadEnabled(true);

	const CCSize winSize = CCDirector::sharedDirector()->getWinSize();

	FL_PlayLayer::Args previewArgs;
	previewArgs.levelFile = "LevelCave.plist";
	previewArgs.previewMode = true;
	previewArgs.initialZoom = 1.8f;
	FL_PlayLayer* cavePreview = FL_PlayLayer::create(previewArgs);
	if (cavePreview) {
		addChild(cavePreview, -1000000);
		cavePreview->attachFixedBackground(this, -2000000);
	}

	CCTextureCache::sharedTextureCache()->addImage("MenuSheet-hd.png");
	CCSpriteFrameCache::sharedSpriteFrameCache()
		->addSpriteFramesWithFile("MenuSheet-hd.plist");

	CCSprite* playSprite = CCSprite::createWithSpriteFrameName("play_btn.png");
	playSprite->setScale(.8);

	CCMenuItemSpriteExtra* playItem = CCMenuItemSpriteExtra::create(
		playSprite, NULL, this, menu_selector(FL_MenuLayer::onPlay)
		);

	CCMenu* menu = CCMenu::create(playItem, NULL);
	menu->alignItemsVerticallyWithPadding(10.0f);
	CCPoint menuPos = menu->getPosition();
	menu->setPosition(ccp(menuPos.x, menuPos.y - 80.0f));
	addChild(menu, 1);

	CCSprite* logo = CCSprite::create("forlorn_logo_menu-hd.png");
	logo->setScale(.8);
	if (logo) {
		CCRect tr = logo->getTextureRect();
		logo->setPosition(ccp(
			winSize.width  * 0.5f - 5.0f,
			winSize.height - tr.size.height * 0.5f - 5.0f
			));
		addChild(logo, 2);

		CCParticleSystemQuad* logoFx =
			CCParticleSystemQuad::create("logoEffect.plist");
		if (logoFx) {
			logoFx->setPosition(logo->getPosition());
			addChild(logoFx, 1);
		}
	}

	return true;
}

void FL_MenuLayer::onContinue(CCObject*) { }
void FL_MenuLayer::keyBackClicked() { CCDirector::sharedDirector()->end(); }

void FL_MenuLayer::onPlay(CCObject*) {
	SimpleAudioEngine::sharedEngine()->playEffect("FL_StartButton.ogg");

	CCDirector::sharedDirector()->replaceScene(
		CCTransitionFade::create(0.5f, FL_LevelScene::scene())
		);
}
