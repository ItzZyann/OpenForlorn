#include "FL_LevelScene.h"
#include "FL_MenuLayer.h"
#include "FL_PlayLayer.h"
#include "SimpleAudioEngine.h"

USING_NS_CC;
using namespace CocosDenshion;

namespace {
	const char* const kLevelFiles[] = {
		"Level001.plist",
		"Level002.plist",
		"Level003.plist",
		"Level004.plist",
		"Level005.plist",
		"LevelCave.plist"
	};
	const int kLevelCount = 6;
}

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
		audio->setBackgroundMusicVolume(1.0f);
	}

	m_internalLayer = CCLayer::create();
	addChild(m_internalLayer);

	CCSprite* worldMap = CCSprite::create("FL_Map_World.png");
	if (worldMap)
		m_internalLayer->addChild(worldMap);

	m_mapDefinitions = CCMenu::create();
	m_mapDefinitions->setAnchorPoint(CCPointZero);
	m_mapDefinitions->setPosition(CCPointZero);
	m_internalLayer->addChild(m_mapDefinitions, 2);

	CCSpriteFrameCache::sharedSpriteFrameCache()
		->addSpriteFramesWithFile("MenuSheet.plist");

	std::string fullPath = CCFileUtils::sharedFileUtils()
		->fullPathForFilename("mapDefinitions.plist");
	m_mapData = CCDictionary::createWithContentsOfFile(fullPath.c_str());
	CC_SAFE_RETAIN(m_mapData);

	showLevels(5);

	CCSprite* backSprite = CCSprite::createWithSpriteFrameName("back_btn.png");
	CCMenuItemSprite* backItem = CCMenuItemSprite::create(
		backSprite, NULL, this, menu_selector(FL_LevelScene::onBack)
		);
	CCMenu* backMenu = CCMenu::create(backItem, NULL);
	backMenu->setPosition({ 50, winSize.height - 30 });
	addChild(backMenu, 10);

	return true;
}

void FL_LevelScene::onEnterTransitionDidFinish() { CCLayer::onEnterTransitionDidFinish(); }

void FL_LevelScene::addMenuItemWithTexture(const char* frameName, int tag,
	CCPoint pos, float scale) {
	CCSprite* normal = CCSprite::createWithSpriteFrameName(frameName);
	CCMenuItemSprite* item = CCMenuItemSprite::create(
		normal, NULL, this, menu_selector(FL_LevelScene::onPlay)
		);
	item->setTag(tag);
	item->setPosition(pos);
	item->setScale(scale);
	m_mapDefinitions->addChild(item);
}

void FL_LevelScene::addPathFromArray(CCArray* points, bool fade) {
	if (!points || points->count() < 2) return;

	for (unsigned int i = 1; i < points->count(); ++i) {
		CCString* ptStr = static_cast<CCString*>(points->objectAtIndex(i));
		CCPoint pt = ptStr ? CCPointFromString(ptStr->getCString()) : CCPointZero;

		CCSprite* dot = CCSprite::createWithSpriteFrameName("mapDot_001.png");
		dot->setPosition(pt);
		m_internalLayer->addChild(dot, 1);

		if (fade) {
			dot->setOpacity(0);

			float delay = (float)i * 0.2f;

			CCActionInterval* fadeIn = CCActionTween::create(
				0.2f, "opacity", 0.0f, 255.0f
				);
			CCActionInterval* scaleSeq = CCSequence::create(
				CCScaleTo::create(0.1f, 2.0f),
				CCScaleTo::create(0.1f, 1.0f),
				NULL
				);
			CCAction* full = CCSequence::create(
				CCDelayTime::create(delay),
				CCSpawn::create(fadeIn, scaleSeq, NULL),
				NULL
				);
			dot->runAction(full);
		}
	}
}

void FL_LevelScene::showLevels(int count) {
	if (!m_mapData) return;

	CCDictionary* levels = static_cast<CCDictionary*>(
		m_mapData->objectForKey("Levels")
		);
	if (!levels) return;

	int available = (int)levels->count();
	if (count > available) count = available;

	int displayCount = (count > 5) ? 5 : count;
	if (displayCount < 1) return;

	int upperBound = (count < 6) ? count : 6;

	for (int i = 1; i <= upperBound; ++i) {
		char key[8];
		snprintf(key, sizeof(key), "%03d", i);

		CCDictionary* entry = static_cast<CCDictionary*>(
			levels->objectForKey(key)
			);
		if (!entry) continue;

		CCString* startStr = static_cast<CCString*>(entry->objectForKey("Start"));
		CCPoint startPos = startStr
			? CCPointFromString(startStr->getCString())
			: CCPointZero;

		int btnVariant = (i < 3) ? i : 3;
		char frameName[32];
		snprintf(frameName, sizeof(frameName), "LevelBtn%d_001.png", btnVariant);

		addMenuItemWithTexture(frameName, i, startPos, 1.0f);

		if (i < displayCount) {
			char nextKey[8];
			snprintf(nextKey, sizeof(nextKey), "%03d", i + 1);
			CCDictionary* nextEntry = static_cast<CCDictionary*>(
				levels->objectForKey(nextKey)
				);
			if (!nextEntry) continue;

			CCString* nextStartStr = static_cast<CCString*>(
				nextEntry->objectForKey("Start")
				);
			CCPoint nextPos = nextStartStr
				? CCPointFromString(nextStartStr->getCString())
				: CCPointZero;

			float dx = nextPos.x - startPos.x;
			float dy = nextPos.y - startPos.y;
			float dist = sqrtf(dx * dx + dy * dy);
			int steps = (int)(dist / 10.0f);
			if (steps < 1) steps = 1;

			CCArray* pathPoints = CCArray::create();
			for (int s = 1; s <= steps; ++s) {
				float t = (float)s / (float)(steps + 1);
				CCPoint pt = ccp(
					startPos.x + dx * t,
					startPos.y + dy * t
					);
				pathPoints->addObject(CCString::createWithFormat("{%f, %f}", pt.x, pt.y));
			}
			addPathFromArray(pathPoints, false);
		}
	}
}

void FL_LevelScene::onBack(CCObject*) {
	CCDirector::sharedDirector()->replaceScene(
		CCTransitionFade::create(0.5f, FL_MenuLayer::scene())
		);
}

void FL_LevelScene::onPlay(CCObject* sender) {
	CCNode* node = dynamic_cast<CCNode*>(sender);
	if (!node) return;

	int tag = node->getTag();
	if (tag < 1 || tag > kLevelCount) return;

	SimpleAudioEngine::sharedEngine()->stopBackgroundMusic();

	FL_PlayLayer::Args args;
	args.levelFile = kLevelFiles[tag - 1]; // tag is 1-based, array is 0-based

	CCScene* gameScene = FL_PlayLayer::scene(args);
	if (gameScene) {
		CCDirector::sharedDirector()->replaceScene(
			CCTransitionFade::create(0.35f, gameScene)
			);
	}
}