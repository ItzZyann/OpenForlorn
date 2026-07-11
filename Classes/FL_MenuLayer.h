#pragma once
#include "incl.h"

class FL_MenuLayer : public CCLayer {
public:
	virtual bool init();
	virtual void keyBackClicked();

	static CCScene* scene();

	void onPlay(CCObject* sender);
	void onContinue(CCObject* sender);

	CREATE_FUNC(FL_MenuLayer);
};