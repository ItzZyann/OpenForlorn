#include "TriggerEventShake.h"
#include "FL_TriggerJson.h"
namespace FLTriggers { TriggerEventShake::TriggerEventShake():duration_(0),strength_(CCPointZero){}void TriggerEventShake::load(const std::string&i,const flrapidjson::Value&j){Trigger::load(i,j);duration_=Json::number(j,"duration",Json::number(j,"shakeDuration"));strength_=Json::point(j,"strength",Json::point(j,"shakeStrength"));}void TriggerEventShake::runEvent(TriggerSystem&,FL_TriggerHost&h){h.triggerShake(duration_,strength_);} }
