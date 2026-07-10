#include "AppDelegate.h"
#include "SimpleAudioEngine.h"
#include "FL_LoadingLayer.h"

using namespace CocosDenshion;
USING_NS_CC;

AppDelegate::AppDelegate() { }
AppDelegate::~AppDelegate() { }

bool AppDelegate::applicationDidFinishLaunching() {
    CCDirector* pDirector = CCDirector::sharedDirector();
    CCEGLView* pEGLView = CCEGLView::sharedOpenGLView();

    pDirector->setOpenGLView(pEGLView);
    pDirector->setDisplayStats(true);
    pDirector->setAnimationInterval(1.0 / 60);

	#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		CCFileUtils::sharedFileUtils()->addSearchPath("Resources");
	#endif

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
