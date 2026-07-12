#include "MovingBlock.h"
#include "FL_TriggerJson.h"
#include "MovingBlockAction.h"
#include "MovingBlockGroup.h"
#include "../FL_Player.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace FLTriggers {

struct MovingBlockRegistry::Runtime {
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
bool overlapX(const CCRect&a,const CCRect&b){return a.getMaxX()>b.getMinX()+2&&a.getMinX()<b.getMaxX()-2;}
}

MovingBlockRegistry::MovingBlockRegistry(){}
MovingBlockRegistry::~MovingBlockRegistry(){}

MovingBlockRegistry::Definition MovingBlockRegistry::parse(const flrapidjson::Value& json){
    Definition d; d.moving=Json::lower(Json::scalar(json,"type"))=="moving"; if(!d.moving)return d;
    d.group=Json::scalar(json,"groupID");d.connectionID=Json::scalar(json,"ID");d.texture=Json::scalar(json,"texture");d.actionType=Json::lower(Json::scalar(json,"actionType"));
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

void MovingBlockRegistry::add(CCNode* node,const Definition& definition){
    if(!node||!definition.moving||definition.checkpoints.empty())return;Runtime r;r.node=node;r.definition=definition;
    r.origin=node->getPosition();r.previousPosition=r.origin;
    r.active=!definition.disabled&&!definition.spawnedByTrigger;platforms_.push_back(r);
    if(!definition.group.empty()&&definition.group!="0")groups_[definition.group].push_back(static_cast<unsigned int>(platforms_.size()-1));
}

void MovingBlockRegistry::addDecorationCandidate(CCNode*node,const std::string&lockedTo){
    if(!node||lockedTo.empty()||lockedTo=="0")return;
    DecorationCandidate c;c.node=node;c.lockedTo=lockedTo;decorationCandidates_.push_back(c);
}
void MovingBlockRegistry::resolveDecorations(){
    for(size_t c=0;c<decorationCandidates_.size();++c){DecorationCandidate&candidate=decorationCandidates_[c];
        int best=-1;
        for(size_t p=0;p<platforms_.size();++p){Runtime&platform=platforms_[p];
            if(!platform.definition.connectionID.empty()&&
                platform.definition.connectionID==candidate.lockedTo){best=static_cast<int>(p);break;}
        }
        if(best>=0)platforms_[best].decorations.push_back(candidate.node);
    }
    decorationCandidates_.clear();
}

void MovingBlockRegistry::activateGroup(const std::string& group){
    std::map<std::string,std::vector<unsigned int> >::iterator it=groups_.find(group);if(it==groups_.end())return;
    for(size_t i=0;i<it->second.size();++i){Runtime&r=platforms_[it->second[i]];r.active=true;r.finished=false;}
}
void MovingBlockRegistry::deactivateGroup(const std::string& group){
    std::map<std::string,std::vector<unsigned int> >::iterator it=groups_.find(group);if(it==groups_.end())return;
    for(size_t i=0;i<it->second.size();++i)platforms_[it->second[i]].active=false;
}
bool MovingBlockRegistry::ownsNode(CCNode* node)const{for(size_t i=0;i<platforms_.size();++i)if(platforms_[i].node==node)return true;return false;}
bool MovingBlockRegistry::intersectsActive(const CCRect&bounds)const{for(size_t i=0;i<platforms_.size();++i)if(platforms_[i].active&&platforms_[i].node&&bounds.intersectsRect(platforms_[i].node->boundingBox()))return true;return false;}

void MovingBlockRegistry::update(float dt,FL_Player* player){
    dt=std::max(0.f,std::min(dt,.1f));
    for(size_t i=0;i<platforms_.size();++i){Runtime&r=platforms_[i];if(!r.active||r.finished||!r.node)continue;
        const CCRect oldBounds=r.node->boundingBox();const CCPoint oldPosition=r.node->getPosition();
        const Checkpoint&c=r.definition.checkpoints[r.checkpoint];r.elapsed+=dt;
        CCPoint start=r.origin;for(unsigned int p=0;p<r.checkpoint;++p)start=ccpAdd(start,r.definition.checkpoints[p].offset);
        const CCPoint end=ccpAdd(start,c.offset);float t=c.duration<=.0001f?1.f:(r.elapsed-c.delay)/c.duration;
        if(r.elapsed>=c.delay)r.node->setPosition(ccpAdd(start,ccpMult(ccpSub(end,start),movingBlockEase(t,c.easing))));
        const CCPoint delta=ccpSub(r.node->getPosition(),oldPosition);const CCRect newBounds=r.node->boundingBox();
        if(std::fabs(delta.x)>.0001f||std::fabs(delta.y)>.0001f){
            for(size_t d=0;d<r.decorations.size();++d){CCNode*node=r.decorations[d];if(node)node->setPosition(ccpAdd(node->getPosition(),delta));}
        }
        if(player){
            CCRect pb=player->getCollisionBounds();const float oldTop=oldBounds.getMaxY();const float foot=pb.getMinY();
            const bool riding=player->getVelocity().y<=0.f&&overlapX(pb,oldBounds)&&std::fabs(foot-oldTop)<=8.f;
            if(riding)player->rideMovingBlock(delta,newBounds.getMaxY());
            else if(player->getVelocity().y<=0&&overlapX(pb,newBounds)&&foot<=newBounds.getMaxY()+6.f&&pb.getMaxY()>newBounds.getMaxY())
                player->landOnMovingBlock(newBounds.getMaxY());
        }
        if(t>=1.f){r.elapsed=0;++r.checkpoint;if(r.checkpoint>=r.definition.checkpoints.size()){
            if(r.definition.actionType=="once"){r.checkpoint=static_cast<unsigned int>(r.definition.checkpoints.size()-1);r.finished=true;}
            else r.checkpoint=0;
        }}r.previousPosition=r.node->getPosition();
    }
}

} // namespace FLTriggers
