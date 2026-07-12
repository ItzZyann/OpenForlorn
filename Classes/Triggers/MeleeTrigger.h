#pragma once

#include "../incl.h"
#include "../FL_PlayerStance.h"
#include <string>

namespace FLTriggers {

// Shared collision/combination rules for objects handled by the original
// PlayLayer collisionRegProj path (switches and BlockCombinationLock).
class MeleeTrigger {
public:
    static CCRect interactionBounds(CCNode* node);
    static bool intersects(const CCRect& attackBounds, CCNode* node);
    static bool requiresSword(const std::string& combinationToken);
    static std::string swordCode(FL_PlayerStanceType stance);
};

}
