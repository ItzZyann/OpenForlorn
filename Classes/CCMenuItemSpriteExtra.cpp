#include "CCMenuItemSpriteExtra.h"

CCMenuItemSpriteExtra::CCMenuItemSpriteExtra(CCNode* normalSprite, CCNode* selectedSprite, CCObject* target, SEL_MenuHandler selector) :
	_altAnim(false), m_pSprite(normalSprite) {
 
	float nodescale = m_pSprite->getScale();
  
	m_fScaleMult = 1.26;
	m_fSelectedScale = nodescale * m_fScaleMult;
	m_fUnselectedScale = nodescale;
	m_fAnimDuration = .3;

	initWithNormalSprite(normalSprite, selectedSprite, nullptr, target, selector);
}

CCMenuItemSpriteExtra* CCMenuItemSpriteExtra::create(CCNode* normalSprite, CCNode* selectedSprite, CCObject* target, SEL_MenuHandler selector) {
	CCMenuItemSpriteExtra* pRet = new CCMenuItemSpriteExtra(normalSprite, selectedSprite, target, selector);
 
	if (pRet && pRet->init()) {
		pRet->autorelease();
		return pRet;
	}
 
	CC_SAFE_DELETE(pRet);
	return nullptr;
}

bool CCMenuItemSpriteExtra::init() {
	if(!m_pSprite) return false;

	m_pSprite->setAnchorPoint({.5, .5});
	m_pSprite->setPosition({m_pSprite->getContentSize().width / 2, m_pSprite->getContentSize().height / 2});
 
	return true;
}

void CCMenuItemSpriteExtra::selected() {
	if(_altAnim)
		m_pSprite->runAction(CCEaseInOut::create(CCMoveTo::create(m_fAnimDuration, _offsetPos), 1.5));
	else
		m_pSprite->runAction(CCEaseBounceOut::create(CCScaleTo::create(m_fAnimDuration, m_fSelectedScale)));
  
	CCMenuItemSprite::selected();
}

void CCMenuItemSpriteExtra::unselected() {
	m_pSprite->stopAllActions();

	if(_altAnim)
		m_pSprite->runAction(CCEaseInOut::create(CCMoveTo::create(m_fAnimDuration, _startPos), 2));
	else
		m_pSprite->runAction(CCEaseBounceOut::create(CCScaleTo::create(.4, m_fUnselectedScale)));
 
	CCMenuItemSprite::unselected();
}

void CCMenuItemSpriteExtra::activate() {
	m_pSprite->stopAllActions();
 
	if(_altAnim)
		m_pSprite->setPosition(_startPos);
	else
		m_pSprite->setScale(m_fUnselectedScale);
 
	CCMenuItemSprite::activate();
}

void CCMenuItemSpriteExtra::setDestination(const CCPoint& dest) {
	_altAnim = true;
	_startPos = m_pSprite->getPosition();
	_offsetPos = ccp(_startPos.x + dest.x, _startPos.y + dest.y);
}

void CCMenuItemSpriteExtra::setScaleMultiplier(float mult) {
	m_fScaleMult = mult;
	m_fSelectedScale = m_fUnselectedScale * mult;
}

void CCMenuItemSpriteExtra::setScale(float s) {
	m_fUnselectedScale = s;
	m_fSelectedScale = s * m_fScaleMult;
	m_pSprite->setScale(s);
}

CCNode* CCMenuItemSpriteExtra::getSprite() {
	return m_pSprite;
}