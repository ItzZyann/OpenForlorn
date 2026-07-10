#include "FL_LoadingLayer.h"
#include "FL_MenuLayer.h"

bool FL_LoadingLayer::init() {
	if(!CCLayer::init()) return false;

	auto dir = CCDirector::sharedDirector();
	auto winSize = dir->getWinSize();

    // load steps by steps
    // because yes xD.
    auto seq = CCSequence::create(
		CCCallFunc::create(this, callfunc_selector(FL_LoadingLayer::preloadAssets)),
		CCCallFunc::create(this, callfunc_selector(FL_LoadingLayer::loadAssets)),
		CCDelayTime::create(.3),
		CCCallFunc::create(this, callfunc_selector(FL_LoadingLayer::loadingFinished)),
		nullptr
	);
	runAction(seq);

    return true;
}

void FL_LoadingLayer::preloadAssets() {
    if(!m_preloadAssets) {
	    auto texture = CCTextureCache::sharedTextureCache();
	    
		texture->addImage("UISheet.png");
		texture->addImage("ForestSheet_MG.png");
		texture->addImage("lightSheet.png");
		// texture->addImage("ForestLevel_MGSheet.png");
		texture->addImage("FlameVillageMG.png");

        m_preloadAssets = true;
    }

    return;
}
void FL_LoadingLayer::loadAssets() {
    if(m_preloadAssets && !m_loadAssets) { 
	    auto frame = CCSpriteFrameCache::sharedSpriteFrameCache();

	    frame->addSpriteFramesWithFile("UISheet.plist");
		frame->addSpriteFramesWithFile("ForestSheet_MG.plist");
		frame->addSpriteFramesWithFile("lightSheet.plist");
		// frame->addSpriteFramesWithFile("ForestLevel_MGSheet.plist");
		frame->addSpriteFramesWithFile("FlameVillageMG.plist");

        m_loadAssets = true;
    }

    return;
}

void FL_LoadingLayer::loadingFinished() {
    if(m_preloadAssets && m_loadAssets) {
	    auto layer = FL_MenuLayer::create();
	    auto scene = CCScene::create();
	    scene->addChild(layer);

	    CCDirector::sharedDirector()
	    	->replaceScene(scene);
    }
}

CCScene* FL_LoadingLayer::scene() {
	auto layer = FL_LoadingLayer::create();
	auto scene = CCScene::create();
	scene->addChild(layer);

	return scene;
}