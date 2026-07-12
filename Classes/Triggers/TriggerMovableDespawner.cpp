#include "TriggerMovableDespawner.h"
#include <sstream>
namespace FLTriggers {
void TriggerMovableDespawner::activateTrigger(TriggerSystem&,FL_TriggerHost&host){
    std::ostringstream height;height<<bounds_.size.height;
    host.triggerCommand("triggermovabledespawner","",height.str(),
        ccp(bounds_.getMidX(),bounds_.getMidY()),bounds_.size.width,true,false);
}
}
