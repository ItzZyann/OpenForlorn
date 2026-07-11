#include "TeleportTrigger.h"
#include "FL_TriggerJson.h"
namespace FLTriggers { TeleportTrigger::TeleportTrigger():target_(CCPointZero),fade_(false){}void TeleportTrigger::load(const std::string&i,const flrapidjson::Value&j){Trigger::load(i,j);target_=Json::point(j,"targetPosition");fade_=Json::boolean(j,"fadeMode");}void TeleportTrigger::activateTrigger(TriggerSystem&,FL_TriggerHost&h){h.triggerTeleport(target_,fade_);if(!triggeredSound_.empty())h.triggerPlaySound(triggeredSound_);} }
