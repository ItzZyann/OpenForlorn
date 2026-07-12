#pragma once
#include "../FL_Block.h"
#include "../FL_CollisionWorld.h"
#include <vector>
class MovableBlock : public FL_Block {
public:
    static MovableBlock* create(const Data& data);
    bool initMovable(const Data& data);
    void activateObject(bool deactivating);
    void deactivateObject(bool resetPosition);
    void resetObject();
    void updateMovable(float dt, const FL_CollisionWorld& world);
    bool pushHorizontal(float distance, const FL_CollisionWorld& world,
        const std::vector<MovableBlock*>& blocks);
    bool active() const { return active_; }
    const CCPoint& spawnPosition() const { return spawnPosition_; }
    const CCPoint& lastDelta() const { return lastDelta_; }
protected:
    MovableBlock();
    CCPoint spawnPosition_, velocity_, lastDelta_;
    bool active_, trapControlled_, ethereal_, dead_;
};
