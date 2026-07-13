#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#include "MovableBlock.h"
#include <cstring>

MovableBlock::MovableBlock():spawnPosition_(CCPointZero),velocity_(CCPointZero),lastDelta_(CCPointZero),active_(false),trapControlled_(false),ethereal_(false),dead_(false){}
MovableBlock* MovableBlock::create(const Data&d){MovableBlock*x=new MovableBlock;if(x&&x->initMovable(d)){x->autorelease();return x;}delete x;return NULL;}
bool MovableBlock::initMovable(const Data&d){if(!FL_Block::init(d))return false;spawnPosition_=d.position;active_=true;return true;}
void MovableBlock::activateObject(bool deactivating){active_=!deactivating;setVisible(active_);}
void MovableBlock::deactivateObject(bool reset){active_=false;if(reset)setPosition(spawnPosition_);stopAllActions();}
void MovableBlock::resetObject(){velocity_=CCPointZero;lastDelta_=CCPointZero;dead_=false;setPosition(spawnPosition_);activateObject(false);}
void MovableBlock::updateMovable(float dt,const FL_CollisionWorld&world){
    if(!active_||dead_||dt<=0)return;
    dt=std::min(dt,.05f);velocity_.y=std::max(-700.f,velocity_.y-980.f*dt);
    const CCRect b=boundingBox();const float hw=b.size.width*.5f,hh=b.size.height*.5f;
    const CCPoint old=getPosition();CCPoint next=ccpAdd(old,ccpMult(velocity_,dt));
    FL_CollisionWorld::MoveResult result;world.moveAabb(old,next,velocity_,hw,hh,result);
    if(result.grounded)velocity_.y=0;lastDelta_=ccpSub(next,old);setPosition(next);
}
bool MovableBlock::pushHorizontal(float distance,const FL_CollisionWorld&world,const std::vector<MovableBlock*>&blocks){
    if(!active_||std::fabs(distance)<.001f)return false;
    const CCRect b=boundingBox();const float hw=b.size.width*.5f,hh=b.size.height*.5f;
    const CCPoint old=getPosition();CCPoint next=ccp(old.x+distance,old.y);CCPoint v=ccp(distance,0);
    FL_CollisionWorld::MoveResult result;world.moveAabb(old,next,v,hw,hh,result);
    CCRect proposed=boundingBox();proposed.origin.x+=next.x-old.x;
    for(size_t i=0;i<blocks.size();++i)if(blocks[i]&&blocks[i]!=this&&blocks[i]->active()&&proposed.intersectsRect(blocks[i]->boundingBox()))return false;
    if(std::fabs(next.x-old.x)<.001f)return false;lastDelta_=ccpSub(next,old);setPosition(next);velocity_.x=(next.x-old.x);return true;
}
