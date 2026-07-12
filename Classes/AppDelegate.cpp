#include "AppDelegate.h"
#include "SimpleAudioEngine.h"
#include "FL_LoadingLayer.h"
#include "AppMacros.h"

using namespace CocosDenshion;
USING_NS_CC;

AppDelegate::AppDelegate() { }
AppDelegate::~AppDelegate() { }

bool AppDelegate::applicationDidFinishLaunching() {
	CCDirector* pDirector = CCDirector::sharedDirector();
	CCEGLView* pEGLView = CCEGLView::sharedOpenGLView();

	#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		CCFileUtils::sharedFileUtils()->addSearchPath("Resources");
	#endif

	pDirector->setOpenGLView(pEGLView);
	// UI HD atlases intentionally use their full 2x logical dimensions.
	// Gameplay applies its HD compensation at node level instead.
	pDirector->setContentScaleFactor(1.0f);
	pDirector->setDisplayStats(true);
	pDirector->setAnimationInterval(1.0 / 60);

	CCScene *pScene = FL_LoadingLayer::scene();
	pDirector->runWithScene(pScene);

	return true;
}

void AppDelegate::applicationDidEnterBackground() {
	CCDirector::sharedDirector()->stopAnimation();
	SimpleAudioEngine::sharedEngine()->pauseBackgroundMusic();
}

void AppDelegate::applicationWillEnterForeground() {
	CCDirector::sharedDirector()->startAnimation();
	SimpleAudioEngine::sharedEngine()->resumeBackgroundMusic();
}
