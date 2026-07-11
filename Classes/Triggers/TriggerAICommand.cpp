#include "TriggerAICommand.h"
namespace FLTriggers {
void TriggerAICommand::load(const std::string& id,const flrapidjson::Value& j){Trigger::load(id,j);if(j.HasMember("targetGroupID"))target_=j["targetGroupID"].IsString()?j["targetGroupID"].GetString():"";if(j.HasMember("commandGroupID"))command_=j["commandGroupID"].IsString()?j["commandGroupID"].GetString():"";}
void TriggerAICommand::activateTrigger(TriggerSystem&,FL_TriggerHost& h){h.triggerCommand("triggeraicommand",target_,command_,CCPointZero,0,true,false);if(!triggeredSound_.empty())h.triggerPlaySound(triggeredSound_);}
}
