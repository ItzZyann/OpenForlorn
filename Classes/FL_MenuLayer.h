#pragma once
#include "incl.h"

class FL_MenuLayer : public CCLayer {
public:
	virtual bool init();
	virtual void keyBackClicked();

	void onExitGame(CCObject*);
	void onPlay(CCObject*);

	CREATE_FUNC(FL_MenuLayer);
};