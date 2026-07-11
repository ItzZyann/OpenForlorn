#include "Triggers/FL_TriggerSystem.h"
#include "Triggers/FL_TriggerJson.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace FLTriggers {

TriggerSystem::TriggerSystem():activeSpatialCamera_(NULL){}
bool TriggerSystem::load(const flrapidjson::Value& c){if(!c.IsObject())return false;
 for(flrapidjson::Value::ConstMemberIterator it=c.MemberBegin();it!=c.MemberEnd();++it){if(!it->value.IsObject())continue;std::string t=Json::lower(Json::scalar(it->value,"Type"));std::shared_ptr<Trigger>x;
  if(t=="cameratrigger")x.reset(new TriggerCamera);else if(t=="triggercameramove")x.reset(new TriggerCameraMove);else if(t=="triggeronoff")x.reset(new TriggerOnOff);else if(t=="triggerdeactivate")x.reset(new TriggerDeactivate);else if(t=="triggerexitlevel")x.reset(new TriggerExitLevel);else if(t=="teleporttrigger")x.reset(new TeleportTrigger);else if(t=="triggereventshake")x.reset(new TriggerEventShake);else if(t=="triggeraicommand")x.reset(new TriggerAICommand);else x.reset(new TriggerCommand);
  x->load(it->name.GetString(),it->value);triggers_.push_back(x);if(!x->groupID().empty())groups_[x->groupID()].push_back(x.get());}
 std::sort(triggers_.begin(),triggers_.end(),[](const std::shared_ptr<Trigger>&a,const std::shared_ptr<Trigger>&b){return a->priority()<b->priority();});return true;}
void TriggerSystem::update(float dt,const CCPoint&p,FL_TriggerHost&h){
 TriggerCamera* selectedCamera=NULL;
 for(size_t i=0;i<triggers_.size();++i){
  Trigger&t=*triggers_[i];t.update(dt,p,*this,h);
  TriggerCamera*camera=dynamic_cast<TriggerCamera*>(&t);
  if(camera){
   if(!camera->calledByEvent()&&camera->contains(p)&&(!selectedCamera||camera->priority()>selectedCamera->priority()))selectedCamera=camera;
   continue;
  }
  bool inside=!t.calledByEvent()&&t.contains(p);
  if(inside&&!t.insideLastFrame_)t.execute(*this,h);
  t.insideLastFrame_=inside;
 }
 if(selectedCamera!=activeSpatialCamera_){
  if(activeSpatialCamera_)activeSpatialCamera_->leaveSpatialZone(h);
  activeSpatialCamera_=selectedCamera;
  if(activeSpatialCamera_)activeSpatialCamera_->execute(*this,h);
 }
}
void TriggerSystem::activateGroup(const std::string&g,GroupCallType type,bool wake,bool sleep,FL_TriggerHost&h){std::map<std::string,std::vector<Trigger*> >::iterator it=groups_.find(g);if(it==groups_.end())return;for(size_t i=0;i<it->second.size();++i){Trigger*t=it->second[i];if(wake)t->setSleeping(false);if(sleep)t->setSleeping(true);if(type==GroupActivate||type==GroupResume)t->activateObject(false,*this,h);else if(type==GroupDeactivate||type==GroupPause)t->activateObject(true,*this,h);else t->setSleeping(!t->sleeping());}}
void TriggerSystem::reset(){activeSpatialCamera_=NULL;for(size_t i=0;i<triggers_.size();++i)triggers_[i]->resetObject();}
}
