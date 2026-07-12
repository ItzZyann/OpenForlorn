#pragma once
#include "incl.h"

class FL_LevelScene : public CCLayer {
public:
	virtual bool init();
	virtual void onEnterTransitionDidFinish();
	virtual ~FL_LevelScene();

	static CCScene* scene();

	void addMenuItemWithTexture(const char* frameName, int tag, CCPoint pos, float scale);
	void addPathFromArray(CCArray* points, bool fade);
	void showLevels(int count);

	void onPlay(CCObject* sender);
	void onBack(CCObject* sender);

	CREATE_FUNC(FL_LevelScene);

private:
	CCLayer*           m_internalLayer;
	CCMenu*            m_mapDefinitions;
	CCSpriteBatchNode* m_uiSheet;
	CCDictionary*      m_mapData;
	int                m_displayCount;
};