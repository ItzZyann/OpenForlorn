#pragma once
#include "MovableBlock.h"
class RollingBlock : public MovableBlock {
public: static RollingBlock* create(const Data& data,float speed);bool initRolling(const Data&,float speed);void updateRolling(float dt);
private: RollingBlock():speed_(0){} float speed_;
};
