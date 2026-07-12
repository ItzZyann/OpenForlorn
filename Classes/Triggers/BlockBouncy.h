#pragma once
#include "../FL_Block.h"

class BlockBouncy : public FL_Block {
public:
    static BlockBouncy* create(const Data& data, float bounceX, float bounceY);
    bool initBouncy(const Data& data, float bounceX, float bounceY);
    void updateBouncy(float dt);
    bool canBounce() const { return cooldown_ <= 0.f; }
    void playBounceAnimation();
    float bounceX() const { return bounceX_; }
    float bounceY() const { return bounceY_; }
private:
    BlockBouncy();
    float bounceX_, bounceY_, cooldown_, baseScaleX_, baseScaleY_;
};
