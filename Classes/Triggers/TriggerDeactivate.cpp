#include "TriggerDeactivate.h"
namespace FLTriggers { void TriggerDeactivate::activateTrigger(TriggerSystem&s,FL_TriggerHost&h){s.activateGroup(callGroupID_,GroupDeactivate,false,true,h);if(!triggeredSound_.empty())h.triggerPlaySound(triggeredSound_);} }
