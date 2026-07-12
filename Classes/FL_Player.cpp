#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#include "FL_Player.h"
#include "particles/FL_PlayerParticles.h"
#include "FL_PlayLayer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#if defined(__clang__) || defined(__GNUC__)
#define FL_CPP_WEAK __attribute__((weak))
#else
#define FL_CPP_WEAK
#endif

USING_NS_CC;

namespace {
    namespace FL_PlayerDetail {

        const char* playerAtlas() {
            return "Frost_Main_Character_spritesheet_01-hd.plist";
        }

        const char* firePlayerAtlas() {
            return "Fire_Main_Character_spritesheet_01-hd.plist";
        }

        float playerVisualScale() { return 0.5f; }
        float playerVisualOffsetY() { return -7.0f; }
        float maximumPhysicsStep() { return 1.0f / 120.0f; }
        float maximumFrameTime() { return 1.0f / 15.0f; }
        float idleFrameDelay() { return 0.14f; }
        float runFrameDelay() { return 0.083f; }
        float attackFrameDelay() { return 0.064f; }
        float hurtFrameDelay() { return 0.095f; }
        float hurtFirstFrameDelay() { return 0.08f; }
        float hurtSecondFrameDelay() { return 0.16f; }
        float hurtAirFirstFrameDelay() { return 0.09f; }
        float hurtAirSecondFrameDelay() { return 0.20f; }
        float hurtLockDuration() { return 0.20f; }
        float attackCooldown() { return 0.4f; }
        float attackStrikeDelay() { return 0.12f; }
        float attackReach() { return 100.0f; }
        float rollSpeed() { return 300.0f; }
        float rollDuration() { return 0.40f; }
        float rollCooldown() { return 0.40f; }
        float rollOutDuration() { return 0.075f; }
        float rollInFrameDelay() { return 0.04f; }
        float rollOutFrameDelay() { return 0.025f; }
        float normalBodyHalfHeight() { return 49.0f; }
        float rollBodyHalfHeight() { return 25.0f; }
        float rollGroundSnapDrop() { return 28.0f; }
        float rollGroundSnapRise() { return 28.0f; }
        float stanceCooldown() { return 0.20f; }

        CCSpriteFrame* frameByName(const char* name) {
            return CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(name);
        }

        void appendFrame(
            std::vector<CCSpriteFrame*>& output,
            const char* format,
            int frame
        ) {
            char frameName[128];
#if defined(_MSC_VER) && _MSC_VER < 1900
            _snprintf(frameName, sizeof(frameName), format, frame);
            frameName[sizeof(frameName) - 1] = '\0';
#else
            std::snprintf(frameName, sizeof(frameName), format, frame);
#endif
            CCSpriteFrame* spriteFrame = frameByName(frameName);
            if (spriteFrame) output.push_back(spriteFrame);
        }

        void appendNumberedFrames(
            std::vector<CCSpriteFrame*>& output,
            const char* format,
            int firstFrame,
            int lastFrame
        ) {
            for (int frame = firstFrame; frame <= lastFrame; ++frame) {
                appendFrame(output, format, frame);
            }
        }

        float clampFloat(float value, float minimum, float maximum) {
            return std::max(minimum, std::min(maximum, value));
        }

    }
}

FL_CPP_WEAK FL_Player* FL_Player::create(
    const CCPoint& spawnPosition,
    const CCSize& levelSize,
    const FL_CollisionWorld& collisionWorld
) {
    FL_Player* player = new FL_Player();
    if (player && player->initialize(spawnPosition, levelSize, collisionWorld)) {
        player->autorelease();
        return player;
    }
    delete player;
    return NULL;
}

FL_CPP_WEAK FL_Player::FL_Player()
    : m_sprite(NULL)
    , m_jumpUpFrame(NULL)
    , m_jumpMiddleFrame(NULL)
    , m_jumpFallFrame(NULL)
    , m_fireRollFrame(NULL)
    , m_fireJumpUpFrame(NULL)
    , m_fireJumpMiddleFrame(NULL)
    , m_fireJumpFallFrame(NULL)
    , m_rollFrame(NULL)
    , m_animationState(ANIMATION_IDLE)
    , m_animationFrame(0)
    , m_animationElapsed(0.0f)
    , m_idleFrameDirection(1)
    , m_spawnPosition(CCPointZero)
    , m_velocity(CCPointZero)
    , m_levelSize(CCSizeMake(1.0f, 1.0f))
    , m_moveInput(0.0f)
    , m_bodyHalfWidth(14.0f)
    , m_bodyHalfHeight(49.0f)
    , m_horizontalSpeed(155.0f)
    , m_groundAcceleration(1250.0f)
    , m_airAcceleration(720.0f)
    , m_gravity(-980.0f)
    , m_jumpVelocity(410.0f)
    , m_attackCooldownRemaining(0.0f)
    , m_attackStrikeDelayRemaining(0.0f)
    , m_rollCooldownRemaining(0.0f)
    , m_rollRemaining(0.0f)
    , m_rollOutRemaining(0.0f)
    , m_invulnerabilityRemaining(0.0f)
    , m_hurtControlLockRemaining(0.0f)
    , m_stanceCooldownRemaining(0.0f)
    , m_health(12)
    , m_maxHealth(12)
    , m_jumpQueued(false)
    , m_attackQueued(false)
    , m_rollQueued(false)
    , m_stanceSwitchQueued(false)
    , m_attacking(false)
    , m_hurtAnimating(false)
    , m_rolling(false)
    , m_rollRecovering(false)
    , m_attackStrikeArmed(false)
    , m_attackStrikeReady(false)
    , m_nextGroundAttackIsFirst(false)
    , m_currentAttackMelee(false)
    , m_meleeAttackQueued(false)
    , m_grounded(false)
    , m_dynamicGrounded(false)
    , m_pushing(false)
    , m_pulling(false)
    , m_pullFacingRight(true)
    , m_propelled(false)
    , m_propelHorizontalFade(false)
    , m_facingRight(true)
    , m_attackFacingRight(true)
    , m_stance(FL_PLAYER_STANCE_FROST)
    , m_attackStance(FL_PLAYER_STANCE_FROST)
    , m_queuedRollDirection(0)
    , m_rollDirection(1)
    , m_meleeComboStage(0) {
}

FL_CPP_WEAK bool FL_Player::initialize(
    const CCPoint& spawnPosition,
    const CCSize& levelSize,
    const FL_CollisionWorld& collisionWorld
) {
    if (!CCNode::init()) return false;

    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(FL_PlayerDetail::playerAtlas());
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(FL_PlayerDetail::firePlayerAtlas());
    m_stance = FL_PlayerStanceState::load();
    m_attackStance = m_stance;
    loadAnimationFrames();
    if (m_idleFrames.empty()) {
        CCLog("Player atlas %s does not contain Frost_Idle1 frames.", FL_PlayerDetail::playerAtlas());
        return false;
    }

    const std::vector<CCSpriteFrame*>& startupFrames = activeIdleFrames().empty() ? m_idleFrames : activeIdleFrames();
    m_sprite = CCSprite::createWithSpriteFrame(startupFrames.front());
    if (!m_sprite) return false;

    m_sprite->setScale(FL_PlayerDetail::playerVisualScale());
    m_sprite->setPosition(ccp(0.0f, FL_PlayerDetail::playerVisualOffsetY()));
    addChild(m_sprite);

    m_spawnPosition = spawnPosition;
    m_levelSize = levelSize;
    m_collisionWorld = collisionWorld;
    setContentSize(CCSizeMake(m_bodyHalfWidth * 2.0f, m_bodyHalfHeight * 2.0f));

    resetToSpawn();
    changeAnimation(ANIMATION_IDLE);
    return true;
}

FL_CPP_WEAK void FL_Player::loadAnimationFramesForPrefix(
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
) {
    char format[128];

#if defined(_MSC_VER) && _MSC_VER < 1900
#define FL_SNPRINTF _snprintf
#else
#define FL_SNPRINTF std::snprintf
#endif

    FL_SNPRINTF(format, sizeof(format), "%s_Idle1_Anim_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    FL_PlayerDetail::appendNumberedFrames(idleFrames, format, 1, 6);

    FL_SNPRINTF(format, sizeof(format), "%s_Run_Anim_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    FL_PlayerDetail::appendNumberedFrames(runFrames, format, 1, 8);

    FL_SNPRINTF(format, sizeof(format), "%s_Cast_MainAttack_1_Anim_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    FL_PlayerDetail::appendNumberedFrames(attack1Frames, format, 2, 9);
    if (attack1Frames.empty()) {
        FL_SNPRINTF(format, sizeof(format), "%s_Attack_1_Anim_%%03d.png", prefix);
        format[sizeof(format) - 1] = '\0';
        FL_PlayerDetail::appendNumberedFrames(attack1Frames, format, 1, 14);
    }

    FL_SNPRINTF(format, sizeof(format), "%s_Cast_MainAttack_2_Anim_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    FL_PlayerDetail::appendNumberedFrames(attack2Frames, format, 1, 8);
    if (attack2Frames.empty()) {
        FL_SNPRINTF(format, sizeof(format), "%s_Attack_2_Anim_%%03d.png", prefix);
        format[sizeof(format) - 1] = '\0';
        FL_PlayerDetail::appendNumberedFrames(attack2Frames, format, 1, 14);
    }

    const int airAttackFrames[] = { 1, 1, 1, 2, 3 };
    FL_SNPRINTF(format, sizeof(format), "%s_Cast_MainAttack_Air_Anim_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    for (unsigned int index = 0;
        index < sizeof(airAttackFrames) / sizeof(airAttackFrames[0]); ++index) {
        FL_PlayerDetail::appendFrame(attackAirFrames, format, airAttackFrames[index]);
    }
    if (attackAirFrames.empty()) {
        FL_SNPRINTF(format, sizeof(format), "%s_AttackAir_1_Anim_%%03d.png", prefix);
        format[sizeof(format) - 1] = '\0';
        FL_PlayerDetail::appendNumberedFrames(attackAirFrames, format, 1, 6);
    }

    FL_SNPRINTF(format, sizeof(format), "%s_toRoll_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    FL_PlayerDetail::appendFrame(toRollFrames, format, 2);
    if (toRollFrames.empty()) FL_PlayerDetail::appendFrame(toRollFrames, format, 1);

    char frameName[128];
    FL_SNPRINTF(frameName, sizeof(frameName), "%s_Roll_001.png", prefix);
    frameName[sizeof(frameName) - 1] = '\0';
    rollFrame = FL_PlayerDetail::frameByName(frameName);

    FL_SNPRINTF(format, sizeof(format), "%s_fromRoll_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    FL_PlayerDetail::appendNumberedFrames(fromRollFrames, format, 1, 3);

    FL_SNPRINTF(format, sizeof(format), "%s_GetHit_Anim_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    FL_PlayerDetail::appendNumberedFrames(getHitFrames, format, 1, 4);

    FL_SNPRINTF(format, sizeof(format), "%s_GetHit_Air_Anim_%%03d.png", prefix);
    format[sizeof(format) - 1] = '\0';
    FL_PlayerDetail::appendNumberedFrames(getHitAirFrames, format, 1, 2);

    FL_SNPRINTF(frameName, sizeof(frameName), "%s_Jump_up_001.png", prefix);
    frameName[sizeof(frameName) - 1] = '\0';
    jumpUpFrame = FL_PlayerDetail::frameByName(frameName);

    FL_SNPRINTF(frameName, sizeof(frameName), "%s_Jump_middle_001.png", prefix);
    frameName[sizeof(frameName) - 1] = '\0';
    jumpMiddleFrame = FL_PlayerDetail::frameByName(frameName);

    FL_SNPRINTF(frameName, sizeof(frameName), "%s_Jump_fall_001.png", prefix);
    frameName[sizeof(frameName) - 1] = '\0';
    jumpFallFrame = FL_PlayerDetail::frameByName(frameName);

#undef FL_SNPRINTF
}

FL_CPP_WEAK void FL_Player::loadAnimationFrames() {
    loadAnimationFramesForPrefix(
        "Frost",
        m_idleFrames,
        m_runFrames,
        m_attack1Frames,
        m_attack2Frames,
        m_attackAirFrames,
        m_toRollFrames,
        m_rollFrame,
        m_fromRollFrames,
        m_getHitFrames,
        m_getHitAirFrames,
        m_jumpUpFrame,
        m_jumpMiddleFrame,
        m_jumpFallFrame
    );

    loadAnimationFramesForPrefix(
        "Fire",
        m_fireIdleFrames,
        m_fireRunFrames,
        m_fireAttack1Frames,
        m_fireAttack2Frames,
        m_fireAttackAirFrames,
        m_fireToRollFrames,
        m_fireRollFrame,
        m_fireFromRollFrames,
        m_fireGetHitFrames,
        m_fireGetHitAirFrames,
        m_fireJumpUpFrame,
        m_fireJumpMiddleFrame,
        m_fireJumpFallFrame
    );

    FL_PlayerDetail::appendNumberedFrames(m_pushFrames, "Frost_Push_Anim_%03d.png", 1, 6);
    FL_PlayerDetail::appendNumberedFrames(m_firePushFrames, "Fire_Push_Anim_%03d.png", 1, 6);
    FL_PlayerDetail::appendNumberedFrames(m_pullFrames, "Frost_Pull_Anim_%03d.png", 1, 6);
    FL_PlayerDetail::appendNumberedFrames(m_firePullFrames, "Fire_Pull_Anim_%03d.png", 1, 6);

    // melee1/melee2/meleeAir in the original animation manager use the sword
    // attack frames, while the normal ranged attack uses Cast_MainAttack.
    const int melee1[] = {1,1,2,3,3,4,5,5,18,18,19};
    const int melee2[] = {6,6,7,8,8,9,10,20,20};
    const int melee3[] = {11,11,12,13,13,14,15,15,16,17,17};
    const int meleeAir[] = {1,2,2,3,4,4,5};
    for (unsigned int i=0;i<sizeof(melee1)/sizeof(melee1[0]);++i) {
        FL_PlayerDetail::appendFrame(m_melee1Frames,"Frost_Attack_1_Anim_%03d.png",melee1[i]);
        FL_PlayerDetail::appendFrame(m_fireMelee1Frames,"Fire_Attack_1_Anim_%03d.png",melee1[i]);
    }
    for (unsigned int i=0;i<sizeof(melee2)/sizeof(melee2[0]);++i) {
        FL_PlayerDetail::appendFrame(m_melee2Frames,"Frost_Attack_1_Anim_%03d.png",melee2[i]);
        FL_PlayerDetail::appendFrame(m_fireMelee2Frames,"Fire_Attack_1_Anim_%03d.png",melee2[i]);
    }
    for (unsigned int i=0;i<sizeof(melee3)/sizeof(melee3[0]);++i) {
        FL_PlayerDetail::appendFrame(m_melee3Frames,"Frost_Attack_1_Anim_%03d.png",melee3[i]);
        FL_PlayerDetail::appendFrame(m_fireMelee3Frames,"Fire_Attack_1_Anim_%03d.png",melee3[i]);
    }
    for (unsigned int i=0;i<sizeof(meleeAir)/sizeof(meleeAir[0]);++i) {
        FL_PlayerDetail::appendFrame(m_meleeAirFrames,"Frost_AttackAir_1_Anim_%03d.png",meleeAir[i]);
        FL_PlayerDetail::appendFrame(m_fireMeleeAirFrames,"Fire_AttackAir_1_Anim_%03d.png",meleeAir[i]);
    }
}

FL_CPP_WEAK void FL_Player::setMoveInput(float direction) {
    m_moveInput = FL_PlayerDetail::clampFloat(direction, -1.0f, 1.0f);
}

FL_CPP_WEAK void FL_Player::requestJump() {
    m_jumpQueued = true;
}

FL_CPP_WEAK void FL_Player::requestAttack() {
    if (m_attacking && m_currentAttackMelee && m_meleeComboStage > 0 && m_meleeComboStage < 3) {
        m_meleeAttackQueued = true;
        return;
    }
    m_attackQueued = true;
}

FL_CPP_WEAK void FL_Player::requestRoll(int direction) {
    if (direction == 0) return;
    m_rollQueued = true;
    m_queuedRollDirection = direction < 0 ? -1 : 1;
}

FL_CPP_WEAK void FL_Player::requestStanceSwitch() {
    m_stanceSwitchQueued = true;
}

FL_CPP_WEAK bool FL_Player::consumeAttackStrike(CCRect& outBounds) {
    if (!m_attackStrikeReady) return false;
    m_attackStrikeReady = false;

    const CCPoint position = getPosition();
    const float width = FL_PlayerDetail::attackReach();
    const float height = m_bodyHalfHeight * 2.0f;
    const float x = m_attackFacingRight ? position.x : position.x - width;
    outBounds = CCRect(
        x,
        position.y - m_bodyHalfHeight,
        width,
        height
    );
    return true;
}

FL_CPP_WEAK int FL_Player::getAttackDamage() const {
    return 1;
}

FL_CPP_WEAK void FL_Player::startAttack() {
    const CCPoint position = getPosition();
    FL_PlayLayer* playLayer = static_cast<FL_PlayLayer*>(getParent());
    bool meleeFacingRight = m_facingRight;
    m_currentAttackMelee = playLayer && playLayer->findMeleeTarget(position, m_facingRight, meleeFacingRight);
    if (m_currentAttackMelee) {
        m_facingRight = meleeFacingRight;
        if (m_sprite) m_sprite->setFlipX(!m_facingRight);
        m_meleeAttackQueued = false;
        startMeleeComboStage(1);
        return;
    }

    const float reach = FL_PlayerDetail::attackReach();
    const float height = m_bodyHalfHeight * 2.0f;
    const float attackX = m_facingRight ? position.x : position.x - reach;
    const CCRect prospectiveBounds(attackX, position.y - m_bodyHalfHeight, reach, height);
	(void)prospectiveBounds;
	m_meleeComboStage = 0;

    AnimationState attackState = ANIMATION_ATTACK_AIR;
    const std::vector<CCSpriteFrame*>* attackFrames = m_currentAttackMelee
        ? &activeMeleeAirFrames() : &activeAttackAirFrames();

    if (m_grounded) {
        if (m_currentAttackMelee || m_nextGroundAttackIsFirst) {
            attackState = ANIMATION_ATTACK_1;
            attackFrames = m_currentAttackMelee ? &activeMelee1Frames() : &activeAttack1Frames();
        }
        else {
            attackState = ANIMATION_ATTACK_2;
            attackFrames = m_currentAttackMelee ? &activeMelee2Frames() : &activeAttack2Frames();
        }
        if (!m_currentAttackMelee) m_nextGroundAttackIsFirst = !m_nextGroundAttackIsFirst;
    }

    if (attackFrames->empty()) return;

    m_attacking = true;
    m_attackFacingRight = m_facingRight;
    m_attackStance = m_stance;
    m_attackStrikeArmed = true;
    m_attackStrikeReady = false;
    m_attackStrikeDelayRemaining = FL_PlayerDetail::attackStrikeDelay();
    m_attackCooldownRemaining = FL_PlayerDetail::attackCooldown();
    changeAnimation(attackState);

    if (playLayer && !m_currentAttackMelee) {
        playLayer->spawnAttackProjectile(
            getPosition(),
            m_facingRight,
            !m_grounded,
            m_attackStance
        );
    }
}

FL_CPP_WEAK void FL_Player::startMeleeComboStage(int stage) {
	m_currentAttackMelee = true;
	m_meleeComboStage = std::max(1, std::min(3, stage));
	m_attacking = true;
	m_attackFacingRight = m_facingRight;
	m_attackStance = m_stance;
	m_attackStrikeArmed = true;
	m_attackStrikeReady = false;
	m_attackStrikeDelayRemaining = m_grounded ? 0.13f : 0.06f;
	m_attackCooldownRemaining = 0.0f;
	changeAnimation(m_grounded ? ANIMATION_ATTACK_1 : ANIMATION_ATTACK_AIR);
}

FL_CPP_WEAK void FL_Player::startRoll(int direction) {
    if (direction == 0 || m_rolling || m_rollRecovering ||
        m_hurtAnimating || m_attacking || m_rollCooldownRemaining > 0.0f) {
        return;
    }

    CCPoint groundedPosition = getPosition();
    bool canRollOnGround = m_grounded;
    if (!canRollOnGround) {
        canRollOnGround = m_collisionWorld.snapToGround(
            groundedPosition,
            m_bodyHalfWidth,
            m_bodyHalfHeight,
            FL_PlayerDetail::rollGroundSnapDrop(),
            FL_PlayerDetail::rollGroundSnapRise()
        );
        if (canRollOnGround) {
            setPosition(groundedPosition);
            m_grounded = true;
        }
    }
    if (!canRollOnGround) return;

    m_rollDirection = direction < 0 ? -1 : 1;
    m_facingRight = m_rollDirection > 0;
    if (m_sprite) {
        m_sprite->stopAllActions();
        m_sprite->setRotation(0.0f);
        m_sprite->setOpacity(255);
        m_sprite->setFlipX(!m_facingRight);
        m_sprite->runAction(CCRotateBy::create(
            FL_PlayerDetail::rollDuration(),
            m_facingRight ? 360.0f : -360.0f
        ));
    }

    setRollCollisionShape(true);
    m_rolling = true;
    m_rollRecovering = false;
    m_rollRemaining = FL_PlayerDetail::rollDuration();
    m_rollOutRemaining = 0.0f;
    m_rollCooldownRemaining = FL_PlayerDetail::rollCooldown();
    m_attackQueued = false;
    m_jumpQueued = false;
    m_attacking = false;
    m_attackStrikeArmed = false;
    m_attackStrikeReady = false;
    m_velocity.x = FL_PlayerDetail::rollSpeed() * static_cast<float>(m_rollDirection);
    m_velocity.y = 0.0f;
    m_grounded = true;
    m_invulnerabilityRemaining = std::max(
        m_invulnerabilityRemaining,
        FL_PlayerDetail::rollDuration() + FL_PlayerDetail::rollOutDuration()
    );

    if (!activeToRollFrames().empty()) changeAnimation(ANIMATION_ROLL_IN);
    else if (activeRollFrame()) changeAnimation(ANIMATION_ROLL);

    FL_PlayerParticles::spawnRollDust(getParent(), getPosition(), m_facingRight);
}

FL_CPP_WEAK void FL_Player::switchStance() {
    if (m_stanceCooldownRemaining > 0.0f) return;

    m_stance = FL_PlayerStanceState::toggle(m_stance);
    m_stanceCooldownRemaining = FL_PlayerDetail::stanceCooldown();
    FL_PlayerStanceState::save(m_stance);

    if (m_sprite) {
        // Pseudocode preserves current facing while swapping Frost/Fire sprites.
        m_sprite->setOpacity(255);
        ccBlendFunc normalBlend = { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
        m_sprite->setBlendFunc(normalBlend);
        m_sprite->setFlipX(!m_facingRight);
    }

    refreshCurrentAnimationFrame();

    FL_PlayerParticles::spawnStanceSwitch(
        getParent(),
        getPosition(),
        m_stance
    );
}

FL_CPP_WEAK void FL_Player::finishRoll() {
    if (!m_rolling) return;

    m_rolling = false;
    m_rollRecovering = true;
    m_rollOutRemaining = FL_PlayerDetail::rollOutDuration();
    m_velocity.x = 0.0f;
    m_velocity.y = 0.0f;
    setRollCollisionShape(false);
    if (m_sprite) {
        m_sprite->stopAllActions();
        m_sprite->setRotation(0.0f);
    }

    CCPoint groundedPosition = getPosition();
    if (m_collisionWorld.snapToGround(
        groundedPosition,
        m_bodyHalfWidth,
        m_bodyHalfHeight,
        12.0f,
        16.0f
    )) {
        setPosition(groundedPosition);
        m_grounded = true;
    }
    else {
        m_grounded = false;
    }

    if (!m_fromRollFrames.empty()) changeAnimation(ANIMATION_ROLL_OUT);
    else m_rollRecovering = false;
}

FL_CPP_WEAK void FL_Player::setRollCollisionShape(bool enabled) {
    const float targetHalfHeight = enabled
        ? FL_PlayerDetail::rollBodyHalfHeight()
        : FL_PlayerDetail::normalBodyHalfHeight();
    if (std::fabs(m_bodyHalfHeight - targetHalfHeight) < 0.01f) return;

    const float verticalShift = m_bodyHalfHeight - targetHalfHeight;
    CCPoint position = getPosition();
    position.y -= verticalShift;
    m_bodyHalfHeight = targetHalfHeight;
    setContentSize(CCSizeMake(m_bodyHalfWidth * 2.0f, m_bodyHalfHeight * 2.0f));
    setPosition(position);
}

FL_CPP_WEAK void FL_Player::updateAttackStrike(float dt) {
    if (!m_attackStrikeArmed) return;
    m_attackStrikeDelayRemaining -= dt;
    if (m_attackStrikeDelayRemaining <= 0.0f) {
        m_attackStrikeArmed = false;
        m_attackStrikeReady = true;
    }
}

FL_CPP_WEAK void FL_Player::step(float dt) {
    const float frameTime = FL_PlayerDetail::clampFloat(
        dt,
        0.0f,
        FL_PlayerDetail::maximumFrameTime()
    );

    m_attackCooldownRemaining = std::max(0.0f, m_attackCooldownRemaining - frameTime);
    m_rollCooldownRemaining = std::max(0.0f, m_rollCooldownRemaining - frameTime);
    m_invulnerabilityRemaining = std::max(0.0f, m_invulnerabilityRemaining - frameTime);
    m_hurtControlLockRemaining = std::max(0.0f, m_hurtControlLockRemaining - frameTime);
    m_stanceCooldownRemaining = std::max(0.0f, m_stanceCooldownRemaining - frameTime);
    updateAttackStrike(frameTime);
    updateDamageVisual();

    if (m_stanceSwitchQueued) {
        switchStance();
        m_stanceSwitchQueued = false;
    }

    if (m_rollQueued) {
        startRoll(m_queuedRollDirection);
        m_rollQueued = false;
        m_queuedRollDirection = 0;
    }

    if (m_attackQueued) {
        if (m_attacking && m_currentAttackMelee && m_meleeComboStage > 0 && m_meleeComboStage < 3) {
			m_meleeAttackQueued = true;
		}
		else if (!m_attacking && !m_hurtAnimating && !m_rolling && !m_rollRecovering &&
            m_attackCooldownRemaining <= 0.0f) {
            startAttack();
        }
        m_attackQueued = false;
    }

    if (frameTime <= 0.0f) {
        updateAnimation(0.0f);
        m_jumpQueued = false;
        return;
    }

    int substeps = static_cast<int>(std::ceil(
        frameTime / FL_PlayerDetail::maximumPhysicsStep()
    ));
    substeps = std::max(1, std::min(8, substeps));
    const float substepTime = frameTime / static_cast<float>(substeps);

    if (m_jumpQueued && m_grounded && !m_rolling && !m_rollRecovering) {
        m_velocity.y = m_jumpVelocity;
        m_grounded = false;
        m_dynamicGrounded = false;
    }
    m_jumpQueued = false;

    for (int substep = 0; substep < substeps; ++substep) {
        simulateSubstep(substepTime);
    }

    if (m_rolling) {
        m_rollRemaining -= frameTime;
        if (m_rollRemaining <= 0.0f) finishRoll();
    }
    if (m_rollRecovering) {
        m_rollOutRemaining -= frameTime;
        if (m_rollOutRemaining <= 0.0f) m_rollRecovering = false;
    }

    if (getPositionY() < -300.0f) {
        takeDamage(2);
        resetToSpawn();
    }
    updateFacing();
    updateAnimation(frameTime);
	// Dynamic support must be reported again by the platform on the next frame.
	// The post-physics contact pass does that after step() returns.
	m_dynamicGrounded = false;
}

FL_CPP_WEAK void FL_Player::simulateSubstep(float dt) {
    const float targetHorizontalVelocity = m_rolling
        ? FL_PlayerDetail::rollSpeed() * static_cast<float>(m_rollDirection)
        : m_moveInput * m_horizontalSpeed;
    const float acceleration = m_rolling
        ? FL_PlayerDetail::rollSpeed() * 12.0f
        : (m_grounded ? m_groundAcceleration : m_airAcceleration);
    if (m_hurtControlLockRemaining <= 0.0f && !m_propelled) {
        m_velocity.x = approach(m_velocity.x, targetHorizontalVelocity, acceleration * dt);
    }
    if (m_rolling) {
        m_velocity.y = 0.0f;
    }
    else {
        // This port renders/authorizes the player body at half the original
        // visual scale, so the original propelled trajectory is spatially 0.5x.
        m_velocity.y += (m_propelled ? -540.f : m_gravity) * dt;
    }

    const CCPoint oldPosition = getPosition();
    CCPoint newPosition = ccp(
        oldPosition.x + m_velocity.x * dt,
        oldPosition.y + m_velocity.y * dt
    );

    if (m_levelSize.width > m_bodyHalfWidth * 2.0f) {
        newPosition.x = FL_PlayerDetail::clampFloat(
            newPosition.x,
            m_bodyHalfWidth,
            m_levelSize.width - m_bodyHalfWidth
        );
    }

    FL_CollisionWorld::MoveResult collisionResult;
    m_collisionWorld.moveAabb(
        oldPosition,
        newPosition,
        m_velocity,
        m_bodyHalfWidth,
        m_bodyHalfHeight,
        collisionResult
    );

    if (m_levelSize.width > m_bodyHalfWidth * 2.0f) {
        newPosition.x = FL_PlayerDetail::clampFloat(
            newPosition.x,
            m_bodyHalfWidth,
            m_levelSize.width - m_bodyHalfWidth
        );
    }

    if (m_rolling) {
        CCPoint snappedPosition = newPosition;
        if (m_collisionWorld.snapToGround(
            snappedPosition,
            m_bodyHalfWidth,
            m_bodyHalfHeight,
            FL_PlayerDetail::rollGroundSnapDrop(),
            FL_PlayerDetail::rollGroundSnapRise()
        )) {
            newPosition = snappedPosition;
            m_grounded = true;
            m_velocity.x = FL_PlayerDetail::rollSpeed() * static_cast<float>(m_rollDirection);
            m_velocity.y = 0.0f;
        }
        else {
            // Pseudocode roll/dodge does not cancel when the player leaves the
            // floor; it keeps the horizontal roll impulse for the action duration.
            // This allows rolling across a small gap.  We only mark the player as
            // airborne here; finishRoll() is controlled by the roll timer.
            m_grounded = false;
            m_velocity.x = FL_PlayerDetail::rollSpeed() * static_cast<float>(m_rollDirection);
            m_velocity.y = 0.0f;
        }
    }
    else {
        m_grounded = collisionResult.grounded || (m_dynamicGrounded && m_velocity.y <= 0.0f);
        if (m_dynamicGrounded && m_velocity.y <= 0.0f) m_velocity.y = 0.0f;
    }

    setPosition(newPosition);
    if (m_propelHorizontalFade) {
        m_velocity.x *= std::pow(.95f, dt * 60.f);
        // Original fadeVelocity ends below 1.5 source units (90 px/s).
        // Stop damping at the original 1.5-unit threshold, but retain that
        // residual velocity. Otherwise the final part of an angled launch
        // abruptly turns vertical instead of continuing along its inertia.
        if (std::fabs(m_velocity.x) < 90.f) {
            m_velocity.x = m_velocity.x < 0.f ? -90.f : 90.f;
            m_propelHorizontalFade = false;
        }
    }
    if (m_grounded) { m_propelled = false; m_propelHorizontalFade = false; }
}

FL_CPP_WEAK float FL_Player::approach(
    float current,
    float target,
    float maximumDelta
) const {
    if (current < target) return std::min(current + maximumDelta, target);
    if (current > target) return std::max(current - maximumDelta, target);
    return target;
}

FL_CPP_WEAK void FL_Player::resetToSpawn() {
    CCPoint position = m_spawnPosition;
    m_bodyHalfHeight = FL_PlayerDetail::normalBodyHalfHeight();
    setContentSize(CCSizeMake(m_bodyHalfWidth * 2.0f, m_bodyHalfHeight * 2.0f));
    if (m_sprite) {
        m_sprite->stopAllActions();
        m_sprite->setRotation(0.0f);
        m_sprite->setOpacity(255);
    }
    m_velocity = CCPointZero;
    m_jumpQueued = false;
    m_attackQueued = false;
    m_rollQueued = false;
    m_stanceSwitchQueued = false;
    m_attacking = false;
    if (m_rolling || m_rollRecovering) setRollCollisionShape(false);
    if (m_sprite) {
        m_sprite->stopAllActions();
        m_sprite->setRotation(0.0f);
    }
    m_rolling = false;
    m_rollRecovering = false;
    m_attackStrikeArmed = false;
    m_attackStrikeReady = false;
    m_attackCooldownRemaining = 0.0f;
    m_attackStrikeDelayRemaining = 0.0f;
    m_rollCooldownRemaining = 0.0f;
    m_rollRemaining = 0.0f;
    m_rollOutRemaining = 0.0f;
    m_stanceCooldownRemaining = 0.0f;
    m_nextGroundAttackIsFirst = false;
	m_currentAttackMelee = false;
	m_meleeAttackQueued = false;
	m_meleeComboStage = 0;
    m_hurtAnimating = false;
    m_hurtControlLockRemaining = 0.0f;
    m_grounded = m_collisionWorld.snapToGround(
        position,
        m_bodyHalfWidth,
        m_bodyHalfHeight,
        64.0f,
        32.0f
    );
    setPosition(position);
}

FL_CPP_WEAK void FL_Player::setCheckpoint(const CCPoint& position) {
    m_spawnPosition = position;
}

FL_CPP_WEAK void FL_Player::respawn() {
    resetToSpawn();
}

FL_CPP_WEAK void FL_Player::teleportTo(const CCPoint& position) {
    // A teleport starts a fresh physics frame.  Carrying a platform contact or
    // a bouncy/push velocity through the portal makes the destination drift or
    // immediately enter the wrong airborne state.
    setPosition(position);
    m_velocity = CCPointZero;
    m_grounded = false;
    m_dynamicGrounded = false;
    m_pushing = false;
    m_pulling = false;
    m_propelled = false;
    m_propelHorizontalFade = false;
    m_jumpQueued = false;
    m_rollQueued = false;
}

FL_CPP_WEAK void FL_Player::updateFacing() {
    if (m_rolling) {
        m_facingRight = m_rollDirection > 0;
    }
    else if (m_moveInput > 0.05f) m_facingRight = true;
    else if (m_moveInput < -0.05f) m_facingRight = false;
    if (m_sprite) m_sprite->setFlipX(!m_facingRight);
}

FL_CPP_WEAK void FL_Player::updateDamageVisual() {
    if (!m_sprite) return;
    if (m_rolling || m_rollRecovering || m_invulnerabilityRemaining <= 0.0f) {
        m_sprite->setOpacity(255);
        return;
    }
    const int phase = static_cast<int>(m_invulnerabilityRemaining * 18.0f);
    m_sprite->setOpacity((phase & 1) ? 110 : 255);
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeIdleFrames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireIdleFrames.empty()) ? m_fireIdleFrames : m_idleFrames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeRunFrames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireRunFrames.empty()) ? m_fireRunFrames : m_runFrames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeAttack1Frames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireAttack1Frames.empty()) ? m_fireAttack1Frames : m_attack1Frames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeAttack2Frames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireAttack2Frames.empty()) ? m_fireAttack2Frames : m_attack2Frames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeAttackAirFrames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireAttackAirFrames.empty()) ? m_fireAttackAirFrames : m_attackAirFrames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeMelee1Frames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireMelee1Frames.empty()) ? m_fireMelee1Frames : m_melee1Frames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeMelee2Frames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireMelee2Frames.empty()) ? m_fireMelee2Frames : m_melee2Frames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeMelee3Frames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireMelee3Frames.empty()) ? m_fireMelee3Frames : m_melee3Frames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeMeleeAirFrames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireMeleeAirFrames.empty()) ? m_fireMeleeAirFrames : m_meleeAirFrames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeToRollFrames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireToRollFrames.empty()) ? m_fireToRollFrames : m_toRollFrames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeFromRollFrames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireFromRollFrames.empty()) ? m_fireFromRollFrames : m_fromRollFrames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeGetHitFrames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireGetHitFrames.empty()) ? m_fireGetHitFrames : m_getHitFrames;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>& FL_Player::activeGetHitAirFrames() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && !m_fireGetHitAirFrames.empty()) ? m_fireGetHitAirFrames : m_getHitAirFrames;
}

FL_CPP_WEAK CCSpriteFrame* FL_Player::activeRollFrame() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && m_fireRollFrame) ? m_fireRollFrame : m_rollFrame;
}

FL_CPP_WEAK CCSpriteFrame* FL_Player::activeJumpUpFrame() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && m_fireJumpUpFrame) ? m_fireJumpUpFrame : m_jumpUpFrame;
}

FL_CPP_WEAK CCSpriteFrame* FL_Player::activeJumpMiddleFrame() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && m_fireJumpMiddleFrame) ? m_fireJumpMiddleFrame : m_jumpMiddleFrame;
}

FL_CPP_WEAK CCSpriteFrame* FL_Player::activeJumpFallFrame() const {
    return (m_stance == FL_PLAYER_STANCE_FIRE && m_fireJumpFallFrame) ? m_fireJumpFallFrame : m_jumpFallFrame;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>* FL_Player::attackFramesForState() const {
    if (m_currentAttackMelee && m_animationState != ANIMATION_ATTACK_AIR) {
		if (m_meleeComboStage >= 3) return &activeMelee3Frames();
		if (m_meleeComboStage == 2) return &activeMelee2Frames();
		return &activeMelee1Frames();
	}
    if (m_animationState == ANIMATION_ATTACK_1) return &activeAttack1Frames();
    if (m_animationState == ANIMATION_ATTACK_2) return &activeAttack2Frames();
    if (m_animationState == ANIMATION_ATTACK_AIR) return m_currentAttackMelee ? &activeMeleeAirFrames() : &activeAttackAirFrames();
    return NULL;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>* FL_Player::rollFramesForState() const {
    if (m_animationState == ANIMATION_ROLL_IN) return &activeToRollFrames();
    if (m_animationState == ANIMATION_ROLL_OUT) return &activeFromRollFrames();
    return NULL;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>* FL_Player::hurtFramesForState() const {
    if (m_animationState == ANIMATION_GET_HIT) return &activeGetHitFrames();
    if (m_animationState == ANIMATION_GET_HIT_AIR) return &activeGetHitAirFrames();
    return NULL;
}

FL_CPP_WEAK void FL_Player::startHurtAnimation(bool forceAirAnimation) {
    const bool useAirHit = ((forceAirAnimation || !m_grounded) && !activeGetHitAirFrames().empty());
    const std::vector<CCSpriteFrame*>& frames = useAirHit ? activeGetHitAirFrames() : activeGetHitFrames();
    if (frames.empty()) return;

    m_attacking = false;
    if (m_rolling || m_rollRecovering) setRollCollisionShape(false);
    if (m_sprite) {
        m_sprite->stopAllActions();
        m_sprite->setRotation(0.0f);
    }
    m_rolling = false;
    m_rollRecovering = false;
    m_attackStrikeArmed = false;
    m_attackStrikeReady = false;
    m_hurtAnimating = true;
    changeAnimation(useAirHit ? ANIMATION_GET_HIT_AIR : ANIMATION_GET_HIT);
}

FL_CPP_WEAK void FL_Player::refreshCurrentAnimationFrame() {
    if (!m_sprite) return;

    CCSpriteFrame* frame = NULL;
    if (m_animationState == ANIMATION_IDLE && !activeIdleFrames().empty()) frame = activeIdleFrames()[std::min<unsigned int>(m_animationFrame, activeIdleFrames().size() - 1)];
    else if (m_animationState == ANIMATION_RUN && !activeRunFrames().empty()) frame = activeRunFrames()[std::min<unsigned int>(m_animationFrame, activeRunFrames().size() - 1)];
    else if (m_animationState == ANIMATION_ATTACK_1 || m_animationState == ANIMATION_ATTACK_2 || m_animationState == ANIMATION_ATTACK_AIR) {
        const std::vector<CCSpriteFrame*>* attack = attackFramesForState();
        if (attack && !attack->empty()) frame = (*attack)[std::min<unsigned int>(m_animationFrame, attack->size() - 1)];
    }
    else if (m_animationState == ANIMATION_ROLL_IN && !activeToRollFrames().empty()) frame = activeToRollFrames()[std::min<unsigned int>(m_animationFrame, activeToRollFrames().size() - 1)];
    else if (m_animationState == ANIMATION_ROLL && activeRollFrame()) frame = activeRollFrame();
    else if (m_animationState == ANIMATION_ROLL_OUT && !activeFromRollFrames().empty()) frame = activeFromRollFrames()[std::min<unsigned int>(m_animationFrame, activeFromRollFrames().size() - 1)];
    else if (m_animationState == ANIMATION_GET_HIT && !activeGetHitFrames().empty()) frame = activeGetHitFrames()[std::min<unsigned int>(m_animationFrame, activeGetHitFrames().size() - 1)];
    else if (m_animationState == ANIMATION_GET_HIT_AIR && !activeGetHitAirFrames().empty()) frame = activeGetHitAirFrames()[std::min<unsigned int>(m_animationFrame, activeGetHitAirFrames().size() - 1)];
    else if (m_animationState == ANIMATION_JUMP_UP) frame = activeJumpUpFrame();
    else if (m_animationState == ANIMATION_JUMP_MIDDLE) frame = activeJumpMiddleFrame();
    else if (m_animationState == ANIMATION_JUMP_FALL) frame = activeJumpFallFrame();
    else if (m_animationState == ANIMATION_PUSH) {
        const std::vector<CCSpriteFrame*>& push = m_stance == FL_PLAYER_STANCE_FIRE ? m_firePushFrames : m_pushFrames;
        if (!push.empty()) frame = push[std::min<unsigned int>(m_animationFrame, push.size() - 1)];
    }
    else if (m_animationState == ANIMATION_PULL) {
        const std::vector<CCSpriteFrame*>& pull = m_stance == FL_PLAYER_STANCE_FIRE ? m_firePullFrames : m_pullFrames;
        if (!pull.empty()) frame = pull[std::min<unsigned int>(m_animationFrame, pull.size() - 1)];
    }

    if (frame) m_sprite->setDisplayFrame(frame);
}

FL_CPP_WEAK void FL_Player::changeAnimation(AnimationState state) {
    if (state == m_animationState && m_animationFrame == 0 && m_animationElapsed == 0.0f) return;

    m_animationState = state;
    m_animationFrame = 0;
    m_animationElapsed = 0.0f;
    m_idleFrameDirection = 1;

    CCSpriteFrame* frame = NULL;
    if (state == ANIMATION_IDLE && !activeIdleFrames().empty()) frame = activeIdleFrames().front();
    else if (state == ANIMATION_RUN && !activeRunFrames().empty()) frame = activeRunFrames().front();
    else if (state == ANIMATION_ATTACK_1 || state == ANIMATION_ATTACK_2 || state == ANIMATION_ATTACK_AIR) {
        const std::vector<CCSpriteFrame*>* attack = attackFramesForState();
        if (attack && !attack->empty()) frame = attack->front();
    }
    else if (state == ANIMATION_ROLL_IN && !activeToRollFrames().empty()) frame = activeToRollFrames().front();
    else if (state == ANIMATION_ROLL && activeRollFrame()) frame = activeRollFrame();
    else if (state == ANIMATION_ROLL_OUT && !activeFromRollFrames().empty()) frame = activeFromRollFrames().front();
    else if (state == ANIMATION_GET_HIT && !activeGetHitFrames().empty()) frame = activeGetHitFrames().front();
    else if (state == ANIMATION_GET_HIT_AIR && !activeGetHitAirFrames().empty()) frame = activeGetHitAirFrames().front();
    else if (state == ANIMATION_JUMP_UP) frame = activeJumpUpFrame();
    else if (state == ANIMATION_JUMP_MIDDLE) frame = activeJumpMiddleFrame();
    else if (state == ANIMATION_JUMP_FALL) frame = activeJumpFallFrame();
    else if (state == ANIMATION_PUSH) {
        const std::vector<CCSpriteFrame*>& push = m_stance == FL_PLAYER_STANCE_FIRE ? m_firePushFrames : m_pushFrames;
        if (!push.empty()) frame = push.front();
    }
    else if (state == ANIMATION_PULL) {
        const std::vector<CCSpriteFrame*>& pull = m_stance == FL_PLAYER_STANCE_FIRE ? m_firePullFrames : m_pullFrames;
        if (!pull.empty()) frame = pull.front();
    }

    if (frame && m_sprite) m_sprite->setDisplayFrame(frame);
}

FL_CPP_WEAK void FL_Player::updateAnimation(float dt) {
    if (m_hurtAnimating) {
        const std::vector<CCSpriteFrame*>* hurtFrames = hurtFramesForState();
        if (!hurtFrames || hurtFrames->empty() || !m_sprite) {
            m_hurtAnimating = false;
        }
        else {
            m_animationElapsed += dt;
            while (m_hurtAnimating) {
                float frameDelay = FL_PlayerDetail::hurtFrameDelay();
                if (m_animationState == ANIMATION_GET_HIT) {
                    frameDelay = (m_animationFrame == 0)
                        ? FL_PlayerDetail::hurtFirstFrameDelay()
                        : ((m_animationFrame == 1)
                            ? FL_PlayerDetail::hurtSecondFrameDelay()
                            : FL_PlayerDetail::hurtFrameDelay());
                }
                else if (m_animationState == ANIMATION_GET_HIT_AIR) {
                    frameDelay = (m_animationFrame == 0)
                        ? FL_PlayerDetail::hurtAirFirstFrameDelay()
                        : FL_PlayerDetail::hurtAirSecondFrameDelay();
                }
                if (m_animationElapsed < frameDelay) break;
                m_animationElapsed -= frameDelay;
                if (m_animationFrame + 1 < hurtFrames->size()) {
                    ++m_animationFrame;
                    m_sprite->setDisplayFrame((*hurtFrames)[m_animationFrame]);
                }
                else {
                    m_hurtAnimating = false;
                    break;
                }
            }
            if (m_hurtAnimating) return;
        }
    }

    if (m_rolling || m_rollRecovering) {
        const std::vector<CCSpriteFrame*>* rollFrames = rollFramesForState();
        if (m_animationState == ANIMATION_ROLL && activeRollFrame() && m_sprite) {
            m_sprite->setDisplayFrame(activeRollFrame());
            return;
        }
        if (!rollFrames || rollFrames->empty() || !m_sprite) {
            if (m_rolling && activeRollFrame()) changeAnimation(ANIMATION_ROLL);
            else m_rollRecovering = false;
        }
        else {
            m_animationElapsed += dt;
            const float rollFrameDelay = (m_animationState == ANIMATION_ROLL_OUT)
                ? FL_PlayerDetail::rollOutFrameDelay()
                : FL_PlayerDetail::rollInFrameDelay();
            while (m_animationElapsed >= rollFrameDelay) {
                m_animationElapsed -= rollFrameDelay;
                if (m_animationFrame + 1 < rollFrames->size()) {
                    ++m_animationFrame;
                    m_sprite->setDisplayFrame((*rollFrames)[m_animationFrame]);
                }
                else {
                    if (m_animationState == ANIMATION_ROLL_IN && m_rolling && activeRollFrame()) {
                        changeAnimation(ANIMATION_ROLL);
                    }
                    else if (m_animationState == ANIMATION_ROLL_OUT) {
                        m_rollRecovering = false;
                    }
                    break;
                }
            }
            if (m_rolling || m_rollRecovering) return;
        }
    }

    if (m_attacking) {
        const std::vector<CCSpriteFrame*>* attackFrames = attackFramesForState();
        if (!attackFrames || attackFrames->empty() || !m_sprite) {
            m_attacking = false;
        }
        else {
            m_animationElapsed += dt;
            const float frameDelay = m_currentAttackMelee ? 0.042f : FL_PlayerDetail::attackFrameDelay();
            while (m_animationElapsed >= frameDelay) {
                m_animationElapsed -= frameDelay;
                if (m_animationFrame + 1 < attackFrames->size()) {
                    ++m_animationFrame;
                    m_sprite->setDisplayFrame((*attackFrames)[m_animationFrame]);
                }
                else {
					if (m_currentAttackMelee && m_meleeAttackQueued && m_meleeComboStage < 3 && m_grounded) {
						m_meleeAttackQueued = false;
						startMeleeComboStage(m_meleeComboStage + 1);
					}
					else {
						m_attacking = false;
						m_meleeAttackQueued = false;
						m_meleeComboStage = 0;
					}
                    break;
                }
            }
            if (m_attacking) return;
        }
    }

    AnimationState desiredState = ANIMATION_IDLE;
    if (!m_grounded) {
        if (m_velocity.y > 80.0f) desiredState = ANIMATION_JUMP_UP;
        else if (m_velocity.y < -80.0f) desiredState = ANIMATION_JUMP_FALL;
        else desiredState = ANIMATION_JUMP_MIDDLE;
    }
    else if (m_pulling) desiredState = ANIMATION_PULL;
    else if (m_pushing) desiredState = ANIMATION_PUSH;
    else if (std::fabs(m_velocity.x) > 12.0f) {
        desiredState = ANIMATION_RUN;
    }

    if (desiredState != m_animationState) changeAnimation(desiredState);

    const std::vector<CCSpriteFrame*>* frames = NULL;
    float frameDelay = 0.1f;
    if (m_animationState == ANIMATION_IDLE) {
        frames = &activeIdleFrames();
        frameDelay = FL_PlayerDetail::idleFrameDelay();
    }
    else if (m_animationState == ANIMATION_RUN) {
        frames = &activeRunFrames();
        frameDelay = FL_PlayerDetail::runFrameDelay();
    }
    else if (m_animationState == ANIMATION_PUSH) {
        frames = &(m_stance == FL_PLAYER_STANCE_FIRE ? m_firePushFrames : m_pushFrames);
        frameDelay = 0.09f;
    }
    else if (m_animationState == ANIMATION_PULL) {
        frames = &(m_stance == FL_PLAYER_STANCE_FIRE ? m_firePullFrames : m_pullFrames);
        frameDelay = 0.09f;
    }

    if (!frames || frames->size() <= 1 || !m_sprite) return;

    m_animationElapsed += dt;
    while (m_animationElapsed >= frameDelay) {
        m_animationElapsed -= frameDelay;
        if (m_animationState == ANIMATION_IDLE) {
            int nextFrame = static_cast<int>(m_animationFrame) + m_idleFrameDirection;
            if (nextFrame >= static_cast<int>(frames->size())) {
                m_idleFrameDirection = -1;
                nextFrame = static_cast<int>(frames->size()) - 2;
            }
            else if (nextFrame < 0) {
                m_idleFrameDirection = 1;
                nextFrame = 1;
            }
            m_animationFrame = static_cast<unsigned int>(nextFrame);
        }
        else {
            m_animationFrame = (m_animationFrame + 1) % frames->size();
        }
        m_sprite->setDisplayFrame((*frames)[m_animationFrame]);
    }
}

FL_CPP_WEAK void FL_Player::applyDamageBounce(
    const CCPoint& sourcePosition,
    float bounceX,
    float bounceY
) {
    if (bounceX <= 0.0f && bounceY <= 0.0f) return;

    const CCPoint playerPosition = getPosition();
    float horizontal = std::max(0.0f, bounceX);
    if (sourcePosition.x >= playerPosition.x) horizontal = -horizontal;

    if (m_rolling || m_rollRecovering) setRollCollisionShape(false);
    if (m_sprite) {
        m_sprite->stopAllActions();
        m_sprite->setRotation(0.0f);
    }
    m_rolling = false;
    m_rollRecovering = false;
    m_velocity.x = horizontal;
    m_velocity.y = std::max(m_velocity.y, std::max(0.0f, bounceY));
    m_grounded = false;
    m_hurtControlLockRemaining = FL_PlayerDetail::hurtLockDuration();
}

FL_CPP_WEAK bool FL_Player::takeDamage(int amount) {
    const CCPoint position = getPosition();
    return takeDamage(amount, position, 0.0f, 0.0f);
}

FL_CPP_WEAK bool FL_Player::takeDamage(
    int amount,
    const CCPoint& sourcePosition,
    float bounceX,
    float bounceY
) {
    if (amount <= 0 || m_invulnerabilityRemaining > 0.0f || m_health <= 0) return false;

    m_health = std::max(0, m_health - amount);
    m_invulnerabilityRemaining = 0.85f;
    applyDamageBounce(sourcePosition, bounceX, bounceY);
    startHurtAnimation(bounceX > 0.0f || bounceY > 0.0f);
    updateDamageVisual();

    if (m_health <= 0) {
        m_health = m_maxHealth;
        resetToSpawn();
    }
    return true;
}

FL_CPP_WEAK int FL_Player::getHealth() const {
    return m_health;
}

FL_CPP_WEAK int FL_Player::getMaxHealth() const {
    return m_maxHealth;
}

FL_CPP_WEAK bool FL_Player::isInvulnerable() const {
    return m_invulnerabilityRemaining > 0.0f;
}

FL_CPP_WEAK CCRect FL_Player::getCollisionBounds() {
    const CCPoint position = getPosition();
    return CCRect(
        position.x - m_bodyHalfWidth,
        position.y - m_bodyHalfHeight,
        m_bodyHalfWidth * 2.0f,
        m_bodyHalfHeight * 2.0f
    );
}

void FL_Player::rideMovingBlock(const CCPoint& delta, float platformTop) {
    CCPoint position = ccpAdd(getPosition(), delta);
    position.y = platformTop + m_bodyHalfHeight;
    setPosition(position);
    m_velocity.y = 0.0f;
    m_grounded = true;
    m_dynamicGrounded = true;
}

void FL_Player::landOnMovingBlock(float platformTop) {
    CCPoint position = getPosition();
    position.y = platformTop + m_bodyHalfHeight;
    setPosition(position);
    m_velocity.y = 0.0f;
    m_grounded = true;
    m_dynamicGrounded = true;
}

void FL_Player::setPushing(bool pushing) {
    m_pushing = pushing;
}

void FL_Player::setPulling(bool pulling, bool faceRight) {
    m_pulling = pulling;
    m_pullFacingRight = faceRight;
    if (pulling) {
		// In the original attack-button handler, grabbing a pullable object is
		// selected instead of starting the melee attack.
		m_attacking = false;
		m_attackStrikeArmed = false;
		m_attackStrikeReady = false;
        m_facingRight = faceRight;
        if (m_sprite) m_sprite->setFlipX(!faceRight);
    }
}

void FL_Player::propel(float sourceBounceX, float sourceBounceY) {
    // Authored velocity is pixels per normalized tick; timeValue is exactly 60.
    // Preserve the full authored horizontal impulse; vertical world distance is
    // half-scale in this port (Frost/Fire playerVisualScale == .5).
    m_velocity.x = sourceBounceX * 60.f;
    m_velocity.y = sourceBounceY * 30.f;
    m_propelled = true;
    m_propelHorizontalFade = sourceBounceX != 0.f;
    setPositionY(getPositionY() + 8.f);
    m_grounded = false;
    m_dynamicGrounded = false;
    m_pushing = false;
    m_pulling = false;
}

bool FL_Player::isGrounded() const {
    return m_grounded;
}

CCPoint FL_Player::getVelocity() const {
    return m_velocity;
}

bool FL_Player::isFacingRight() const {
    return m_facingRight;
}

bool FL_Player::getAttackFacingRight() const {
    return m_attackFacingRight;
}


bool FL_Player::isFireStance() const {
    return m_stance == FL_PLAYER_STANCE_FIRE;
}

FL_PlayerStanceType FL_Player::getStance() const {
    return m_stance;
}

FL_PlayerStanceType FL_Player::getAttackStance() const {
    return m_attackStance;
}

#undef FL_CPP_WEAK
