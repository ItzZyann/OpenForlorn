#pragma once

#include "../incl.h"
#include "../rapidjson/document.h"

#include <map>
#include <string>
#include <vector>

class FL_Player;

namespace FLTriggers {

class MovingPlatformController {
public:
    struct Checkpoint {
        CCPoint offset;
        float delay;
        float duration;
        std::string easing;
        std::string spawnGroup;
        Checkpoint():offset(CCPointZero),delay(0),duration(0),easing("-"){}
    };

    struct Definition {
        std::string group;
        std::string actionType;
        std::string texture;
        std::vector<Checkpoint> checkpoints;
        bool disabled;
        bool spawnedByTrigger;
        bool invisible;
        bool moving;
        Definition():actionType("repeat"),disabled(false),spawnedByTrigger(false),invisible(false),moving(false){}
    };

    MovingPlatformController();
    ~MovingPlatformController();
    static Definition parse(const flrapidjson::Value& json);
    void add(CCNode* node, const Definition& definition);
    void addDecoration(CCNode* node, const std::string& group);
    void addDecorationCandidate(CCNode* node, const std::string& group, const std::string& objectType, const std::string& texture);
    void resolveDecorations();
    void activateGroup(const std::string& group);
    void deactivateGroup(const std::string& group);
    void update(float dt, FL_Player* player);
    bool ownsNode(CCNode* node) const;

private:
    struct Runtime;
    std::vector<Runtime> platforms_;
    std::map<std::string, std::vector<unsigned int> > groups_;
    struct DecorationCandidate { CCNode* node; std::string group; std::string type; std::string texture; };
    std::vector<DecorationCandidate> decorationCandidates_;
};

} // namespace FLTriggers
