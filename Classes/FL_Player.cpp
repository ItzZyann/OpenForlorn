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

CCSpriteFrame* frameByName(const char* name) {
    return CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(name);
}

void appendNumberedFrames(
    std::vector<CCSpriteFrame*>& output,
    const char* format,
    int frameCount
) {
    char frameName[128];
    for (int frame = 1; frame <= frameCount; ++frame) {
        std::snprintf(frameName, sizeof(frameName), format, frame);
        CCSpriteFrame* spriteFrame = frameByName(frameName);
        if (spriteFrame) output.push_back(spriteFrame);
    }
}

float clampFloat(float value, float minimum, float maximum) {
    return std::max(minimum, std::min(maximum, value));
}

} // namespace FL_PlayerDetail
} // namespace

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
    , m_jumpQueued(false)
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
    FL_PlayerDetail::appendNumberedFrames(m_idleFrames, "Frost_Idle1_Anim_%03d.png", 6);
    FL_PlayerDetail::appendNumberedFrames(m_runFrames, "Frost_Run_Anim_%03d.png", 8);
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

FL_CPP_WEAK void FL_Player::step(float dt) {
    const float frameTime = FL_PlayerDetail::clampFloat(dt, 0.0f, FL_PlayerDetail::maximumFrameTime());
    if (frameTime <= 0.0f) {
        updateAnimation(0.0f);
        m_jumpQueued = false;
        return;
    }

    int substeps = static_cast<int>(std::ceil(frameTime / FL_PlayerDetail::maximumPhysicsStep()));
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

    if (getPositionY() < -300.0f) resetToSpawn();
    updateFacing();
    updateAnimation(frameTime);
}

FL_CPP_WEAK void FL_Player::simulateSubstep(float dt) {
    const float targetHorizontalVelocity = m_moveInput * m_horizontalSpeed;
    const float acceleration = m_grounded ? m_groundAcceleration : m_airAcceleration;
    m_velocity.x = approach(m_velocity.x, targetHorizontalVelocity, acceleration * dt);
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
FL_CPP_WEAK float FL_Player::approach(float current, float target, float maximumDelta) const {
    if (current < target) return std::min(current + maximumDelta, target);
    if (current > target) return std::max(current - maximumDelta, target);
    return target;
}

FL_CPP_WEAK void FL_Player::resetToSpawn() {
    CCPoint position = m_spawnPosition;
    m_velocity = CCPointZero;
    m_jumpQueued = false;
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

FL_CPP_WEAK void FL_Player::changeAnimation(AnimationState state) {
    if (state == m_animationState && m_animationFrame == 0 && m_animationElapsed == 0.0f) return;
    m_animationState = state;
    m_animationFrame = 0;
    m_animationElapsed = 0.0f;

    CCSpriteFrame* frame = NULL;
    if (state == ANIMATION_IDLE && !m_idleFrames.empty()) frame = m_idleFrames.front();
    else if (state == ANIMATION_RUN && !m_runFrames.empty()) frame = m_runFrames.front();
    else if (state == ANIMATION_JUMP_UP) frame = m_jumpUpFrame;
    else if (state == ANIMATION_JUMP_MIDDLE) frame = m_jumpMiddleFrame;
    else if (state == ANIMATION_JUMP_FALL) frame = m_jumpFallFrame;
    if (frame && m_sprite) m_sprite->setDisplayFrame(frame);
}

FL_CPP_WEAK void FL_Player::updateAnimation(float dt) {
    AnimationState desiredState = ANIMATION_IDLE;
    if (!m_grounded) {
        if (m_velocity.y > 80.0f) desiredState = ANIMATION_JUMP_UP;
        else if (m_velocity.y < -80.0f) desiredState = ANIMATION_JUMP_FALL;
        else desiredState = ANIMATION_JUMP_MIDDLE;
    } else if (std::fabs(m_velocity.x) > 12.0f) {
        desiredState = ANIMATION_RUN;
    }

    if (desiredState != m_animationState) changeAnimation(desiredState);

    const std::vector<CCSpriteFrame*>* frames = NULL;
    float frameDelay = 0.1f;
    if (m_animationState == ANIMATION_IDLE) {
        frames = &m_idleFrames;
        frameDelay = 1.0f / 8.0f;
    } else if (m_animationState == ANIMATION_RUN) {
        frames = &m_runFrames;
        frameDelay = 1.0f / 14.0f;
    }

    if (!frames || frames->size() <= 1 || !m_sprite) return;
    m_animationElapsed += dt;
    while (m_animationElapsed >= frameDelay) {
        m_animationElapsed -= frameDelay;
        m_animationFrame = (m_animationFrame + 1) % frames->size();
        m_sprite->setDisplayFrame((*frames)[m_animationFrame]);
    }
}

FL_CPP_WEAK bool FL_Player::isGrounded() const {
    return m_grounded;
}

FL_CPP_WEAK CCPoint FL_Player::getVelocity() const {
    return m_velocity;
}

#undef FL_CPP_WEAK
