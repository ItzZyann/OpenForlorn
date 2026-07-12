#pragma once
#include "FL_TriggerSystem.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace std;
namespace FLTriggers { namespace Json {
inline std::string scalar(const flrapidjson::Value&o,const char*k,const char*f=""){if(!o.HasMember(k))return f;const flrapidjson::Value&v=o[k];if(v.IsString())return v.GetString();char b[64];if(v.IsInt()){snprintf(b,sizeof(b),"%d",v.GetInt());return b;}if(v.IsNumber()){snprintf(b,sizeof(b),"%g",v.GetDouble());return b;}return f;}
inline float number(const flrapidjson::Value&o,const char*k,float f=0){if(!o.HasMember(k))return f;const flrapidjson::Value&v=o[k];if(v.IsNumber())return v.GetFloat();if(v.IsString())return static_cast<float>(std::atof(v.GetString()));return f;}
inline bool boolean(const flrapidjson::Value&o,const char*k,bool f=false){if(!o.HasMember(k))return f;const flrapidjson::Value&v=o[k];if(v.IsBool())return v.GetBool();if(v.IsInt())return v.GetInt()!=0;if(v.IsString())return std::string(v.GetString())=="1"||std::string(v.GetString())=="true";return f;}
inline CCPoint point(const flrapidjson::Value&o,const char*k,const CCPoint&f=CCPointZero){if(!o.HasMember(k))return f;const flrapidjson::Value&v=o[k];if(!v.IsArray())return f;return ccp(v.Size()>0&&v[0].IsNumber()?v[0].GetFloat():f.x,v.Size()>1&&v[1].IsNumber()?v[1].GetFloat():f.y);}
inline std::string lower(std::string v){std::transform(v.begin(),v.end(),v.begin(),::tolower);return v;}
}}
