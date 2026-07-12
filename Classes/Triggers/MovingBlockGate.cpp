#include "MovingBlockGate.h"
MovingBlockGate* MovingBlockGate::create(const Data&d,const CCPoint&e,float duration,const std::string& easing){MovingBlockGate*x=new MovingBlockGate;if(x&&x->FL_Block::init(d)){x->start_=d.position;x->end_=e;x->duration_=duration;x->easing_=easing;x->autorelease();return x;}delete x;return NULL;}
void MovingBlockGate::activateObject(bool d){if(!d)resumeObject();else pauseObject();}
void MovingBlockGate::pauseObject(){stopAllActions();runAction(CCMoveTo::create(duration_,start_));}
void MovingBlockGate::resumeObject(){stopAllActions();runAction(CCMoveTo::create(duration_,end_));}
