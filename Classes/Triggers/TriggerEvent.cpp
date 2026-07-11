#include "TriggerEvent.h"
namespace FLTriggers { void TriggerEvent::activateTrigger(TriggerSystem&s,FL_TriggerHost&h){if(cooldownRemaining_>0)return;runEvent(s,h);cooldownRemaining_=1;} }
