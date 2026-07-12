#include "RollingBlock.h"
RollingBlock* RollingBlock::create(const Data&d,float s){RollingBlock*x=new RollingBlock;if(x&&x->initRolling(d,s)){x->autorelease();return x;}delete x;return NULL;}
bool RollingBlock::initRolling(const Data&d,float s){if(!initMovable(d))return false;speed_=s;velocity_.x=s;return true;}
void RollingBlock::updateRolling(float dt){if(!active_)return;setRotation(getRotation()-speed_*dt);}
