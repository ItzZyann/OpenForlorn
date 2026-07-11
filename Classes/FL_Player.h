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
    void requestAttack();
    void step(float dt);
    void resetToSpawn();

    bool takeDamage(int amount);
    bool takeDamage(int amount, const CCPoint& sourcePosition, float bounceX, float bounceY);
    int getHealth() const;
    int getMaxHealth() const;
    bool isInvulnerable() const;
    CCRect getCollisionBounds();

    bool isGrounded() const;
    CCPoint getVelocity() const;

private:
    enum AnimationState {
        ANIMATION_IDLE,
        ANIMATION_RUN,
        ANIMATION_ATTACK_1,
        ANIMATION_ATTACK_2,
        ANIMATION_ATTACK_AIR,
        ANIMATION_GET_HIT,
        ANIMATION_GET_HIT_AIR,
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
    void updateDamageVisual();
    void startAttack();
    void startHurtAnimation(bool forceAirAnimation);
    void applyDamageBounce(const CCPoint& sourcePosition, float bounceX, float bounceY);

    void simulateSubstep(float dt);
    float approach(float current, float target, float maximumDelta) const;
    const std::vector<CCSpriteFrame*>* attackFramesForState() const;
    const std::vector<CCSpriteFrame*>* hurtFramesForState() const;

    CCSprite* m_sprite;
    std::vector<CCSpriteFrame*> m_idleFrames;
    std::vector<CCSpriteFrame*> m_runFrames;
    std::vector<CCSpriteFrame*> m_attack1Frames;
    std::vector<CCSpriteFrame*> m_attack2Frames;
    std::vector<CCSpriteFrame*> m_attackAirFrames;
    std::vector<CCSpriteFrame*> m_getHitFrames;
    std::vector<CCSpriteFrame*> m_getHitAirFrames;
    CCSpriteFrame* m_jumpUpFrame;
    CCSpriteFrame* m_jumpMiddleFrame;
    CCSpriteFrame* m_jumpFallFrame;

    AnimationState m_animationState;
    unsigned int m_animationFrame;
    float m_animationElapsed;
    int m_idleFrameDirection;

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
    float m_attackCooldownRemaining;
    float m_invulnerabilityRemaining;
    float m_hurtControlLockRemaining;

    int m_health;
    int m_maxHealth;

    bool m_jumpQueued;
    bool m_attackQueued;
    bool m_attacking;
    bool m_hurtAnimating;
    bool m_nextGroundAttackIsFirst;
    bool m_grounded;
    bool m_facingRight;
};
