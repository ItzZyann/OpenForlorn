#include "CameraTrigger.h"
#include "FL_TriggerJson.h"
#include <algorithm>
namespace FLTriggers {
TriggerCamera::TriggerCamera():offset_(CCPointZero),target_(CCPointZero),zoomTarget_(0),offsetDuration_(0),zoomDuration_(0),hasOffset_(false),hasTarget_(false),hasZoom_(false),lock_(false),instantMove_(false),instantZoom_(false){}
void TriggerCamera::load(const std::string&i,const flrapidjson::Value&j){Trigger::load(i,j);callOnlyOnce_=false;hasOffset_=j.HasMember("cameraOffset")&&j["cameraOffset"].IsArray()&&!j["cameraOffset"].Empty();offset_=Json::point(j,"cameraOffset");hasTarget_=j.HasMember("targetPosition")&&j["targetPosition"].IsArray()&&j["targetPosition"].Size()>1;target_=Json::point(j,"targetPosition");hasZoom_=j.HasMember("zoomTarget");zoomTarget_=Json::number(j,"zoomTarget");lockType_=Json::lower(Json::scalar(j,"cameraLockType","full"));lock_=Json::boolean(j,"lockCamera");offsetDuration_=std::max(0.f,Json::number(j,"offsetDuration"));zoomDuration_=std::max(0.f,Json::number(j,"zoomDuration"));offsetEasing_=Json::scalar(j,"offsetEasingType","InOut");zoomEasing_=Json::scalar(j,"zoomEasingType","InOut");instantMove_=Json::boolean(j,"instantMove");instantZoom_=Json::boolean(j,"instantZoom");}
void TriggerCamera::activateTrigger(TriggerSystem&,FL_TriggerHost&h){bool reset=lockType_.empty()||lockType_=="0"||lockType_=="none";h.triggerCamera(offset_,hasOffset_,zoomTarget_,hasZoom_,lock_,target_,hasTarget_,reset,offsetDuration_,zoomDuration_,offsetEasing_,zoomEasing_,instantMove_,instantZoom_);}
void TriggerCamera::update(float dt,const CCPoint&p,TriggerSystem&s,FL_TriggerHost&h){Trigger::update(dt,p,s,h);}
void TriggerCamera::leaveSpatialZone(FL_TriggerHost&h){if(lock_)h.triggerReleaseCamera();}
}
