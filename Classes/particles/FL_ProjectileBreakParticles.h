#pragma once

#include "../incl.h"
#include "../FL_PlayerStance.h"

#include <cstdio>

// Header-only projectile destruction effects.  This keeps the modified archive
// usable without adding a new .cpp file to the Xcode target while still keeping
// projectile-hit particles separate from the generic player particles file.
class FL_ProjectileBreakParticles {
public:
    static inline void spawnProjectileBreak(
        CCNode* parent,
        const CCPoint& impactPosition,
        bool facingRight,
        FL_PlayerStanceType stance,
        float delaySeconds = 0.0f
    );

private:
    static inline void ensureFramesLoaded();
    static inline float dictionaryFloat(CCDictionary* dictionary, const char* key, float fallback);
    static inline int dictionaryInt(CCDictionary* dictionary, const char* key, int fallback);
    static inline const char* dictionaryCString(CCDictionary* dictionary, const char* key);
    static inline void applyParticleTexture(CCParticleSystemQuad* particle, const char* textureFrameName);
    static inline CCParticleSystemQuad* createFromPlist(const char* plistFile, bool keepRunning = false);
    static inline CCParticleSystemQuad* createFallback(FL_PlayerStanceType stance);
    static inline CCAnimation* createHitAnimation(FL_PlayerStanceType stance);
    static inline CCSprite* createFirstHitSprite(FL_PlayerStanceType stance);
    static inline void applyMagicBlend(CCSprite* sprite);
    static inline int effectZ();
    static inline int particleZ();
};

inline int FL_ProjectileBreakParticles::effectZ() {
    // Pseudocode effect sprites are rendered on NPCspriteSheet/effectsSheet at z 110.
    return 110 * 10000;
}

inline int FL_ProjectileBreakParticles::particleZ() {
    // Projectile hit particles are particle batch effects, equivalent to P1 + 2.
    return 122 * 10000;
}

inline void FL_ProjectileBreakParticles::ensureFramesLoaded() {
    static bool loaded = false;
    if (loaded) return;
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("particleImgSheet.plist");
    CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("Effects_spritesheet_01-hd.plist");
    loaded = true;
}

inline float FL_ProjectileBreakParticles::dictionaryFloat(
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

inline int FL_ProjectileBreakParticles::dictionaryInt(
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

inline const char* FL_ProjectileBreakParticles::dictionaryCString(
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

inline void FL_ProjectileBreakParticles::applyParticleTexture(
    CCParticleSystemQuad* particle,
    const char* textureFrameName
) {
    if (!particle || !textureFrameName) return;
    ensureFramesLoaded();
    CCSpriteFrame* textureFrame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(textureFrameName);
    if (!textureFrame || !textureFrame->getTexture()) return;
    particle->setTextureWithRect(textureFrame->getTexture(), textureFrame->getRect());
}

inline CCParticleSystemQuad* FL_ProjectileBreakParticles::createFallback(
    FL_PlayerStanceType stance
) {
    CCParticleSystemQuad* particle = CCParticleSystemQuad::createWithTotalParticles(24);
    if (!particle) return NULL;

    applyParticleTexture(particle, stance == FL_PLAYER_STANCE_FIRE ? "fireParticle.png" : "snow.png");
    particle->setEmitterMode(kCCParticleModeGravity);
    particle->setDuration(0.05f);
    particle->setLife(0.22f);
    particle->setLifeVar(0.08f);
    particle->setAngle(0.0f);
    particle->setAngleVar(360.0f);
    particle->setSpeed(115.0f);
    particle->setSpeedVar(55.0f);
    particle->setGravity(ccp(0.0f, 260.0f));
    particle->setPosVar(ccp(6.0f, 6.0f));
    particle->setStartSize(12.0f);
    particle->setStartSizeVar(6.0f);
    particle->setEndSize(0.0f);
    particle->setEndSizeVar(0.0f);
    if (stance == FL_PLAYER_STANCE_FIRE) {
        particle->setStartColor(ccc4f(1.0f, 0.45f, 0.05f, 1.0f));
        particle->setEndColor(ccc4f(1.0f, 0.05f, 0.0f, 0.0f));
    }
    else {
        particle->setStartColor(ccc4f(0.50f, 0.85f, 1.0f, 1.0f));
        particle->setEndColor(ccc4f(0.10f, 0.35f, 1.0f, 0.0f));
    }
    particle->setEmissionRate(1000000.0f);
    particle->setPositionType(kCCPositionTypeGrouped);
    particle->setAutoRemoveOnFinish(true);
    ccBlendFunc blendFunc = { GL_SRC_ALPHA, GL_ONE };
    particle->setBlendFunc(blendFunc);
    return particle;
}

inline CCParticleSystemQuad* FL_ProjectileBreakParticles::createFromPlist(
    const char* plistFile,
    bool keepRunning
) {
    ensureFramesLoaded();

    CCDictionary* dictionary = CCDictionary::createWithContentsOfFile(plistFile);
    if (!dictionary) return createFallback(FL_PLAYER_STANCE_FROST);

    int totalParticles = dictionaryInt(dictionary, "maxParticles", 24);
    if (totalParticles < 1) totalParticles = 1;
    if (totalParticles > 400) totalParticles = 400;

    CCParticleSystemQuad* particle = CCParticleSystemQuad::createWithTotalParticles(
        static_cast<unsigned int>(totalParticles)
    );
    if (!particle) return NULL;

    const char* textureFrameName = dictionaryCString(dictionary, "textureFileName");
    applyParticleTexture(particle, textureFrameName ? textureFrameName : "sparkle.png");

    const int emitterType = dictionaryInt(dictionary, "emitterType", 0);
    particle->setEmitterMode(emitterType == 1 ? kCCParticleModeRadius : kCCParticleModeGravity);

    float duration = dictionaryFloat(dictionary, "duration", 0.05f);
    if (!keepRunning && duration < 0.0f) duration = 0.05f;
    particle->setDuration(duration);

    const float life = dictionaryFloat(dictionary, "particleLifespan", 0.25f);
    const float lifeVariance = dictionaryFloat(dictionary, "particleLifespanVariance", life * 0.25f);
    particle->setLife(life);
    particle->setLifeVar(lifeVariance);

    particle->setAngle(dictionaryFloat(dictionary, "angle", 0.0f));
    particle->setAngleVar(dictionaryFloat(dictionary, "angleVariance", 0.0f));
    particle->setStartSize(dictionaryFloat(dictionary, "startParticleSize", 8.0f)*2.f);
    particle->setStartSizeVar(dictionaryFloat(dictionary, "startParticleSizeVariance", 0.0f)*2.f);
    particle->setEndSize(dictionaryFloat(dictionary, "finishParticleSize", 0.0f)*2.f);
    particle->setEndSizeVar(dictionaryFloat(dictionary, "finishParticleSizeVariance", 0.0f)*2.f);
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

    if (life > 0.0001f) particle->setEmissionRate(static_cast<float>(totalParticles) / life);
    else particle->setEmissionRate(1000000.0f);

    particle->setPositionType(kCCPositionTypeGrouped);
    particle->setAutoRemoveOnFinish(!keepRunning);
    return particle;
}

inline CCAnimation* FL_ProjectileBreakParticles::createHitAnimation(
    FL_PlayerStanceType stance
) {
    ensureFramesLoaded();
    const char* prefix = stance == FL_PLAYER_STANCE_FIRE
        ? "kProjFire_Fireball_hit_%03d.png"
        : "kProjFrost_Frostbolt_hit_%03d.png";
    const int frameCount = stance == FL_PLAYER_STANCE_FIRE ? 5 : 4;
    const float frameDelay = stance == FL_PLAYER_STANCE_FIRE ? 0.06f : 0.04f;

    CCAnimation* animation = CCAnimation::create();
    if (!animation) return NULL;

    CCSpriteFrameCache* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
    char frameName[96];
    for (int i = 1; i <= frameCount; ++i) {
        snprintf(frameName, sizeof(frameName), prefix, i);
        CCSpriteFrame* frame = cache->spriteFrameByName(frameName);
        if (frame) animation->addSpriteFrame(frame);
    }

    if (animation->getFrames()->count() == 0) return NULL;
    animation->setDelayPerUnit(frameDelay);
    animation->setRestoreOriginalFrame(false);
    return animation;
}

inline CCSprite* FL_ProjectileBreakParticles::createFirstHitSprite(
    FL_PlayerStanceType stance
) {
    ensureFramesLoaded();
    const char* frameName = stance == FL_PLAYER_STANCE_FIRE
        ? "kProjFire_Fireball_hit_001.png"
        : "kProjFrost_Frostbolt_hit_001.png";
    CCSpriteFrame* frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName);
    return frame ? CCSprite::createWithSpriteFrame(frame) : NULL;
}

inline void FL_ProjectileBreakParticles::applyMagicBlend(CCSprite* sprite) {
    if (!sprite) return;
    ccBlendFunc blendFunc = { GL_SRC_ALPHA, GL_ONE };
    sprite->setBlendFunc(blendFunc);
    sprite->setOpacity(255);
}

inline void FL_ProjectileBreakParticles::spawnProjectileBreak(
    CCNode* parent,
    const CCPoint& impactPosition,
    bool facingRight,
    FL_PlayerStanceType stance,
    float delaySeconds
) {
    if (!parent) return;

    const float delay = delaySeconds > 0.0f ? delaySeconds : 0.0f;
    const char* hitParticleFile = FL_PlayerStanceState::projectileHitParticle(stance);

    CCParticleSystemQuad* hitParticle = createFromPlist(hitParticleFile, false);
    if (hitParticle) {
        hitParticle->setPosition(impactPosition);
        hitParticle->setVisible(delay <= 0.0f);
        parent->addChild(hitParticle, particleZ());
        parent->reorderChild(hitParticle, particleZ());
        if (delay > 0.0f) hitParticle->stopSystem();
        hitParticle->runAction(CCSequence::create(
            CCDelayTime::create(delay),
            CCShow::create(),
            CCCallFunc::create(hitParticle, callfunc_selector(CCParticleSystem::resetSystem)),
            CCDelayTime::create(0.80f),
            CCCallFunc::create(hitParticle, callfunc_selector(CCNode::removeFromParent)),
            NULL
        ));
    }

    // The animated projectile explosion is intentionally disabled.  The
    // original impact particle system above remains visible on destruction.
#if 0
    CCSprite* hitSprite = createFirstHitSprite(stance);
    if (hitSprite) {
        hitSprite->setPosition(impactPosition);
        hitSprite->setScale(1.5f);
        hitSprite->setScaleX(facingRight ? 1.5f : -1.5f);
        hitSprite->setVisible(delay <= 0.0f);
        applyMagicBlend(hitSprite);
        parent->addChild(hitSprite, effectZ());
        parent->reorderChild(hitSprite, effectZ());

        CCAnimation* animation = createHitAnimation(stance);
        CCFiniteTimeAction* hitAction = animation ? static_cast<CCFiniteTimeAction*>(CCAnimate::create(animation)) : CCDelayTime::create(0.18f);
        hitSprite->runAction(CCSequence::create(
            CCDelayTime::create(delay),
            CCShow::create(),
            hitAction,
            CCCallFunc::create(hitSprite, callfunc_selector(CCNode::removeFromParent)),
            NULL
        ));
    }
#endif
}
