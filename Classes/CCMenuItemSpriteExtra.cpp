#include "CCMenuItemSpriteExtra.h"

USING_NS_CC;

CCMenuItemSpriteExtra* CCMenuItemSpriteExtra::create(
	CCNode* normalSprite,
	CCNode* selectedSprite,
	CCObject* target,
	SEL_MenuHandler selector
) {
	CCMenuItemSpriteExtra* pRet = new CCMenuItemSpriteExtra();
	if (pRet && pRet->initWithNormalSprite(normalSprite, selectedSprite, nullptr, target, selector)) {
		pRet->autorelease();
		return pRet;
	}
	delete pRet;
	return nullptr;
}

bool CCMenuItemSpriteExtra::initWithNormalSprite(
	CCNode* normalSprite,
	CCNode* selectedSprite,
	CCNode* disabledSprite,
	CCObject* target,
	SEL_MenuHandler selector
) {
	if (!CCMenuItemSprite::initWithNormalSprite(normalSprite, selectedSprite, disabledSprite, target, selector))
		return false;

	m_baseScale = 1.0f;
	return true;
}

void CCMenuItemSpriteExtra::selected() {
	CCMenuItemSprite::selected();

	m_baseScale = getScale();
	stopActionByTag(1);

	auto scaleTo = CCScaleTo::create(0.1f, m_baseScale * 0.9f);
	scaleTo->setTag(1);
	runAction(scaleTo);
}

void CCMenuItemSpriteExtra::unselected() {
	CCMenuItemSprite::unselected();

	stopActionByTag(1);

	auto scaleTo = CCScaleTo::create(0.1f, m_baseScale);
	scaleTo->setTag(1);
	runAction(scaleTo);
}

void CCMenuItemSpriteExtra::activate() {
	CCMenuItemSprite::activate();
}
