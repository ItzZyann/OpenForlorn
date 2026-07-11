#pragma once

#include "../incl.h"
#include "../FL_PlayerStance.h"
#include "FL_ProjectileBreakParticles.h"


class FL_ParticleSystemQuadFixed : public CCParticleSystemQuad {
public:
    FL_ParticleSystemQuadFixed() : m_flipParticleTextureX(false) {}

    static FL_ParticleSystemQuadFixed* createWithTotalParticles(unsigned int particles) {
        FL_ParticleSystemQuadFixed* system = new FL_ParticleSystemQuadFixed();
        if (system && system->initWithTotalParticles(particles)) {
            system->autorelease();
            return system;
        }
        CC_SAFE_DELETE(system);
        return NULL;
    }

    void setFlipParticleTextureX(bool flip) {
        m_flipParticleTextureX = flip;
    }

    virtual void updateQuadWithParticle(tCCParticle* particle, const CCPoint& newPosition) {
        CCParticleSystemQuad::updateQuadWithParticle(particle, newPosition);

        if (!m_flipParticleTextureX || !m_pQuads) return;

        // This is not the visual mirror of the whole effect. The whole effect
        // is mirrored by a parent node in spawnAttackImpact(). This UV swap is
        // only a counter-flip so the individual particle image stays visually
        // normal after the parent node mirrors the particle layout.
        ccV3F_C4B_T2F_Quad& quad = m_pQuads[m_uParticleIdx];
        ccTex2F tmp;

        tmp = quad.bl.texCoords;
        quad.bl.texCoords = quad.br.texCoords;
        quad.br.texCoords = tmp;

        tmp = quad.tl.texCoords;
        quad.tl.texCoords = quad.tr.texCoords;
        quad.tr.texCoords = tmp;
    }

private:
    bool m_flipParticleTextureX;
};

class FL_PlayerParticles {
public:
    static inline float projectileDuration();
    static inline float projectileStartOffset();
    static inline float projectileTravelDistance();
    static inline CCNode* createAttackProjectileVisual(CCNode* parent, const CCPoint& start, bool facingRight, FL_PlayerStanceType stance, float lifetime);
    static inline void spawnAttackCast(CCNode* parent, const CCPoint& playerPosition, bool facingRight, bool airborne, FL_PlayerStanceType stance);
    static inline void spawnAttackImpact(CCNode* parent, const CCPoint& impactPosition, bool facingRight, FL_PlayerStanceType stance);
    static inline void spawnStanceSwitch(CCNode* parent, const CCPoint& playerPosition, FL_PlayerStanceType stance);
    static inline void spawnRollDust(CCNode* parent, const CCPoint& playerPosition, bool facingRight);

private:
    static inline void ensureParticleFramesLoaded();
    static inline CCParticleSystemQuad* createFromPlist(const char* plistFile, bool keepRunning, bool flipParticleTextureX = false);
    static inline CCParticleSystemQuad* createSafeFallback(unsigned int particles, float duration, float life, float size);
    static inline float dictionaryFloat(CCDictionary* dictionary, const char* key, float fallback);
    static inline int dictionaryInt(CCDictionary* dictionary, const char* key, int fallback);
    static inline const char* dictionaryCString(CCDictionary* dictionary, const char* key);
    static inline void applyParticleTexture(CCParticleSystemQuad* particle, const char* textureFrameName);
    static inline void addParticle(CCNode* parent, CCParticleSystemQuad* particle, const CCPoint& position, int zOrder);
    static inline CCSprite* createSpriteFromAnyFrame(const char* first, const char* second, const char* third);
    static inline void applyMagicSpriteBlend(CCSprite* sprite);
    static inline void stopAndRemoveParticle(CCParticleSystemQuad* particle, float stopDelay, float removeDelay);
};

static inline float FL_PlayerParticles_directionAngle(bool facingRight) {
    return facingRight ? 0.0f : 180.0f;
}

static inline CCPoint FL_PlayerParticles_forwardOffset(bool facingRight, float x, float y) {
    return ccp(facingRight ? x : -x, y);
}

static inline int FL_PlayerParticles_effectZ() {
    // Pseudocode PlayLayer adds NPCspriteSheet/effectsSheet to controlLayer at z = 110.
    // FL_PlayLayer multiplies layer categories by 10000, so use the equivalent
    // gameplay z instead of an arbitrary huge value.
    return 110 * 10000;
}

static inline int FL_PlayerParticles_particleZ() {
    // Pseudocode addParticleBatchNodeForType("P1") uses getZValueForType("P1") + 2.
    // P1 is 120, so particle systems render at 122.
    return 122 * 10000;
}

inline void FL_PlayerParticles::ensureParticleFramesLoaded() {
    static bool loaded = false;
    if (loaded) return;
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("particleImgSheet.plist");
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("Effects_spritesheet_01.plist");
    loaded = true;
}

inline float FL_PlayerParticles::dictionaryFloat(
    CCDictionary* dictionary,
    const char* key,
    float fallback
) {
    if (!dictionary || !key) return fallback;
    CCObject* object = dictionary->objectForKey(key);
    if (!object) return fallback;
    CCString* stringValue = static_cast<CCString*>(object);
    if (!stringValue) return fallback;
    return stringValue->floatValue();
}

inline int FL_PlayerParticles::dictionaryInt(
    CCDictionary* dictionary,
    const char* key,
    int fallback
) {
    if (!dictionary || !key) return fallback;
    CCObject* object = dictionary->objectForKey(key);
    if (!object) return fallback;
    CCString* stringValue = static_cast<CCString*>(object);
    if (!stringValue) return fallback;
    return stringValue->intValue();
}

inline const char* FL_PlayerParticles::dictionaryCString(
    CCDictionary* dictionary,
    const char* key
) {
    if (!dictionary || !key) return NULL;
    CCObject* object = dictionary->objectForKey(key);
    if (!object) return NULL;
    CCString* stringValue = static_cast<CCString*>(object);
    if (!stringValue) return NULL;
    return stringValue->getCString();
}

inline void FL_PlayerParticles::applyParticleTexture(
    CCParticleSystemQuad* particle,
    const char* textureFrameName
) {
    if (!particle || !textureFrameName) return;
    ensureParticleFramesLoaded();

    CCSpriteFrame* textureFrame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(textureFrameName);
    if (!textureFrame || !textureFrame->getTexture()) return;

    particle->setTextureWithRect(textureFrame->getTexture(), textureFrame->getRect());
}

inline CCParticleSystemQuad* FL_PlayerParticles::createSafeFallback(
    unsigned int particles,
    float duration,
    float life,
    float size
) {
    CCParticleSystemQuad* particle = FL_ParticleSystemQuadFixed::createWithTotalParticles(particles);
    if (!particle) return NULL;

    applyParticleTexture(particle, "sparkle.png");

    const float safeDuration = duration > 0.01f ? duration : 0.01f;
    const float safeLife = life > 0.01f ? life : 0.18f;

    particle->setEmitterMode(kCCParticleModeGravity);
    particle->setDuration(safeDuration);
    particle->setLife(safeLife);
    particle->setLifeVar(safeLife * 0.35f);
    particle->setSpeed(95.0f);
    particle->setSpeedVar(50.0f);
    particle->setGravity(ccp(0.0f, -60.0f));
    particle->setRadialAccel(0.0f);
    particle->setRadialAccelVar(0.0f);
    particle->setTangentialAccel(0.0f);
    particle->setTangentialAccelVar(0.0f);
    particle->setAngle(0.0f);
    particle->setAngleVar(55.0f);
    particle->setPosVar(ccp(10.0f, 8.0f));
    particle->setStartSize(size);
    particle->setStartSizeVar(size * 0.35f);
    particle->setEndSize(0.0f);
    particle->setEndSizeVar(0.0f);
    particle->setStartColor(ccc4f(0.62f, 0.86f, 1.0f, 0.95f));
    particle->setStartColorVar(ccc4f(0.08f, 0.08f, 0.10f, 0.0f));
    particle->setEndColor(ccc4f(0.18f, 0.38f, 0.95f, 0.0f));
    particle->setEndColorVar(ccc4f(0.05f, 0.05f, 0.12f, 0.0f));
    particle->setEmissionRate(static_cast<float>(particles) / safeLife);
    particle->setPositionType(kCCPositionTypeGrouped);
    particle->setAutoRemoveOnFinish(true);
    return particle;
}

inline CCParticleSystemQuad* FL_PlayerParticles::createFromPlist(
    const char* plistFile,
    bool keepRunning,
    bool flipParticleTextureX
) {
    ensureParticleFramesLoaded();

    CCDictionary* dictionary = CCDictionary::createWithContentsOfFile(plistFile);
    if (!dictionary) return createSafeFallback(24, 0.12f, 0.22f, 8.0f);

    int totalParticles = dictionaryInt(dictionary, "maxParticles", 32);
    if (totalParticles < 1) totalParticles = 1;
    if (totalParticles > 400) totalParticles = 400;

    CCParticleSystemQuad* particle = FL_ParticleSystemQuadFixed::createWithTotalParticles(
        static_cast<unsigned int>(totalParticles)
    );
    if (!particle) return NULL;

    const char* textureFrameName = dictionaryCString(dictionary, "textureFileName");
    applyParticleTexture(particle, textureFrameName ? textureFrameName : "sparkle.png");
    static_cast<FL_ParticleSystemQuadFixed*>(particle)->setFlipParticleTextureX(flipParticleTextureX);

    const int emitterType = dictionaryInt(dictionary, "emitterType", 0);
    particle->setEmitterMode(emitterType == 1 ? kCCParticleModeRadius : kCCParticleModeGravity);

    float duration = dictionaryFloat(dictionary, "duration", 0.10f);
    if (!keepRunning && duration < 0.0f) duration = 0.12f;
    particle->setDuration(duration);

    // Keep plist lifetime semantics intact. Several original effects
    // intentionally use particleLifespan = 0 with a positive variance;
    // replacing life with the variance makes particles travel too far
    // horizontally compared with PlayLayer::playMeleeHit in pseudocode.
    const float life = dictionaryFloat(dictionary, "particleLifespan", 0.25f);
    const float lifeVariance = dictionaryFloat(dictionary, "particleLifespanVariance", life * 0.25f);
    particle->setLife(life);
    particle->setLifeVar(lifeVariance);

    particle->setAngle(dictionaryFloat(dictionary, "angle", 0.0f));
    particle->setAngleVar(dictionaryFloat(dictionary, "angleVariance", 0.0f));
    particle->setStartSize(dictionaryFloat(dictionary, "startParticleSize", 8.0f));
    particle->setStartSizeVar(dictionaryFloat(dictionary, "startParticleSizeVariance", 0.0f));
    particle->setEndSize(dictionaryFloat(dictionary, "finishParticleSize", 0.0f));
    particle->setEndSizeVar(dictionaryFloat(dictionary, "finishParticleSizeVariance", 0.0f));
    particle->setPosVar(ccp(
        dictionaryFloat(dictionary, "sourcePositionVariancex", 0.0f),
        dictionaryFloat(dictionary, "sourcePositionVariancey", 0.0f)
    ));

    particle->setStartColor(ccc4f(
        dictionaryFloat(dictionary, "startColorRed", 1.0f),
        dictionaryFloat(dictionary, "startColorGreen", 1.0f),
        dictionaryFloat(dictionary, "startColorBlue", 1.0f),
        dictionaryFloat(dictionary, "startColorAlpha", 1.0f)
    ));
    particle->setStartColorVar(ccc4f(
        dictionaryFloat(dictionary, "startColorVarianceRed", 0.0f),
        dictionaryFloat(dictionary, "startColorVarianceGreen", 0.0f),
        dictionaryFloat(dictionary, "startColorVarianceBlue", 0.0f),
        dictionaryFloat(dictionary, "startColorVarianceAlpha", 0.0f)
    ));
    particle->setEndColor(ccc4f(
        dictionaryFloat(dictionary, "finishColorRed", 0.0f),
        dictionaryFloat(dictionary, "finishColorGreen", 0.0f),
        dictionaryFloat(dictionary, "finishColorBlue", 0.0f),
        dictionaryFloat(dictionary, "finishColorAlpha", 0.0f)
    ));
    particle->setEndColorVar(ccc4f(
        dictionaryFloat(dictionary, "finishColorVarianceRed", 0.0f),
        dictionaryFloat(dictionary, "finishColorVarianceGreen", 0.0f),
        dictionaryFloat(dictionary, "finishColorVarianceBlue", 0.0f),
        dictionaryFloat(dictionary, "finishColorVarianceAlpha", 0.0f)
    ));

    if (emitterType == 1) {
        particle->setStartRadius(dictionaryFloat(dictionary, "maxRadius", 0.0f));
        particle->setStartRadiusVar(dictionaryFloat(dictionary, "maxRadiusVariance", 0.0f));
        particle->setEndRadius(dictionaryFloat(dictionary, "minRadius", 0.0f));
        particle->setEndRadiusVar(0.0f);
        particle->setRotatePerSecond(dictionaryFloat(dictionary, "rotatePerSecond", 0.0f));
        particle->setRotatePerSecondVar(dictionaryFloat(dictionary, "rotatePerSecondVariance", 0.0f));
    }
    else {
        particle->setGravity(ccp(
            dictionaryFloat(dictionary, "gravityx", 0.0f),
            dictionaryFloat(dictionary, "gravityy", 0.0f)
        ));
        particle->setSpeed(dictionaryFloat(dictionary, "speed", 0.0f));
        particle->setSpeedVar(dictionaryFloat(dictionary, "speedVariance", 0.0f));
        particle->setTangentialAccel(dictionaryFloat(dictionary, "tangentialAcceleration", 0.0f));
        particle->setTangentialAccelVar(dictionaryFloat(dictionary, "tangentialAccelVariance", 0.0f));
        particle->setRadialAccel(dictionaryFloat(dictionary, "radialAcceleration", 0.0f));
        particle->setRadialAccelVar(dictionaryFloat(dictionary, "radialAccelVariance", 0.0f));
    }

    particle->setStartSpin(dictionaryFloat(dictionary, "rotationStart", 0.0f));
    particle->setStartSpinVar(dictionaryFloat(dictionary, "rotationStartVariance", 0.0f));
    particle->setEndSpin(dictionaryFloat(dictionary, "rotationEnd", 0.0f));
    particle->setEndSpinVar(dictionaryFloat(dictionary, "rotationEndVariance", 0.0f));

    ccBlendFunc blendFunc = {
        static_cast<GLenum>(dictionaryInt(dictionary, "blendFuncSource", GL_SRC_ALPHA)),
        static_cast<GLenum>(dictionaryInt(dictionary, "blendFuncDestination", GL_ONE_MINUS_SRC_ALPHA))
    };
    particle->setBlendFunc(blendFunc);

    // Match cocos2d-x CCParticleSystem::initWithDictionary semantics.
    // It computes emissionRate as maxParticles / particleLifespan, not
    // maxParticles / duration. Some original Forlorn effects intentionally
    // use particleLifespan = 0 with a positive variance; that produces a
    // near-instant burst. Stretching those particles over duration makes
    // meleeHitFrost look like a horizontal stream instead of the original
    // impact burst.
    if (life > 0.0001f) {
        particle->setEmissionRate(static_cast<float>(totalParticles) / life);
    }
    else {
        particle->setEmissionRate(1000000.0f);
    }
    particle->setPositionType(kCCPositionTypeGrouped);
    particle->setAutoRemoveOnFinish(!keepRunning);
    return particle;
}

inline CCSprite* FL_PlayerParticles::createSpriteFromAnyFrame(
    const char* first,
    const char* second,
    const char* third
) {
    ensureParticleFramesLoaded();
    CCSpriteFrameCache* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
    CCSpriteFrame* frame = NULL;
    if (!frame && first) frame = cache->spriteFrameByName(first);
    if (!frame && second) frame = cache->spriteFrameByName(second);
    if (!frame && third) frame = cache->spriteFrameByName(third);
    return frame ? CCSprite::createWithSpriteFrame(frame) : NULL;
}

inline void FL_PlayerParticles::applyMagicSpriteBlend(CCSprite* sprite) {
    if (!sprite) return;

    // Original Forlorn effects are rendered through effect/particle sheets
    // using additive blend in several paths (GL_SRC_ALPHA, GL_ONE).  The
    // replacement code creates standalone sprites, so without setting the
    // same blend on the sprite itself, frostbolt/hit textures look washed out
    // or semi-transparent compared with pseudocode.
    ccBlendFunc blendFunc = { GL_SRC_ALPHA, GL_ONE };
    sprite->setBlendFunc(blendFunc);
    sprite->setOpacity(255);
}

inline void FL_PlayerParticles::stopAndRemoveParticle(
    CCParticleSystemQuad* particle,
    float stopDelay,
    float removeDelay
) {
    if (!particle) return;
    particle->runAction(CCSequence::create(
        CCDelayTime::create(stopDelay),
        CCCallFunc::create(particle, callfunc_selector(CCParticleSystem::stopSystem)),
        CCDelayTime::create(removeDelay),
        CCCallFunc::create(particle, callfunc_selector(CCNode::removeFromParent)),
        NULL
    ));
}

inline void FL_PlayerParticles::addParticle(
    CCNode* parent,
    CCParticleSystemQuad* particle,
    const CCPoint& position,
    int zOrder
) {
    if (!parent || !particle) return;
    particle->setPosition(position);
    parent->addChild(particle, zOrder);
    parent->reorderChild(particle, zOrder);

    // Match PlayLayer::playMeleeHit order from pseudocode: setPosition first,
    // then resetSystem. Resetting before positioning makes short burst effects
    // emit from the old/default emitter origin, which looks like the larger
    // impact texture/elements are shifted left.
    particle->resetSystem();
}

inline float FL_PlayerParticles::projectileDuration() {
    return 0.60f;
}

inline float FL_PlayerParticles::projectileStartOffset() {
    return 42.0f;
}

inline float FL_PlayerParticles::projectileTravelDistance() {
    return 360.0f;
}

inline CCNode* FL_PlayerParticles::createAttackProjectileVisual(
    CCNode* parent,
    const CCPoint& start,
    bool facingRight,
    FL_PlayerStanceType stance,
    float lifetime
) {
    if (!parent) return NULL;

    // The visual root is controlled by FL_PlayLayer when collision with level
    // geometry is required.  Keep all sprite/particle children local to this
    // root so the whole projectile can be moved and destroyed as one object.
    CCNode* projectileRoot = CCNode::create();
    if (!projectileRoot) return NULL;
    projectileRoot->setPosition(start);
    projectileRoot->setScaleX(facingRight ? -1.0f : 1.0f);
    parent->addChild(projectileRoot, FL_PlayerParticles_effectZ());
    parent->reorderChild(projectileRoot, FL_PlayerParticles_effectZ());

    CCParticleSystemQuad* tail = createFromPlist(FL_PlayerStanceState::projectileTail(stance), true);
    if (tail) {
        tail->setAngle(0.0f);
        addParticle(projectileRoot, tail, ccp(0.0f, 0.0f), 1);
        tail->runAction(CCSequence::create(
            CCDelayTime::create(lifetime),
            CCCallFunc::create(tail, callfunc_selector(CCParticleSystem::stopSystem)),
            NULL
        ));
    }

    CCSprite* bolt = createSpriteFromAnyFrame(
        stance == FL_PLAYER_STANCE_FIRE ? "kProjFire_Fireball_middle_001.png" : "kProjFrost_Frostbolt_middle_001.png",
        stance == FL_PLAYER_STANCE_FIRE ? "kProjFire_Fireball_start_003.png" : "kProjFrost_Frostbolt_start_003.png",
        stance == FL_PLAYER_STANCE_FIRE ? "Fire_Cast_MainAttack_Air_Anim_003.png" : "Frost_Cast_MainAttack_Air_Anim_003.png"
    );
    if (bolt) {
        bolt->setPosition(CCPointZero);
        bolt->setScale(0.55f);
        bolt->setOpacity(255);
        applyMagicSpriteBlend(bolt);
        projectileRoot->addChild(bolt, 2);
        bolt->runAction(CCSequence::create(
            CCDelayTime::create(lifetime),
            CCCallFunc::create(bolt, callfunc_selector(CCNode::removeFromParent)),
            NULL
        ));
    }

    return projectileRoot;
}

inline void FL_PlayerParticles::spawnAttackCast(
    CCNode* parent,
    const CCPoint& playerPosition,
    bool facingRight,
    bool airborne,
    FL_PlayerStanceType stance
) {
    if (!parent) return;

    const float lifetime = projectileDuration();
    const CCPoint start = ccpAdd(
        playerPosition,
        FL_PlayerParticles_forwardOffset(facingRight, projectileStartOffset(), airborne ? 4.0f : -3.0f)
    );
    const CCPoint travel = FL_PlayerParticles_forwardOffset(facingRight, projectileTravelDistance(), 0.0f);

    CCNode* projectileRoot = createAttackProjectileVisual(parent, start, facingRight, stance, lifetime);
    if (!projectileRoot) return;

    FL_ProjectileBreakParticles::spawnProjectileBreak(
        parent,
        ccpAdd(start, travel),
        facingRight,
        stance,
        lifetime
    );

    projectileRoot->runAction(CCSequence::create(
        CCMoveBy::create(lifetime, travel),
        CCDelayTime::create(0.12f),
        CCCallFunc::create(projectileRoot, callfunc_selector(CCNode::removeFromParent)),
        NULL
    ));
}

inline void FL_PlayerParticles::spawnAttackImpact(
    CCNode* parent,
    const CCPoint& impactPosition,
    bool facingRight,
    FL_PlayerStanceType stance
) {
    if (!parent) return;

    // Match pseudocode layering:
    // - meleeHitFrost.plist particle systems are added through addParticleSystem("P1"),
    //   which renders at getZValueForType("P1") + 2 = 122.
    // - hitBurstFrost is created by playEffect(), which adds the sprite to
    //   NPCspriteSheet at z = 110.
    // Keep them as separate roots so the particle layer and effect-sprite layer
    // can have the same ordering as the original code.
    CCNode* particleRoot = CCNode::create();
    if (particleRoot) {
        particleRoot->setPosition(impactPosition);
        particleRoot->setScaleX(facingRight ? -1.0f : 1.0f);
        parent->addChild(particleRoot, FL_PlayerParticles_particleZ());
        parent->reorderChild(particleRoot, FL_PlayerParticles_particleZ());

        CCParticleSystemQuad* hit = createFromPlist(FL_PlayerStanceState::meleeHitParticle(stance), false, false);
        if (hit) {
            addParticle(particleRoot, hit, ccp(0.0f, 0.0f), 1);
        }

        particleRoot->runAction(CCSequence::create(
            CCDelayTime::create(0.85f),
            CCCallFunc::create(particleRoot, callfunc_selector(CCNode::removeFromParent)),
            NULL
        ));
    }

    CCNode* spriteRoot = CCNode::create();
    if (!spriteRoot) return;
    spriteRoot->setPosition(impactPosition);
    spriteRoot->setScaleX(facingRight ? -1.0f : 1.0f);
    parent->addChild(spriteRoot, FL_PlayerParticles_effectZ());
    parent->reorderChild(spriteRoot, FL_PlayerParticles_effectZ());

    CCSprite* burst = createSpriteFromAnyFrame(
        FL_PlayerStanceState::hitBurstFrame(stance),
        "hitBurst_001.png",
        NULL
    );
    if (burst) {
        const float jitterX = CCRANDOM_MINUS1_1() * 10.0f;
        const float jitterY = CCRANDOM_MINUS1_1() * 10.0f;

        burst->setPosition(ccp(18.0f + jitterX, jitterY));
        burst->setScale(1.5f);
        burst->setRotation(CCRANDOM_0_1() * 360.0f);
        applyMagicSpriteBlend(burst);
        spriteRoot->addChild(burst, 3);
        burst->runAction(CCSequence::create(
            CCDelayTime::create(0.10f),
            CCCallFunc::create(burst, callfunc_selector(CCNode::removeFromParent)),
            NULL
        ));
    }

    spriteRoot->runAction(CCSequence::create(
        CCDelayTime::create(0.30f),
        CCCallFunc::create(spriteRoot, callfunc_selector(CCNode::removeFromParent)),
        NULL
    ));
}

inline void FL_PlayerParticles::spawnStanceSwitch(
    CCNode* parent,
    const CCPoint& playerPosition,
    FL_PlayerStanceType stance
) {
    if (!parent) return;

    // Pseudocode keeps fireEmitter/frostEmitter pre-created, positions the one
    // matching the new stance at the player, then resetSystem().  Create the
    // same short-lived emitter here, from the same plist files, so no Xcode
    // target update is required.
    CCParticleSystemQuad* stanceParticle = createFromPlist(
        FL_PlayerStanceState::stanceSwitchParticle(stance),
        false
    );
    if (!stanceParticle) return;

    stanceParticle->setPositionType(kCCPositionTypeRelative);
    addParticle(parent, stanceParticle, playerPosition, FL_PlayerParticles_particleZ());
    stopAndRemoveParticle(stanceParticle, 0.18f, 0.85f);
}

inline void FL_PlayerParticles::spawnRollDust(
    CCNode* parent,
    const CCPoint& playerPosition,
    bool facingRight
) {
    if (!parent) return;

    CCParticleSystemQuad* dust = createFromPlist("dustEffect_snow.plist", true);
    if (dust) {
        dust->setAngle(facingRight ? 180.0f : 0.0f);
        dust->setPositionType(kCCPositionTypeGrouped);
        addParticle(parent, dust, ccpAdd(playerPosition, ccp(0.0f, -44.0f)), FL_PlayerParticles_particleZ());
        stopAndRemoveParticle(dust, 0.14f, 0.90f);
    }
}
