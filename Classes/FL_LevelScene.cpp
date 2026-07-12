#include "FL_LevelScene.h"
#include "FL_MenuLayer.h"
#include "FL_PlayLayer.h"
#include "FL_LevelLoadingLayer.h"
#include "LagrangeDescriptor.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;
using namespace CocosDenshion;

CCScene* FL_LevelScene::scene() {
	CCScene* scene = CCScene::create();
	FL_LevelScene* layer = FL_LevelScene::create();
	scene->addChild(layer);
	return scene;
}

FL_LevelScene::~FL_LevelScene() {
	removeAllChildrenWithCleanup(true);
	CC_SAFE_RELEASE(m_mapData);
}

bool FL_LevelScene::init() {
	if (!CCLayer::init()) return false;

	const CCSize winSize = CCDirector::sharedDirector()->getWinSize();

	SimpleAudioEngine* audio = SimpleAudioEngine::sharedEngine();
	if (!audio->isBackgroundMusicPlaying()) {
		audio->playBackgroundMusic("loop_17.mp3", true);
	}

	CCNode* p = getParent();
	if (p)
		setContentSize(p->getContentSize());

	m_internalLayer = CCLayer::create();

	CCSprite* worldMap = CCSprite::create("FL_Map_World-hd.png");
	if (worldMap) {
		// The map image is scene content, not UI: keep its authored 1x map size.
		worldMap->setScale(0.5f);
		m_internalLayer->addChild(worldMap);
		m_internalLayer->setContentSize(CCSizeMake(
			worldMap->getContentSize().width * 0.5f,
			worldMap->getContentSize().height * 0.5f
			));
	}
	else {
		m_internalLayer->setContentSize(CCSizeZero);
	}

	addChild(m_internalLayer);

	m_mapDefinitions = CCMenu::create(NULL);
	m_mapDefinitions->setAnchorPoint(CCPointZero);
	m_mapDefinitions->setPosition(CCPointZero);
	m_internalLayer->addChild(m_mapDefinitions, 2);

	CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("MenuSheet-hd.plist");

	m_uiSheet = CCSpriteBatchNode::create("UISheet-hd.png");
	m_internalLayer->addChild(m_uiSheet, 1);

	std::string fullPath = CCFileUtils::sharedFileUtils()->fullPathForFilename("mapDefinitions.plist");
	m_mapData = CCDictionary::createWithContentsOfFile(fullPath.c_str());
	CC_SAFE_RETAIN(m_mapData);

	m_displayCount = 5;
	showLevels(m_displayCount);

	CCSprite* backSprite = CCSprite::createWithSpriteFrameName("back_btn.png");
	CCMenuItemSprite* backItem = CCMenuItemSprite::create(
		backSprite, NULL, this, menu_selector(FL_LevelScene::onBack));
	CCMenu* backMenu = CCMenu::create(backItem, NULL);

	float bw = backSprite ? backSprite->getContentSize().width : 50.0f;
	float bh = backSprite ? backSprite->getContentSize().height : 30.0f;
	backMenu->setPosition(ccp(bw * 0.5f + 5.0f, winSize.height - bh * 0.5f - 5.0f));
	addChild(backMenu, 10);

	return true;
}

void FL_LevelScene::onEnterTransitionDidFinish() {
	CCLayer::onEnterTransitionDidFinish();
}

void FL_LevelScene::addMenuItemWithTexture(const char* frameName, int tag,
	CCPoint pos, float scale) {
	CCSprite* normal = CCSprite::createWithSpriteFrameName(frameName);
	CCMenuItemSprite* item = CCMenuItemSprite::create(
		normal, NULL, this, menu_selector(FL_LevelScene::onPlay));
	item->setTag(tag);
	item->setPosition(pos);
	item->setScale(scale);
	m_mapDefinitions->addChild(item);
}

void FL_LevelScene::addPathFromArray(CCArray* points, bool fade) {
	if (!points || points->count() < 2) return;

	for (unsigned int i = 1; i < points->count(); ++i) {
		CCPoint pt = CCPointZero;
		CCObject* obj = points->objectAtIndex(i);
		if (obj) {
			CCString* ptStr = dynamic_cast<CCString*>(obj);
			if (ptStr) pt = CCPointFromString(ptStr->getCString());
		}

		CCSprite* dot = CCSprite::createWithSpriteFrameName("mapDot_001.png");
		dot->setPosition(pt);
		m_uiSheet->addChild(dot);

		if (fade) {
			dot->setOpacity(0);

			float delay = (float)i * 0.2f;

			CCActionInterval* fadeIn = CCActionTween::create(0.2f, "opacity", 0.0f, 255.0f);
			CCActionInterval* scaleSeq = CCSequence::create(
				CCScaleTo::create(0.1f, 2.0f),
				CCScaleTo::create(0.1f, 1.0f),
				NULL);
			CCAction* full = CCSequence::create(
				CCDelayTime::create(delay),
				CCSpawn::create(fadeIn, scaleSeq, NULL),
				NULL);
			dot->runAction(full);
		}
	}
}

void FL_LevelScene::showLevels(int count) {
	if (!m_mapData) return;

	CCDictionary* levels = static_cast<CCDictionary*>(m_mapData->objectForKey("Levels"));
	if (!levels) return;

	int available = (int)levels->count();
	if (count > available) count = available;

	int displayCount = (count > 5) ? 5 : count;
	if (displayCount < 1) return;

	int upperBound = (count < 6) ? count : 6;

	for (int i = 1; i <= upperBound; ++i) {
		char key[8];
		snprintf(key, sizeof(key), "%03d", i);

		CCDictionary* entry = static_cast<CCDictionary*>(levels->objectForKey(key));
		if (!entry) continue;

		CCString* startStr = static_cast<CCString*>(entry->objectForKey("Start"));
		CCPoint startPos = startStr ? CCPointFromString(startStr->getCString()) : CCPointZero;

		int btnVariant = (i < 3) ? i : 3;
		char frameName[32];
		snprintf(frameName, sizeof(frameName), "LevelBtn%d_001.png", btnVariant);

		addMenuItemWithTexture(frameName, i, startPos, 1.0f);

		if (i < displayCount) {
			CCString* p1Str = static_cast<CCString*>(entry->objectForKey("point1"));
			CCString* p2Str = static_cast<CCString*>(entry->objectForKey("point2"));
			CCPoint cp1 = p1Str ? CCPointFromString(p1Str->getCString()) : CCPointZero;
			CCPoint cp2 = p2Str ? CCPointFromString(p2Str->getCString()) : CCPointZero;

			char nextKey[8];
			snprintf(nextKey, sizeof(nextKey), "%03d", i + 1);
			CCDictionary* nextEntry = static_cast<CCDictionary*>(levels->objectForKey(nextKey));
			if (!nextEntry) continue;

			CCString* nextStartStr = static_cast<CCString*>(nextEntry->objectForKey("Start"));
			CCPoint nextPos = nextStartStr ? CCPointFromString(nextStartStr->getCString()) : CCPointZero;

			LagrangeDescriptor* ld = LagrangeDescriptor::createLagrangeDescriptor(
				nextPos,
				cp1,
				cp2,
				20.0f,
				startPos);

			CCArray* pathPoints = ld->pointContainer();
			addPathFromArray(pathPoints, false);
		}
	}
}

void FL_LevelScene::onBack(CCObject*) {
	CCDirector::sharedDirector()->replaceScene(
		CCTransitionFade::create(0.5f, FL_MenuLayer::scene()));
}

void FL_LevelScene::onPlay(CCObject* sender) {
	CCNode* node = dynamic_cast<CCNode*>(sender);
	if (!node) return;

	int tag = node->getTag();

	SimpleAudioEngine::sharedEngine()->stopBackgroundMusic();

	char buf[32];
	snprintf(buf, sizeof(buf), "Level%03d.plist", tag);

	FL_PlayLayer::Args args;
	args.levelFile = buf;

	CCScene* gameScene = FL_PlayLayer::scene(args);
	if (gameScene) {
		CCDirector::sharedDirector()->replaceScene(
			CCTransitionFade::create(0.35f, gameScene));
	}
}
