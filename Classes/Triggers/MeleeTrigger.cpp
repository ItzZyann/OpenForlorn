#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif

#endif

#include "MeleeTrigger.h"
#include <algorithm>
#include <cstring>

namespace FLTriggers {

CCRect MeleeTrigger::interactionBounds(CCNode* node) {
    if (!node) return CCRectZero;
    const CCRect bounds = node->boundingBox();
    // collisionRegProj objects have a wider logical control rectangle than
    // their trimmed sprite. In particular door_sword_fire is only 22 px wide.
    const float paddingX = std::max(24.0f, (100.0f - bounds.size.width) * 0.5f);
    const float paddingY = std::max(12.0f, (98.0f - bounds.size.height) * 0.5f);
    return CCRect(bounds.origin.x - paddingX, bounds.origin.y - paddingY,
        bounds.size.width + paddingX * 2.0f, bounds.size.height + paddingY * 2.0f);
}

bool MeleeTrigger::intersects(const CCRect& attackBounds, CCNode* node) {
    return node && attackBounds.intersectsRect(interactionBounds(node));
}

bool MeleeTrigger::requiresSword(const std::string& token) {
    return token.find('s') != std::string::npos;
}

std::string MeleeTrigger::swordCode(FL_PlayerStanceType stance) {
    return stance == FL_PLAYER_STANCE_FIRE ? "rs" : "bs";
}

}
