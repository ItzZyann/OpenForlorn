#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#include "Trigger.h"
#include "FL_TriggerJson.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace FLTriggers {
Trigger::Trigger():bounds_(0,0,0,0),priority_(0),triggerDelay_(0),delayRemaining_(0),cooldownRemaining_(0),callOnlyOnce_(true),calledByEvent_(false),hasBeenExecuted_(false),sleeping_(false),defaultSleeping_(false),pending_(false),insideLastFrame_(false){}
void Trigger::load(const std::string&id,const flrapidjson::Value&j){id_=id;groupID_=Json::scalar(j,"groupID");callGroupID_=Json::scalar(j,"spawnGroupID");triggeredSound_=Json::scalar(j,"triggeredSound");priority_=static_cast<int>(Json::number(j,"p_uID"));triggerDelay_=std::max(0.f,Json::number(j,"triggerDelay"));callOnlyOnce_=Json::boolean(j,"callOnlyOnce",true);calledByEvent_=Json::boolean(j,"calledByEvent");defaultSleeping_=Json::boolean(j,"sleeping");sleeping_=defaultSleeping_;CCPoint p=Json::point(j,"position"),s=Json::point(j,"scale",ccp(1,1));float w=100*std::max(.01f,std::fabs(s.x)),h=100*std::max(.01f,std::fabs(s.y));bounds_=CCRect(p.x-w*.5f,p.y-h*.5f,w,h);}
bool Trigger::contains(const CCPoint&p)const{return bounds_.containsPoint(p);}void Trigger::activateObject(bool d,TriggerSystem&s,FL_TriggerHost&h){if(!sleeping_&&!d)execute(s,h);}void Trigger::execute(TriggerSystem&s,FL_TriggerHost&h){if(sleeping_||(hasBeenExecuted_&&callOnlyOnce_))return;hasBeenExecuted_=true;if(callOnlyOnce_&&!calledByEvent_)sleeping_=true;if(triggerDelay_<=0)activateTrigger(s,h);else{pending_=true;delayRemaining_=triggerDelay_;}}
void Trigger::activateTrigger(TriggerSystem&s,FL_TriggerHost&h){if(!callGroupID_.empty()&&callGroupID_!="0")s.activateGroup(callGroupID_,GroupActivate,true,false,h);if(!triggeredSound_.empty())h.triggerPlaySound(triggeredSound_);}void Trigger::update(float dt,const CCPoint&,TriggerSystem&s,FL_TriggerHost&h){cooldownRemaining_=std::max(0.f,cooldownRemaining_-dt);if(pending_){delayRemaining_-=dt;if(delayRemaining_<=0){pending_=false;activateTrigger(s,h);}}}void Trigger::resetObject(){hasBeenExecuted_=false;sleeping_=defaultSleeping_;pending_=false;insideLastFrame_=false;}
}
