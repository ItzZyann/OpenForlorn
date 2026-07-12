#include "MovingPlatform.h"
#include "FL_TriggerJson.h"
#include "../FL_Player.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace FLTriggers {

struct MovingPlatformController::Runtime {
    CCNode* node;
    Definition definition;
    CCPoint origin;
    CCPoint previousPosition;
    unsigned int checkpoint;
    float elapsed;
    bool active;
    bool finished;
    std::vector<CCNode*> decorations;
    Runtime():node(NULL),origin(CCPointZero),previousPosition(CCPointZero),checkpoint(0),elapsed(0),active(false),finished(false){}
};

namespace {
float ease(float t,const std::string& name){
    t=std::max(0.f,std::min(1.f,t)); const std::string n=Json::lower(name);
    if(n=="in")return t*t; if(n=="out")return 1.f-(1.f-t)*(1.f-t);
    if(n=="inout")return t<.5f?2.f*t*t:1.f-2.f*(1.f-t)*(1.f-t);
    if(n=="bounceout"){
        if(t<1.f/2.75f)return 7.5625f*t*t;
        if(t<2.f/2.75f){t-=1.5f/2.75f;return 7.5625f*t*t+.75f;}
        if(t<2.5f/2.75f){t-=2.25f/2.75f;return 7.5625f*t*t+.9375f;}
        t-=2.625f/2.75f;return 7.5625f*t*t+.984375f;
    }
    if(n=="elasticout") return t==0||t==1?t:std::pow(2.f,-10.f*t)*std::sin((t-.075f)*20.943951f)+1.f;
    return t;
}
bool overlapX(const CCRect&a,const CCRect&b){return a.getMaxX()>b.getMinX()+2&&a.getMinX()<b.getMaxX()-2;}
bool containsText(const std::string&value,const char*part){return value.find(part)!=std::string::npos;}
bool isAllowedDecoration(const MovingPlatformController::Definition&platform,const std::string&type,const std::string&texture,float distance){
    // Power-stone platforms are authored as a compact pile of independent
    // disabled/particle objects without group ids.
    if(containsText(platform.texture,"powerstone_platform_green"))
        return (type=="disabled"||type=="particle")&&distance<=(platform.invisible?60.f:90.f);
    if(containsText(platform.texture,"powerstone_platform_small_green"))return false;
    // The two invisible doorway movers in Level002 are collision anchors for
    // the actual large falling icicles. Nearby chains belong to the room.
    if(containsText(platform.texture,"doorway_rightside"))return containsText(texture,"sn_ice_icicle_large")&&distance<=115.f;
    // Rune movers drive the compact trap assemblies to their left/right. Keep
    // only structural trap art; scenery, fireflies and barricades stay put.
    if(containsText(platform.texture,"runes_long_green")){
        const bool trapPart=containsText(texture,"pillar_mid")||containsText(texture,"pillar_top")||
            containsText(texture,"icicle_row_platform")||containsText(texture,"crystal_purple")||
            containsText(texture,"relief_ring")||containsText(texture,"shine_circleStrong");
        return trapPart&&distance>=135.f&&distance<=190.f;
    }
    return false;
}
}

MovingPlatformController::MovingPlatformController(){}
MovingPlatformController::~MovingPlatformController(){}

MovingPlatformController::Definition MovingPlatformController::parse(const flrapidjson::Value& json){
    Definition d; d.moving=Json::lower(Json::scalar(json,"type"))=="moving"; if(!d.moving)return d;
    d.group=Json::scalar(json,"groupID");d.texture=Json::scalar(json,"texture");d.actionType=Json::lower(Json::scalar(json,"actionType"));
    if(d.actionType.empty()||d.actionType=="-")d.actionType="repeat";
    d.disabled=Json::boolean(json,"disabled",false);
    d.spawnedByTrigger=Json::boolean(json,"spawnedByTrigger",false);
    d.invisible=Json::boolean(json,"invisible",false);
    if(json.HasMember("checkpoints")&&json["checkpoints"].IsObject()){
        std::vector<std::pair<int,Checkpoint> > ordered;
        for(flrapidjson::Value::ConstMemberIterator it=json["checkpoints"].MemberBegin();it!=json["checkpoints"].MemberEnd();++it){
            int index=-1;if(std::sscanf(it->name.GetString(),"point_%d",&index)!=1||index<0||!it->value.IsObject())continue;
            Checkpoint c;c.offset=Json::point(it->value,"position");c.delay=std::max(0.f,Json::number(it->value,"delay"));
            c.duration=std::max(0.f,Json::number(it->value,"duration"));c.easing=Json::scalar(it->value,"easingType");
            c.spawnGroup=Json::scalar(it->value,"spawnGroupID");ordered.push_back(std::make_pair(index,c));
        }
        std::sort(ordered.begin(),ordered.end(),[](const std::pair<int,Checkpoint>&a,const std::pair<int,Checkpoint>&b){return a.first<b.first;});
        for(size_t i=0;i<ordered.size();++i)d.checkpoints.push_back(ordered[i].second);
    }
    if(d.checkpoints.empty()){
        Checkpoint c;c.offset=Json::point(json,"target");c.delay=std::max(0.f,Json::number(json,"delay"));
        c.duration=std::max(0.f,Json::number(json,"duration"));c.easing=Json::scalar(json,"easingType");d.checkpoints.push_back(c);
    }
    return d;
}

void MovingPlatformController::add(CCNode* node,const Definition& definition){
    if(!node||!definition.moving||definition.checkpoints.empty())return;Runtime r;r.node=node;r.definition=definition;
    r.origin=node->getPosition();r.previousPosition=r.origin;
    r.active=!definition.disabled&&!definition.spawnedByTrigger;platforms_.push_back(r);
    if(!definition.group.empty()&&definition.group!="0")groups_[definition.group].push_back(static_cast<unsigned int>(platforms_.size()-1));
}

void MovingPlatformController::addDecoration(CCNode* node,const std::string& group){
    addDecorationCandidate(node,group,"disabled","");
}
void MovingPlatformController::addDecorationCandidate(CCNode*node,const std::string&group,const std::string&objectType,const std::string&texture){
    if(!node)return;DecorationCandidate c;c.node=node;c.group=group;c.type=Json::lower(objectType);c.texture=texture;decorationCandidates_.push_back(c);
}
void MovingPlatformController::resolveDecorations(){
    for(size_t c=0;c<decorationCandidates_.size();++c){DecorationCandidate&candidate=decorationCandidates_[c];
        int best=-1;float bestDistance=1e30f;const CCPoint position=candidate.node->getPosition();
        for(size_t p=0;p<platforms_.size();++p){Runtime&platform=platforms_[p];
            const bool explicitGroup=!candidate.group.empty()&&candidate.group!="0"&&candidate.group==platform.definition.group;
            const float dx=position.x-platform.origin.x,dy=position.y-platform.origin.y,distance=std::sqrt(dx*dx+dy*dy);
            if(!explicitGroup&&!isAllowedDecoration(platform.definition,candidate.type,candidate.texture,distance))continue;
            const float score=explicitGroup?distance-10000.f:distance;if(score<bestDistance){bestDistance=score;best=static_cast<int>(p);}
        }
        if(best>=0)platforms_[best].decorations.push_back(candidate.node);
    }
    decorationCandidates_.clear();
}

void MovingPlatformController::activateGroup(const std::string& group){
    std::map<std::string,std::vector<unsigned int> >::iterator it=groups_.find(group);if(it==groups_.end())return;
    for(size_t i=0;i<it->second.size();++i){Runtime&r=platforms_[it->second[i]];r.active=true;r.finished=false;}
}
void MovingPlatformController::deactivateGroup(const std::string& group){
    std::map<std::string,std::vector<unsigned int> >::iterator it=groups_.find(group);if(it==groups_.end())return;
    for(size_t i=0;i<it->second.size();++i)platforms_[it->second[i]].active=false;
}
bool MovingPlatformController::ownsNode(CCNode* node)const{for(size_t i=0;i<platforms_.size();++i)if(platforms_[i].node==node)return true;return false;}

void MovingPlatformController::update(float dt,FL_Player* player){
    dt=std::max(0.f,std::min(dt,.1f));
    for(size_t i=0;i<platforms_.size();++i){Runtime&r=platforms_[i];if(!r.active||r.finished||!r.node)continue;
        const CCRect oldBounds=r.node->boundingBox();const CCPoint oldPosition=r.node->getPosition();
        const Checkpoint&c=r.definition.checkpoints[r.checkpoint];r.elapsed+=dt;
        CCPoint start=r.origin;for(unsigned int p=0;p<r.checkpoint;++p)start=ccpAdd(start,r.definition.checkpoints[p].offset);
        const CCPoint end=ccpAdd(start,c.offset);float t=c.duration<=.0001f?1.f:(r.elapsed-c.delay)/c.duration;
        if(r.elapsed>=c.delay)r.node->setPosition(ccpAdd(start,ccpMult(ccpSub(end,start),ease(t,c.easing))));
        const CCPoint delta=ccpSub(r.node->getPosition(),oldPosition);const CCRect newBounds=r.node->boundingBox();
        if(std::fabs(delta.x)>.0001f||std::fabs(delta.y)>.0001f){
            for(size_t d=0;d<r.decorations.size();++d){CCNode*node=r.decorations[d];if(node)node->setPosition(ccpAdd(node->getPosition(),delta));}
        }
        if(player){
            CCRect pb=player->getCollisionBounds();const float oldTop=oldBounds.getMaxY();const float foot=pb.getMinY();
            const bool riding=overlapX(pb,oldBounds)&&std::fabs(foot-oldTop)<=8.f;
            if(riding)player->rideMovingPlatform(delta,newBounds.getMaxY());
            else if(player->getVelocity().y<=0&&overlapX(pb,newBounds)&&foot<=newBounds.getMaxY()+6.f&&pb.getMaxY()>newBounds.getMaxY())
                player->landOnMovingPlatform(newBounds.getMaxY());
        }
        if(t>=1.f){r.elapsed=0;++r.checkpoint;if(r.checkpoint>=r.definition.checkpoints.size()){
            if(r.definition.actionType=="once"){r.checkpoint=static_cast<unsigned int>(r.definition.checkpoints.size()-1);r.finished=true;}
            else r.checkpoint=0;
        }}r.previousPosition=r.node->getPosition();
    }
}

} // namespace FLTriggers
