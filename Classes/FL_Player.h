#pragma once

#include "incl.h"
#include "FL_CollisionWorld.h"
#include "FL_PlayerStance.h"

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
    void requestRoll(int direction);
    void requestStanceSwitch();
    bool consumeAttackStrike(CCRect& outBounds);
    int getAttackDamage() const;
    void step(float dt);
    void resetToSpawn();

    bool takeDamage(int amount);
    bool takeDamage(int amount, const CCPoint& sourcePosition, float bounceX, float bounceY);
    int getHealth() const;
    int getMaxHealth() const;
    bool isInvulnerable() const;
    CCRect getCollisionBounds();
    void rideMovingBlock(const CCPoint& delta, float platformTop);
    void landOnMovingBlock(float platformTop);
    void setPushing(bool pushing);
    void setPulling(bool pulling, bool faceRight);
    void propel(float sourceBounceX, float sourceBounceY);

    bool isGrounded() const;
    CCPoint getVelocity() const;
    bool isFacingRight() const;
    bool isMeleeAttacking() const { return m_currentAttackMelee; }
    bool getAttackFacingRight() const;
    bool isFireStance() const;
    FL_PlayerStanceType getStance() const;
    FL_PlayerStanceType getAttackStance() const;
    void setCheckpoint(const CCPoint& position);
    void respawn();
    void teleportTo(const CCPoint& position);

private:
    enum AnimationState {
        ANIMATION_IDLE,
        ANIMATION_RUN,
        ANIMATION_ATTACK_1,
        ANIMATION_ATTACK_2,
        ANIMATION_ATTACK_AIR,
        ANIMATION_ROLL_IN,
        ANIMATION_ROLL,
        ANIMATION_ROLL_OUT,
        ANIMATION_GET_HIT,
        ANIMATION_GET_HIT_AIR,
        ANIMATION_JUMP_UP,
        ANIMATION_JUMP_MIDDLE,
        ANIMATION_JUMP_FALL
        , ANIMATION_PUSH, ANIMATION_PULL
    };

    FL_Player();

    bool initialize(
        const CCPoint& spawnPosition,
        const CCSize& levelSize,
        const FL_CollisionWorld& collisionWorld
    );

    void loadAnimationFrames();
    void loadAnimationFramesForPrefix(
        const char* prefix,
        std::vector<CCSpriteFrame*>& idleFrames,
        std::vector<CCSpriteFrame*>& runFrames,
        std::vector<CCSpriteFrame*>& attack1Frames,
        std::vector<CCSpriteFrame*>& attack2Frames,
        std::vector<CCSpriteFrame*>& attackAirFrames,
        std::vector<CCSpriteFrame*>& toRollFrames,
        CCSpriteFrame*& rollFrame,
        std::vector<CCSpriteFrame*>& fromRollFrames,
        std::vector<CCSpriteFrame*>& getHitFrames,
        std::vector<CCSpriteFrame*>& getHitAirFrames,
        CCSpriteFrame*& jumpUpFrame,
        CCSpriteFrame*& jumpMiddleFrame,
        CCSpriteFrame*& jumpFallFrame
    );
    void changeAnimation(AnimationState state);
    void updateAnimation(float dt);
    void updateFacing();
    void updateDamageVisual();
    void startAttack();
    void startMeleeComboStage(int stage);
    void startRoll(int direction);
    void switchStance();
    void refreshCurrentAnimationFrame();
    void finishRoll();
    void setRollCollisionShape(bool enabled);
    void updateAttackStrike(float dt);
    void startHurtAnimation(bool forceAirAnimation);
    void applyDamageBounce(const CCPoint& sourcePosition, float bounceX, float bounceY);

    void simulateSubstep(float dt);
    float approach(float current, float target, float maximumDelta) const;
    const std::vector<CCSpriteFrame*>* attackFramesForState() const;
    const std::vector<CCSpriteFrame*>* rollFramesForState() const;
    const std::vector<CCSpriteFrame*>* hurtFramesForState() const;
    const std::vector<CCSpriteFrame*>& activeIdleFrames() const;
    const std::vector<CCSpriteFrame*>& activeRunFrames() const;
    const std::vector<CCSpriteFrame*>& activeAttack1Frames() const;
    const std::vector<CCSpriteFrame*>& activeAttack2Frames() const;
    const std::vector<CCSpriteFrame*>& activeAttackAirFrames() const;
    const std::vector<CCSpriteFrame*>& activeMelee1Frames() const;
    const std::vector<CCSpriteFrame*>& activeMelee2Frames() const;
    const std::vector<CCSpriteFrame*>& activeMelee3Frames() const;
    const std::vector<CCSpriteFrame*>& activeMeleeAirFrames() const;
    const std::vector<CCSpriteFrame*>& activeToRollFrames() const;
    const std::vector<CCSpriteFrame*>& activeFromRollFrames() const;
    const std::vector<CCSpriteFrame*>& activeGetHitFrames() const;
    const std::vector<CCSpriteFrame*>& activeGetHitAirFrames() const;
    CCSpriteFrame* activeRollFrame() const;
    CCSpriteFrame* activeJumpUpFrame() const;
    CCSpriteFrame* activeJumpMiddleFrame() const;
    CCSpriteFrame* activeJumpFallFrame() const;

    CCSprite* m_sprite;
    std::vector<CCSpriteFrame*> m_idleFrames;
    std::vector<CCSpriteFrame*> m_runFrames;
    std::vector<CCSpriteFrame*> m_attack1Frames;
    std::vector<CCSpriteFrame*> m_attack2Frames;
    std::vector<CCSpriteFrame*> m_attackAirFrames;
    std::vector<CCSpriteFrame*> m_melee1Frames;
    std::vector<CCSpriteFrame*> m_melee2Frames;
    std::vector<CCSpriteFrame*> m_melee3Frames;
    std::vector<CCSpriteFrame*> m_meleeAirFrames;
    std::vector<CCSpriteFrame*> m_toRollFrames;
    std::vector<CCSpriteFrame*> m_fromRollFrames;
    CCSpriteFrame* m_rollFrame;
    std::vector<CCSpriteFrame*> m_getHitFrames;
    std::vector<CCSpriteFrame*> m_getHitAirFrames;
    std::vector<CCSpriteFrame*> m_pushFrames;
    std::vector<CCSpriteFrame*> m_pullFrames;
    CCSpriteFrame* m_jumpUpFrame;
    CCSpriteFrame* m_jumpMiddleFrame;
    CCSpriteFrame* m_jumpFallFrame;

    std::vector<CCSpriteFrame*> m_fireIdleFrames;
    std::vector<CCSpriteFrame*> m_fireRunFrames;
    std::vector<CCSpriteFrame*> m_fireAttack1Frames;
    std::vector<CCSpriteFrame*> m_fireAttack2Frames;
    std::vector<CCSpriteFrame*> m_fireAttackAirFrames;
    std::vector<CCSpriteFrame*> m_fireMelee1Frames;
    std::vector<CCSpriteFrame*> m_fireMelee2Frames;
    std::vector<CCSpriteFrame*> m_fireMelee3Frames;
    std::vector<CCSpriteFrame*> m_fireMeleeAirFrames;
    std::vector<CCSpriteFrame*> m_fireToRollFrames;
    std::vector<CCSpriteFrame*> m_fireFromRollFrames;
    std::vector<CCSpriteFrame*> m_fireGetHitFrames;
    std::vector<CCSpriteFrame*> m_fireGetHitAirFrames;
    std::vector<CCSpriteFrame*> m_firePushFrames;
    std::vector<CCSpriteFrame*> m_firePullFrames;
    CCSpriteFrame* m_fireRollFrame;
    CCSpriteFrame* m_fireJumpUpFrame;
    CCSpriteFrame* m_fireJumpMiddleFrame;
    CCSpriteFrame* m_fireJumpFallFrame;

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
    float m_attackStrikeDelayRemaining;
    float m_rollCooldownRemaining;
    float m_rollRemaining;
    float m_rollOutRemaining;
    float m_invulnerabilityRemaining;
    float m_hurtControlLockRemaining;
    float m_stanceCooldownRemaining;

    int m_health;
    int m_maxHealth;

    bool m_jumpQueued;
    bool m_attackQueued;
    bool m_rollQueued;
    bool m_stanceSwitchQueued;
    bool m_attacking;
    bool m_hurtAnimating;
    bool m_rolling;
    bool m_rollRecovering;
    bool m_attackStrikeArmed;
    bool m_attackStrikeReady;
    bool m_nextGroundAttackIsFirst;
    bool m_currentAttackMelee;
    bool m_meleeAttackQueued;
    bool m_grounded;
    bool m_dynamicGrounded;
    bool m_pushing;
    bool m_pulling;
    bool m_pullFacingRight;
    bool m_propelled;
    bool m_propelHorizontalFade;
    bool m_facingRight;
    bool m_attackFacingRight;
    FL_PlayerStanceType m_stance;
    FL_PlayerStanceType m_attackStance;
    int m_queuedRollDirection;
    int m_rollDirection;
    int m_meleeComboStage;
};
