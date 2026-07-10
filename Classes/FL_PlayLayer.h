#pragma once
#include "incl.h"
#include "FL_Block.h" 
#include "FL_UILayerDelegate.h"

#include <map>
#include <string>
#include <vector>

class FL_PlayLayer : public CCLayer, public FL_UILayerDelegate {
public:
	struct Args {
		int levelID;
		Args() : levelID(1) {}
	};

	static CCScene* scene(const Args& args);
	static FL_PlayLayer* create(const Args& args);

	virtual bool init(const Args& args);
	virtual ~FL_PlayLayer();
	FL_PlayLayer();

	virtual void registerWithTouchDispatcher();
	virtual bool ccTouchBegan(CCTouch* touch, CCEvent* event);
	virtual void ccTouchMoved(CCTouch* touch, CCEvent* event);
	virtual void ccTouchEnded(CCTouch* touch, CCEvent* event);

	virtual void keyBackClicked();

	virtual void uiUpPressed() override;
	virtual void uiUpReleased() override;
	virtual void uiDownPressed() override;
	virtual void uiDownReleased() override;
	virtual void uiLeftPressed() override;
	virtual void uiLeftReleased() override;
	virtual void uiRightPressed() override;
	virtual void uiRightReleased() override;

	void onLeft();
	void onRight();
	void onUp();
	void onDown();

	virtual void onEnter();
	virtual void onExit();
	virtual void update(float dt);

private:
	struct Members;
	Members* m;
};