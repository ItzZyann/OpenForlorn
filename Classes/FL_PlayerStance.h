#pragma once

#include "incl.h"

// Header-only helper so the modified archive does not require adding a new .cpp
// file to the Xcode target.  It mirrors pseudocode's two stance states: Frost
// by default, Fire after switching, and stores the current stance between level
// reloads.
enum FL_PlayerStanceType {
    FL_PLAYER_STANCE_FROST = 0,
    FL_PLAYER_STANCE_FIRE = 1
};

class FL_PlayerStanceState {
public:
    static inline FL_PlayerStanceType load() {
        CCUserDefault* defaults = CCUserDefault::sharedUserDefault();
        if (!defaults) return FL_PLAYER_STANCE_FROST;
        return defaults->getBoolForKey(storageKey(), false)
            ? FL_PLAYER_STANCE_FIRE
            : FL_PLAYER_STANCE_FROST;
    }

    static inline void save(FL_PlayerStanceType stance) {
        CCUserDefault* defaults = CCUserDefault::sharedUserDefault();
        if (!defaults) return;
        defaults->setBoolForKey(storageKey(), stance == FL_PLAYER_STANCE_FIRE);
        defaults->flush();
    }

    static inline FL_PlayerStanceType toggle(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE
            ? FL_PLAYER_STANCE_FROST
            : FL_PLAYER_STANCE_FIRE;
    }

    static inline const char* name(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE ? "Fire" : "Frost";
    }

    static inline const char* characterAtlas(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE
            ? "Fire_Main_Character_spritesheet_01.plist"
            : "Frost_Main_Character_spritesheet_01.plist";
    }

    static inline const char* projectileBase(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE
            ? "kProjFire_Fireball"
            : "kProjFrost_Frostbolt";
    }

    static inline const char* projectileTail(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE
            ? "Fireball_tail2.plist"
            : "Frostbolt_tail.plist";
    }

    static inline const char* meleeHitParticle(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE
            ? "meleeHitFire.plist"
            : "meleeHitFrost.plist";
    }

    static inline const char* projectileHitParticle(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE
            ? "fireHit.plist"
            : "frostHit.plist";
    }

    static inline const char* hitBurstFrame(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE
            ? "hitBurstFire_001.png"
            : "hitBurstFrost_001.png";
    }

    static inline const char* stanceSwitchParticle(FL_PlayerStanceType stance) {
        return stance == FL_PLAYER_STANCE_FIRE
            ? "fireParticle.plist"
            : "frostParticle.plist";
    }

private:
    static inline const char* storageKey() {
        return "FL_Player_CurrentStance_IsFire";
    }
};
