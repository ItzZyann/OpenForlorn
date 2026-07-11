#include "TriggerInfoText.h"
namespace FLTriggers { void TriggerInfoText::activateTrigger(TriggerSystem&,FL_TriggerHost& h){h.triggerCommand("triggerinfotext",callGroupID_,"",CCPointZero,0,true,false);} }
