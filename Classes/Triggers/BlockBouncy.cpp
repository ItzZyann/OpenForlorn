#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#include "BlockBouncy.h"
#include <algorithm>
#include <cstring>

BlockBouncy::BlockBouncy():bounceX_(0),bounceY_(0),cooldown_(0),baseScaleX_(1),baseScaleY_(1){}
BlockBouncy* BlockBouncy::create(const Data&d,float x,float y){BlockBouncy*b=new BlockBouncy;if(b&&b->initBouncy(d,x,y)){b->autorelease();return b;}delete b;return NULL;}
bool BlockBouncy::initBouncy(const Data&d,float x,float y){if(!FL_Block::init(d))return false;bounceX_=x;bounceY_=y;baseScaleX_=getScaleX();baseScaleY_=getScaleY();return true;}
void BlockBouncy::updateBouncy(float dt){cooldown_=std::max(0.f,cooldown_-dt);}
void BlockBouncy::playBounceAnimation(){
    if(cooldown_>0.f)return;cooldown_=.22f;stopActionByTag(114);setScaleX(baseScaleX_);setScaleY(baseScaleY_);
    CCActionInterval*a=CCSequence::create(
        CCScaleTo::create(.075f,baseScaleX_*1.2f,baseScaleY_*.6f),
        CCScaleTo::create(.15f,baseScaleX_*.6f,baseScaleY_*1.2f),
        CCScaleTo::create(.15f,baseScaleX_,baseScaleY_),NULL);
    a->setTag(114);runAction(a);
}
