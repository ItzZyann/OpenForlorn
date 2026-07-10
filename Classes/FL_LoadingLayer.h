#pragma once
#include "incl.h"

class FL_LoadingLayer : public CCLayer {
private:
    bool m_preloadAssets;
    bool m_loadAssets;

public:
    FL_LoadingLayer()
        : m_preloadAssets(false)
        , m_loadAssets(false) { };

	virtual bool init();

    void preloadAssets();
	void loadAssets();
	void loadingFinished();

	static cocos2d::CCScene* scene();
	CREATE_FUNC(FL_LoadingLayer);
};