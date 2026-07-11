#include "CheckpointTrigger.h"
namespace FLTriggers { void CheckpointTrigger::activateTrigger(TriggerSystem&, FL_TriggerHost& h) { h.triggerCommand("triggereventsafespot", "", "", CCPointZero, 0, true, false); if (!triggeredSound_.empty()) h.triggerPlaySound(triggeredSound_); } }
