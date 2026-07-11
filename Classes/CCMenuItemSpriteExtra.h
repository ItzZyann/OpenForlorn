#pragma once
#include "incl.h"

class CCMenuItemSpriteExtra : public CCMenuItemSprite {
private:
	CCNode* m_pSprite;
	float m_fAnimDuration;
	float m_fSelectedScale;
	float m_fUnselectedScale;
	float m_fScaleMult;
	CCPoint _startPos;
	CCPoint _offsetPos;
	bool _altAnim;
	
public:
	static CCMenuItemSpriteExtra* create(CCNode* normalSprite, CCNode* selectedSprite, CCObject* target, SEL_MenuHandler selector);
	void setScaleMultiplier(float mult);
	void setDestination(const CCPoint& dest);
	virtual void setScale(float s) override;
	CCNode* getSprite();

protected:
	CCMenuItemSpriteExtra(CCNode* normalSprite, CCNode* selectedSprite, CCObject* target, SEL_MenuHandler selector);
	virtual bool init();
	virtual void selected() override;
	virtual void unselected() override;
	virtual void activate() override;
};