#pragma once
#include "rapidjson/document.h"
#include <cstring>
#include <cstdlib>

inline bool GetBoolFlexible(const flrapidjson::Value& obj, const char* key) {
	if (!obj.HasMember(key))
		return false;

	const flrapidjson::Value& v = obj[key];

	if (v.IsBool())
		return v.GetBool();

	if (v.IsInt())
		return v.GetInt() != 0;

	if (v.IsString()) {
		const char* s = v.GetString();
		if (strcmp(s, "1") == 0) return true;
		if (strcmp(s, "0") == 0) return false;
		if (strcmp(s, "true") == 0) return true;
		if (strcmp(s, "false") == 0) return false;
	}

	return false;
}

// added this too
// this took me 10 minutes of debugging
// and this is just the fk. error
inline bool GetBoolFlexibleFromArray(const flrapidjson::Value& arr, const char* indexStr) {
	int index = atoi(indexStr);
	if (!arr.IsArray() || index < 0 || (flrapidjson::SizeType)index >= arr.Size())
		return false;

	const flrapidjson::Value& v = arr[(flrapidjson::SizeType)index];

	if (v.IsBool())
		return v.GetBool();

	if (v.IsInt())
		return v.GetInt() != 0;

	if (v.IsString()) {
		const char* s = v.GetString();
		if (strcmp(s, "1") == 0) return true;
		if (strcmp(s, "0") == 0) return false;
		if (strcmp(s, "true") == 0) return true;
		if (strcmp(s, "false") == 0) return false;
	}

	return false;
}

inline int GetIntSafe(const flrapidjson::Value& obj, const char* key, int defaultValue = 0) {
	if (!obj.HasMember(key))
		return defaultValue;

	const flrapidjson::Value& v = obj[key];
	if (v.IsInt())
		return v.GetInt();

	if (v.IsString())
		return atoi(v.GetString());

	return defaultValue;
}

inline float GetFloatSafe(const flrapidjson::Value& obj, const char* key, float defaultValue = 0.f) {
	if (!obj.HasMember(key))
		return defaultValue;

	const flrapidjson::Value& v = obj[key];
	if (v.IsNumber())
		return v.GetFloat();

	if (v.IsString())
		return static_cast<float>(atof(v.GetString()));

	return defaultValue;
}

inline const char* GetStringSafe(const flrapidjson::Value& obj, const char* key, const char* defaultValue = "") {
	if (!obj.HasMember(key))
		return defaultValue;

	const flrapidjson::Value& v = obj[key];
	if (v.IsString())
		return v.GetString();

	return defaultValue;
}

// NOTE ONLY LEVEL DATA 1 AND 4 WORKS
// IDK ABOUT OTHERS, THEY DO HAVE ERROR