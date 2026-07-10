#pragma once

#include "incl.h"
#include "FL_CollisionWorld.h"

#include <vector>

class FL_Player : public CCNode {
public:
    static FL_Player* create(
        const CCPoint& spawnPosition,
        const CCSize& levelSize,
        const FL_CollisionWorld& collisionWorld
    );

    void setMoveInput(float direction);
    void requestJump();
    void step(float dt);
    void resetToSpawn();

    bool isGrounded() const;
    CCPoint getVelocity() const;

private:
    enum AnimationState {
        ANIMATION_IDLE,
        ANIMATION_RUN,
        ANIMATION_JUMP_UP,
        ANIMATION_JUMP_MIDDLE,
        ANIMATION_JUMP_FALL
    };

    FL_Player();

    bool initialize(
        const CCPoint& spawnPosition,
        const CCSize& levelSize,
        const FL_CollisionWorld& collisionWorld
    );

    void loadAnimationFrames();
    void changeAnimation(AnimationState state);
    void updateAnimation(float dt);
    void updateFacing();

    void simulateSubstep(float dt);
    float approach(float current, float target, float maximumDelta) const;

    CCSprite* m_sprite;
    std::vector<CCSpriteFrame*> m_idleFrames;
    std::vector<CCSpriteFrame*> m_runFrames;
    CCSpriteFrame* m_jumpUpFrame;
    CCSpriteFrame* m_jumpMiddleFrame;
    CCSpriteFrame* m_jumpFallFrame;

    AnimationState m_animationState;
    unsigned int m_animationFrame;
    float m_animationElapsed;

    FL_CollisionWorld m_collisionWorld;
    CCPoint m_spawnPosition;
    CCPoint m_velocity;
    CCSize m_levelSize;

    float m_moveInput;
    float m_bodyHalfWidth;
    float m_bodyHalfHeight;
    float m_horizontalSpeed;
    float m_groundAcceleration;
    float m_airAcceleration;
    float m_gravity;
    float m_jumpVelocity;

    bool m_jumpQueued;
    bool m_grounded;
    bool m_facingRight;
};
