#include "FL_UILayer.h"
#include "FL_UILayerDelegate.h"

bool FL_UILayer::init() {
	if (!CCLayer::init()) return false;

	auto dir = CCDirector::sharedDirector();
	auto winSize = dir->getWinSize();

	auto sprch = CCSpriteFrameCache::sharedSpriteFrameCache();
	sprch->addSpriteFramesWithFile("UISheet.plist");

	m_dpadIdleFrame = sprch->spriteFrameByName("Dpad_Btn.png");
	m_dpadDwnFrame = sprch->spriteFrameByName("Dpad_Btn_Dwn.png");
	m_dpadIdleFrame->retain();
	m_dpadDwnFrame->retain();

	m_dpadSprite = CCSprite::createWithSpriteFrame(m_dpadIdleFrame);
	m_dpadSprite->setPosition(ccp(
		20.0f + m_dpadSprite->getContentSize().width / 2.0f,
		20.0f + m_dpadSprite->getContentSize().height / 2.0f
	));
	addChild(m_dpadSprite, 100);

	m_attackIdleFrame = sprch->spriteFrameByName("Attack_Btn.png");
	m_attackDwnFrame = sprch->spriteFrameByName("Attack_Btn_Dwn.png");
	m_attackIdleFrame->retain();
	m_attackDwnFrame->retain();
	m_attackSprite = CCSprite::createWithSpriteFrame(m_attackIdleFrame);

	m_jumpIdleFrame = sprch->spriteFrameByName("Jump_Btn.png");
	m_jumpDwnFrame = sprch->spriteFrameByName("Jump_Btn_Dwn.png");
	m_jumpIdleFrame->retain();
	m_jumpDwnFrame->retain();
	m_jumpSprite = CCSprite::createWithSpriteFrame(m_jumpIdleFrame);

	float rightMargin = 20.0f;
	float bottomMargin = 20.0f;
	float gap = 20.0f;

	m_jumpSprite->setPosition(ccp(
		winSize.width - rightMargin - m_jumpSprite->getContentSize().width / 2.0f,
		bottomMargin + m_jumpSprite->getContentSize().height / 2.0f
	));
	m_attackSprite->setPosition(ccp(
		m_jumpSprite->getPositionX() - m_jumpSprite->getContentSize().width / 2.0f
			- gap - m_attackSprite->getContentSize().width / 2.0f,
		bottomMargin + m_attackSprite->getContentSize().height / 2.0f
	));

	addChild(m_attackSprite, 100);
	addChild(m_jumpSprite, 100);

	setTouchEnabled(true);

	return true;
}

void FL_UILayer::registerWithTouchDispatcher() {
	CCDirector::sharedDirector()
        ->getTouchDispatcher()
            ->addTargetedDelegate(this, 0, true);
}

void FL_UILayer::setDelegate(FL_UILayerDelegate* delegate) {
	m_delegate = delegate;
}

FL_UILayer::Direction FL_UILayer::hitTest(const CCPoint& loc) {
	CCSize dpadSize = m_dpadSprite->getContentSize();
	CCPoint dpadPos = m_dpadSprite->getPosition();
	float dpadLeft = dpadPos.x - dpadSize.width / 2.0f;
	float dpadRight = dpadPos.x + dpadSize.width / 2.0f;
	float dpadBottom = dpadPos.y - dpadSize.height / 2.0f;
	float dpadTop = dpadPos.y + dpadSize.height / 2.0f;
	if(loc.x >= dpadLeft && loc.x <= dpadRight && loc.y >= dpadBottom && loc.y <= dpadTop) {
		return (loc.x < dpadPos.x) ? DIR_LEFT : DIR_RIGHT;
	}

	CCSize jumpSize = m_jumpSprite->getContentSize();
	CCPoint jumpPos = m_jumpSprite->getPosition();
	float jumpLeft = jumpPos.x - jumpSize.width / 2.0f;
	float jumpRight = jumpPos.x + jumpSize.width / 2.0f;
	float jumpBottom = jumpPos.y - jumpSize.height / 2.0f;
	float jumpTop = jumpPos.y + jumpSize.height / 2.0f;
	if(loc.x >= jumpLeft && loc.x <= jumpRight && loc.y >= jumpBottom && loc.y <= jumpTop) {
		return DIR_UP;
	}

	CCSize atkSize = m_attackSprite->getContentSize();
	CCPoint atkPos = m_attackSprite->getPosition();
	float atkLeft = atkPos.x - atkSize.width / 2.0f;
	float atkRight = atkPos.x + atkSize.width / 2.0f;
	float atkBottom = atkPos.y - atkSize.height / 2.0f;
	float atkTop = atkPos.y + atkSize.height / 2.0f;
	if(loc.x >= atkLeft && loc.x <= atkRight && loc.y >= atkBottom && loc.y <= atkTop) {
		return DIR_DOWN;
	}

	return DIR_NONE;
}

void FL_UILayer::pressDirection(Direction dir) {
	switch (dir) {
	case DIR_LEFT:
		m_dpadSprite->setDisplayFrame(m_dpadDwnFrame);
		m_dpadSprite->setFlipX(false);
		if(m_delegate) m_delegate->uiLeftPressed();
		break;
	case DIR_RIGHT:
		m_dpadSprite->setDisplayFrame(m_dpadDwnFrame);
		m_dpadSprite->setFlipX(true);
		if(m_delegate) m_delegate->uiRightPressed();
		break;
	case DIR_UP:
		m_jumpSprite->setDisplayFrame(m_jumpDwnFrame);
		if(m_delegate) m_delegate->uiUpPressed();
		break;
	case DIR_DOWN:
		m_attackSprite->setDisplayFrame(m_attackDwnFrame);
		if(m_delegate) m_delegate->uiDownPressed();
		break;
	default:
		break;
	}
}

void FL_UILayer::releaseDirection(Direction dir) {
	switch (dir) {
	case DIR_LEFT:
		m_dpadSprite->setDisplayFrame(m_dpadIdleFrame);
		m_dpadSprite->setFlipX(false);
		if(m_delegate) m_delegate->uiLeftReleased();
		break;
	case DIR_RIGHT:
		m_dpadSprite->setDisplayFrame(m_dpadIdleFrame);
		m_dpadSprite->setFlipX(false);
		if(m_delegate) m_delegate->uiRightReleased();
		break;
	case DIR_UP:
		m_jumpSprite->setDisplayFrame(m_jumpIdleFrame);
		if(m_delegate) m_delegate->uiUpReleased();
		break;
	case DIR_DOWN:
		m_attackSprite->setDisplayFrame(m_attackIdleFrame);
		if(m_delegate) m_delegate->uiDownReleased();
		break;
	default:
		break;
	}
}

bool FL_UILayer::ccTouchBegan(CCTouch* touch, CCEvent* event) {
	CCPoint loc = this->convertTouchToNodeSpace(touch);
	Direction dir = hitTest(loc);
	if(dir == DIR_NONE) return false;

	pressDirection(dir);
	m_activeTouches[touch] = dir;
	return true;
}

void FL_UILayer::ccTouchMoved(CCTouch* touch, CCEvent* event) {
	std::map<CCTouch*, Direction>::iterator it = m_activeTouches.find(touch);
	if (it == m_activeTouches.end()) return;
	if (it->second != DIR_LEFT && it->second != DIR_RIGHT) return;

	CCPoint loc = this->convertTouchToNodeSpace(touch);
	Direction newDir = hitTest(loc);

	if(newDir != it->second && (newDir == DIR_LEFT || newDir == DIR_RIGHT)) {
		releaseDirection(it->second);
		pressDirection(newDir);
		it->second = newDir;
	}
}

void FL_UILayer::ccTouchEnded(CCTouch* touch, CCEvent* event) {
	std::map<CCTouch*, Direction>::iterator it = m_activeTouches.find(touch);
	if(it == m_activeTouches.end()) return;

	releaseDirection(it->second);
	m_activeTouches.erase(it);
}

void FL_UILayer::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
	ccTouchEnded(touch, event);
}