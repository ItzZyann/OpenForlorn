#include "TeleportTrigger.h"
#include "FL_TriggerJson.h"
namespace FLTriggers {

TeleportTrigger::TeleportTrigger():target_(CCPointZero),fade_(false){}

void TeleportTrigger::load(const std::string&i,const flrapidjson::Value&j){
    Trigger::load(i,j);
    target_=Json::point(j,"targetPosition");
    fade_=Json::boolean(j,"fadeMode");

    // Authored teleport pairs (for example t_1/t_2 in Level001) are called by
    // reusable TriggerOnOff zones.  The original teleport object can therefore
    // be activated every time its group is called.  Keep an explicit plist
    // callOnlyOnce value, but do not accidentally make an unspecified teleport
    // single-use through Trigger's generic default.
    if(!j.HasMember("callOnlyOnce")) callOnlyOnce_=false;
}

void TeleportTrigger::activateTrigger(TriggerSystem&,FL_TriggerHost&h){
    h.triggerTeleport(target_,fade_);
    if(!triggeredSound_.empty())h.triggerPlaySound(triggeredSound_);
}

}
