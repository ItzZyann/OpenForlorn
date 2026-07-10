#pragma once
#include "incl.h"
#include <map>

class FL_UILayerDelegate;
class FL_UILayer : public CCLayer {
public:
	virtual bool init();
	void setDelegate(FL_UILayerDelegate* pDelegate);

	virtual void registerWithTouchDispatcher();
	virtual bool ccTouchBegan(CCTouch* touch, CCEvent* event);
	virtual void ccTouchMoved(CCTouch* touch, CCEvent* event);
	virtual void ccTouchEnded(CCTouch* touch, CCEvent* event);
	virtual void ccTouchCancelled(CCTouch* touch, CCEvent* event);

	CREATE_FUNC(FL_UILayer);

private:
	enum Direction {
		DIR_NONE = 0,
		DIR_LEFT,
		DIR_RIGHT,
		DIR_UP,    // jump
		DIR_DOWN   // attack
	};

	Direction hitTest(const CCPoint& loc);
	void pressDirection(Direction dir);
	void releaseDirection(Direction dir);

	FL_UILayerDelegate* m_delegate;

	CCSprite* m_dpadSprite;
	CCSpriteFrame* m_dpadIdleFrame;
	CCSpriteFrame* m_dpadDwnFrame;

	CCSprite* m_jumpSprite;
	CCSpriteFrame* m_jumpIdleFrame;
	CCSpriteFrame* m_jumpDwnFrame;

	CCSprite* m_attackSprite;
	CCSpriteFrame* m_attackIdleFrame;
	CCSpriteFrame* m_attackDwnFrame;

	std::map<CCTouch*, Direction> m_activeTouches;
};
