#include "TriggerGroupController.h"
namespace FLTriggers { void TriggerGroupController::activateTrigger(TriggerSystem& s,FL_TriggerHost& h){if(!callGroupID_.empty())s.activateGroup(callGroupID_,GroupSwitchState,true,false,h);} }
