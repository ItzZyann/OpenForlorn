#include "TriggerExitLevel.h"
#include "FL_TriggerJson.h"
namespace FLTriggers { TriggerExitLevel::TriggerExitLevel():dontMove_(false){}void TriggerExitLevel::load(const std::string&i,const flrapidjson::Value&j){Trigger::load(i,j);dontMove_=Json::boolean(j,"dontMove");callOnlyOnce_=true;}void TriggerExitLevel::activateTrigger(TriggerSystem&,FL_TriggerHost&h){h.triggerExitLevel(dontMove_);} }
