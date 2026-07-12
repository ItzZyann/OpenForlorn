#pragma once
#include "../FL_Block.h"
class MovingBlockGate : public FL_Block {
public: static MovingBlockGate* create(const Data&,const CCPoint& end,float duration,const std::string& easing);void activateObject(bool deactivating);void pauseObject();void resumeObject();
private: MovingBlockGate():duration_(0){}CCPoint start_,end_;float duration_;std::string easing_;
};
