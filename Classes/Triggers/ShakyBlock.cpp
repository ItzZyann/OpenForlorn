#include "ShakyBlock.h"
ShakyBlock* ShakyBlock::create(const Data&d,float delay){ShakyBlock*x=new ShakyBlock;if(x&&x->initShaky(d,delay)){x->autorelease();return x;}delete x;return NULL;}
bool ShakyBlock::initShaky(const Data&d,float delay){if(!FL_Block::init(d))return false;delay_=delay;spawn_=d.position;return true;}
void ShakyBlock::activateObject(bool deactivating){if(deactivating){stopAllActions();return;}CCActionInterval*shake=CCSequence::create(CCMoveBy::create(.04f,ccp(4,0)),CCMoveBy::create(.04f,ccp(-8,0)),CCMoveBy::create(.04f,ccp(4,0)),NULL);runAction(CCSequence::create(CCDelayTime::create(delay_),CCRepeat::create(shake,5),CCEaseIn::create(CCMoveBy::create(.45f,ccp(0,-500)),2.f),NULL));}
void ShakyBlock::resetObject(){stopAllActions();setPosition(spawn_);setVisible(true);}
