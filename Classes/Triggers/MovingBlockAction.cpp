#include "MovingBlockAction.h"
#include "FL_TriggerJson.h"
#include <algorithm>
#include <cmath>
namespace FLTriggers {
float movingBlockEase(float t,const std::string& name){
    t=std::max(0.f,std::min(1.f,t));const std::string n=Json::lower(name);
    if(n=="in")return t*t;if(n=="out")return 1.f-(1.f-t)*(1.f-t);
    if(n=="inout")return t<.5f?2.f*t*t:1.f-2.f*(1.f-t)*(1.f-t);
    if(n=="bounceout"){if(t<1.f/2.75f)return 7.5625f*t*t;if(t<2.f/2.75f){t-=1.5f/2.75f;return 7.5625f*t*t+.75f;}if(t<2.5f/2.75f){t-=2.25f/2.75f;return 7.5625f*t*t+.9375f;}t-=2.625f/2.75f;return 7.5625f*t*t+.984375f;}
    if(n=="elasticout")return t==0||t==1?t:std::pow(2.f,-10.f*t)*std::sin((t-.075f)*20.943951f)+1.f;
    return t;
}
}
