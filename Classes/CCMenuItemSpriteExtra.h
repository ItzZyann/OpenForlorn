#pragma once
#include "incl.h"

// A drop-in replacement for CCMenuItemSprite that adds a small
// "pop" scale animation on touch down/up, similar to what many
// cocos2d-x game UIs use for button feedback.
//
// Usage is identical to CCMenuItemSprite::create(...):
//   CCMenuItemSpriteExtra::create(normalSprite, selectedSprite, target, selector);
class CCMenuItemSpriteExtra : public cocos2d::CCMenuItemSprite {
public:
	static CCMenuItemSpriteExtra* create(
		cocos2d::CCNode* normalSprite,
		cocos2d::CCNode* selectedSprite,
		cocos2d::CCObject* target,
		cocos2d::SEL_MenuHandler selector
	);

	virtual bool initWithNormalSprite(
		cocos2d::CCNode* normalSprite,
		cocos2d::CCNode* selectedSprite,
		cocos2d::CCNode* disabledSprite,
		cocos2d::CCObject* target,
		cocos2d::SEL_MenuHandler selector
	);

	virtual void selected();
	virtual void unselected();
	virtual void activate();

protected:
	float m_baseScale;
};
