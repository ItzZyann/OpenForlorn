#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#include "FL_Player.h"

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
            return "Frost_Main_Character_spritesheet_01.plist";
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
    , m_invulnerabilityRemaining(0.0f)
    , m_hurtControlLockRemaining(0.0f)
    , m_health(12)
    , m_maxHealth(12)
    , m_jumpQueued(false)
    , m_attackQueued(false)
    , m_attacking(false)
    , m_hurtAnimating(false)
    , m_nextGroundAttackIsFirst(false)
    , m_grounded(false)
    , m_facingRight(true) {
}

FL_CPP_WEAK bool FL_Player::initialize(
    const CCPoint& spawnPosition,
    const CCSize& levelSize,
    const FL_CollisionWorld& collisionWorld
) {
    if (!CCNode::init()) return false;

    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(FL_PlayerDetail::playerAtlas());
    loadAnimationFrames();
    if (m_idleFrames.empty()) {
        CCLog("Player atlas %s does not contain Frost_Idle1 frames.", FL_PlayerDetail::playerAtlas());
        return false;
    }

    m_sprite = CCSprite::createWithSpriteFrame(m_idleFrames.front());
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

FL_CPP_WEAK void FL_Player::loadAnimationFrames() {
    FL_PlayerDetail::appendNumberedFrames(m_idleFrames, "Frost_Idle1_Anim_%03d.png", 1, 6);
    FL_PlayerDetail::appendNumberedFrames(m_runFrames, "Frost_Run_Anim_%03d.png", 1, 8);
    FL_PlayerDetail::appendNumberedFrames(m_attack1Frames, "Frost_Cast_MainAttack_1_Anim_%03d.png", 2, 9);
    FL_PlayerDetail::appendNumberedFrames(m_attack2Frames, "Frost_Cast_MainAttack_2_Anim_%03d.png", 1, 8);

    const int airAttackFrames[] = { 1, 1, 1, 2, 3 };
    for (unsigned int index = 0;
        index < sizeof(airAttackFrames) / sizeof(airAttackFrames[0]); ++index) {
        FL_PlayerDetail::appendFrame(
            m_attackAirFrames,
            "Frost_Cast_MainAttack_Air_Anim_%03d.png",
            airAttackFrames[index]
        );
    }

    FL_PlayerDetail::appendNumberedFrames(m_getHitFrames, "Frost_GetHit_Anim_%03d.png", 1, 4);
    FL_PlayerDetail::appendNumberedFrames(m_getHitAirFrames, "Frost_GetHit_Air_Anim_%03d.png", 1, 2);

    m_jumpUpFrame = FL_PlayerDetail::frameByName("Frost_Jump_up_001.png");
    m_jumpMiddleFrame = FL_PlayerDetail::frameByName("Frost_Jump_middle_001.png");
    m_jumpFallFrame = FL_PlayerDetail::frameByName("Frost_Jump_fall_001.png");
}

FL_CPP_WEAK void FL_Player::setMoveInput(float direction) {
    m_moveInput = FL_PlayerDetail::clampFloat(direction, -1.0f, 1.0f);
}

FL_CPP_WEAK void FL_Player::requestJump() {
    m_jumpQueued = true;
}

FL_CPP_WEAK void FL_Player::requestAttack() {
    m_attackQueued = true;
}

FL_CPP_WEAK void FL_Player::startAttack() {
    AnimationState attackState = ANIMATION_ATTACK_AIR;
    const std::vector<CCSpriteFrame*>* attackFrames = &m_attackAirFrames;

    if (m_grounded) {
        if (m_nextGroundAttackIsFirst) {
            attackState = ANIMATION_ATTACK_1;
            attackFrames = &m_attack1Frames;
        }
        else {
            attackState = ANIMATION_ATTACK_2;
            attackFrames = &m_attack2Frames;
        }
        m_nextGroundAttackIsFirst = !m_nextGroundAttackIsFirst;
    }

    if (attackFrames->empty()) return;

    m_attacking = true;
    m_attackCooldownRemaining = FL_PlayerDetail::attackCooldown();
    changeAnimation(attackState);
}

FL_CPP_WEAK void FL_Player::step(float dt) {
    const float frameTime = FL_PlayerDetail::clampFloat(
        dt,
        0.0f,
        FL_PlayerDetail::maximumFrameTime()
    );

    m_attackCooldownRemaining = std::max(0.0f, m_attackCooldownRemaining - frameTime);
    m_invulnerabilityRemaining = std::max(0.0f, m_invulnerabilityRemaining - frameTime);
    m_hurtControlLockRemaining = std::max(0.0f, m_hurtControlLockRemaining - frameTime);
    updateDamageVisual();

    if (m_attackQueued) {
        if (!m_attacking && !m_hurtAnimating && m_attackCooldownRemaining <= 0.0f) startAttack();
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

    if (m_jumpQueued && m_grounded) {
        m_velocity.y = m_jumpVelocity;
        m_grounded = false;
    }
    m_jumpQueued = false;

    for (int substep = 0; substep < substeps; ++substep) {
        simulateSubstep(substepTime);
    }

    if (getPositionY() < -300.0f) {
        takeDamage(2);
        resetToSpawn();
    }
    updateFacing();
    updateAnimation(frameTime);
}

FL_CPP_WEAK void FL_Player::simulateSubstep(float dt) {
    const float targetHorizontalVelocity = m_moveInput * m_horizontalSpeed;
    const float acceleration = m_grounded ? m_groundAcceleration : m_airAcceleration;
    if (m_hurtControlLockRemaining <= 0.0f) {
        m_velocity.x = approach(m_velocity.x, targetHorizontalVelocity, acceleration * dt);
    }
    m_velocity.y += m_gravity * dt;

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

    m_grounded = collisionResult.grounded;
    setPosition(newPosition);
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
    m_velocity = CCPointZero;
    m_jumpQueued = false;
    m_attackQueued = false;
    m_attacking = false;
    m_attackCooldownRemaining = 0.0f;
    m_nextGroundAttackIsFirst = false;
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

FL_CPP_WEAK void FL_Player::updateFacing() {
    if (m_moveInput > 0.05f) m_facingRight = true;
    else if (m_moveInput < -0.05f) m_facingRight = false;
    if (m_sprite) m_sprite->setFlipX(!m_facingRight);
}

FL_CPP_WEAK void FL_Player::updateDamageVisual() {
    if (!m_sprite) return;
    if (m_invulnerabilityRemaining <= 0.0f) {
        m_sprite->setOpacity(255);
        return;
    }
    const int phase = static_cast<int>(m_invulnerabilityRemaining * 18.0f);
    m_sprite->setOpacity((phase & 1) ? 110 : 255);
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>* FL_Player::attackFramesForState() const {
    if (m_animationState == ANIMATION_ATTACK_1) return &m_attack1Frames;
    if (m_animationState == ANIMATION_ATTACK_2) return &m_attack2Frames;
    if (m_animationState == ANIMATION_ATTACK_AIR) return &m_attackAirFrames;
    return NULL;
}

FL_CPP_WEAK const std::vector<CCSpriteFrame*>* FL_Player::hurtFramesForState() const {
    if (m_animationState == ANIMATION_GET_HIT) return &m_getHitFrames;
    if (m_animationState == ANIMATION_GET_HIT_AIR) return &m_getHitAirFrames;
    return NULL;
}

FL_CPP_WEAK void FL_Player::startHurtAnimation(bool forceAirAnimation) {
    const bool useAirHit = ((forceAirAnimation || !m_grounded) && !m_getHitAirFrames.empty());
    const std::vector<CCSpriteFrame*>& frames = useAirHit ? m_getHitAirFrames : m_getHitFrames;
    if (frames.empty()) return;

    m_attacking = false;
    m_hurtAnimating = true;
    changeAnimation(useAirHit ? ANIMATION_GET_HIT_AIR : ANIMATION_GET_HIT);
}

FL_CPP_WEAK void FL_Player::changeAnimation(AnimationState state) {
    if (state == m_animationState && m_animationFrame == 0 && m_animationElapsed == 0.0f) return;

    m_animationState = state;
    m_animationFrame = 0;
    m_animationElapsed = 0.0f;
    m_idleFrameDirection = 1;

    CCSpriteFrame* frame = NULL;
    if (state == ANIMATION_IDLE && !m_idleFrames.empty()) frame = m_idleFrames.front();
    else if (state == ANIMATION_RUN && !m_runFrames.empty()) frame = m_runFrames.front();
    else if (state == ANIMATION_ATTACK_1 && !m_attack1Frames.empty()) frame = m_attack1Frames.front();
    else if (state == ANIMATION_ATTACK_2 && !m_attack2Frames.empty()) frame = m_attack2Frames.front();
    else if (state == ANIMATION_ATTACK_AIR && !m_attackAirFrames.empty()) frame = m_attackAirFrames.front();
    else if (state == ANIMATION_GET_HIT && !m_getHitFrames.empty()) frame = m_getHitFrames.front();
    else if (state == ANIMATION_GET_HIT_AIR && !m_getHitAirFrames.empty()) frame = m_getHitAirFrames.front();
    else if (state == ANIMATION_JUMP_UP) frame = m_jumpUpFrame;
    else if (state == ANIMATION_JUMP_MIDDLE) frame = m_jumpMiddleFrame;
    else if (state == ANIMATION_JUMP_FALL) frame = m_jumpFallFrame;

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

    if (m_attacking) {
        const std::vector<CCSpriteFrame*>* attackFrames = attackFramesForState();
        if (!attackFrames || attackFrames->empty() || !m_sprite) {
            m_attacking = false;
        }
        else {
            m_animationElapsed += dt;
            while (m_animationElapsed >= FL_PlayerDetail::attackFrameDelay()) {
                m_animationElapsed -= FL_PlayerDetail::attackFrameDelay();
                if (m_animationFrame + 1 < attackFrames->size()) {
                    ++m_animationFrame;
                    m_sprite->setDisplayFrame((*attackFrames)[m_animationFrame]);
                }
                else {
                    m_attacking = false;
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
    else if (std::fabs(m_velocity.x) > 12.0f) {
        desiredState = ANIMATION_RUN;
    }

    if (desiredState != m_animationState) changeAnimation(desiredState);

    const std::vector<CCSpriteFrame*>* frames = NULL;
    float frameDelay = 0.1f;
    if (m_animationState == ANIMATION_IDLE) {
        frames = &m_idleFrames;
        frameDelay = FL_PlayerDetail::idleFrameDelay();
    }
    else if (m_animationState == ANIMATION_RUN) {
        frames = &m_runFrames;
        frameDelay = FL_PlayerDetail::runFrameDelay();
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

bool FL_Player::isGrounded() const {
    return m_grounded;
}

CCPoint FL_Player::getVelocity() const {
    return m_velocity;
}

#undef FL_CPP_WEAK
