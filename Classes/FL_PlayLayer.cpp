#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#include "FL_PlayLayer.h"
#include "FL_MenuLayer.h"
#include "FL_UILayer.h"
#include "FL_Player.h"
#include "FL_CollisionWorld.h"
#include "JsonUtils.h"

#include "rapidjson/document.h"
#include "rapidjson/error/error.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
#include <GL/glfw.h>
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
#include <ApplicationServices/ApplicationServices.h>
#endif

USING_NS_CC;

namespace {

	struct LevelObject {
		std::string id;
		std::string objectType;
		FL_Block::Data data;
		std::vector<CCPoint> collisionPoints;
		bool passable;
		bool blocksMovement;

		LevelObject()
			: passable(false)
			, blocksMovement(false) {
		}
	};

	struct NpcSpawn {
		std::string id;
		std::string npcType;
		CCPoint position;
		CCPoint scale;
		bool flippedX;
		bool flippedY;
		float rotation;
		std::string layerType;
		int zOrder;

		NpcSpawn()
			: position(CCPointZero)
			, scale(ccp(1.0f, 1.0f))
			, flippedX(false)
			, flippedY(false)
			, rotation(0.0f)
			, layerType("NPC")
			, zOrder(0) {
		}
	};

	struct CameraTriggerData {
		std::string id;
		CCRect bounds;
		CCPoint cameraOffset;
		CCPoint targetPosition;
		std::string cameraLockType;
		std::string offsetEasingType;
		std::string zoomEasingType;
		float zoomTarget;
		float offsetDuration;
		float zoomDuration;
		float triggerDelay;
		int pUID;
		bool hasCameraOffset;
		bool hasTargetPosition;
		bool hasZoomTarget;
		bool calledByEvent;
		bool lockCamera;
		bool instantMove;
		bool instantZoom;
		bool wasInside;
		bool firedThisVisit;
		float insideTime;

		CameraTriggerData()
			: bounds(CCRect(0.0f, 0.0f, 0.0f, 0.0f))
			, cameraOffset(CCPointZero)
			, targetPosition(CCPointZero)
			, zoomTarget(0.0f)
			, offsetDuration(0.0f)
			, zoomDuration(0.0f)
			, triggerDelay(0.0f)
			, pUID(0)
			, hasCameraOffset(false)
			, hasTargetPosition(false)
			, hasZoomTarget(false)
			, calledByEvent(false)
			, lockCamera(false)
			, instantMove(false)
			, instantZoom(false)
			, wasInside(false)
			, firedThisVisit(false)
			, insideTime(0.0f) {
		}
	};

	struct LevelStruct {
		struct SheetData {
			std::string texture;
			std::string type;

			bool operator<(const SheetData& other) const {
				if (texture != other.texture) return texture < other.texture;
				return type < other.type;
			}
		};

		std::vector<LevelObject> blocks;
		std::vector<LevelObject> backgrounds;
		std::vector<NpcSpawn> npcSpawns;
		std::vector<CameraTriggerData> cameraTriggers;
		FL_CollisionWorld collisionWorld;
		std::set<SheetData> sheets;
		CCPoint playerSpawn;
		CCSize levelSize;
		std::string backgroundImage;
		std::string backgroundMusic;
		float backgroundOffsetY;

		LevelStruct()
			: playerSpawn(ccp(300.0f, 300.0f))
			, levelSize(CCSizeMake(512.0f, 399.0f))
			, backgroundOffsetY(0.0f) {
		}
	};

	struct EntityPart {
		std::string texture;
		CCPoint position;
		CCPoint scale;
		bool flippedX;
		bool flippedY;
		float rotation;
		int zOrder;

		EntityPart()
			: position(CCPointZero)
			, scale(ccp(1.0f, 1.0f))
			, flippedX(false)
			, flippedY(false)
			, rotation(0.0f)
			, zOrder(0) {
		}
	};

	struct EntityFrame {
		std::vector<EntityPart> parts;
	};

	struct EntityDescriptor {
		std::string npcType;
		std::string sheetName;
		std::vector<EntityFrame> frames;
		float frameDelay;
		float displayScale;
		CCSize boundsSize;
		int contactDamage;

		EntityDescriptor()
			: frameDelay(0.1f)
			, displayScale(1.0f)
			, boundsSize(CCSizeMake(128.0f, 128.0f))
			, contactDamage(0) {
		}
	};

	struct EntityRuntime {
		CCNode* root;
		std::shared_ptr<EntityDescriptor> descriptor;
		std::vector<CCSprite*> sprites;
		unsigned int frameIndex;
		float elapsed;
		bool active;
		bool visible;

		EntityRuntime()
			: root(NULL)
			, frameIndex(0)
			, elapsed(0.0f)
			, active(true)
			, visible(true) {
		}
	};

	struct RuntimeNode {
		CCNode* node;
		CCRect bounds;
		bool active;
		bool visible;
		bool pauseActions;
		bool trackNodeBounds;
		float boundsPadding;
		int warmupFrames;
		int entityIndex;

		RuntimeNode()
			: node(NULL)
			, active(true)
			, visible(true)
			, pauseActions(false)
			, trackNodeBounds(false)
			, boundsPadding(0.0f)
			, warmupFrames(0)
			, entityIndex(-1) {
		}
	};

	std::string toLower(const std::string& value) {
		std::string result = value;
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return result;
	}

	// Objects with explicit `points` use those points. These block types store their
	// collision implicitly in the sprite transform, so their transformed sprite
	// bounds must also be inserted into the collision world.
	bool objectTypeUsesSpriteCollision(const std::string& rawType) {
		const std::string type = toLower(rawType);
		return type == "solid" ||
			type == "moving" ||
			type == "shaky" ||
			type == "rolling" ||
			type == "movable" ||
			type == "destructable" ||
			type == "bouncy" ||
			type == "blockspring" ||
			type == "statue" ||
			type == "trapfalling";
	}

	bool endsWithIgnoreCase(const std::string& value, const std::string& suffix) {
		if (value.size() < suffix.size()) return false;
		return toLower(value.substr(value.size() - suffix.size())) == toLower(suffix);
	}

	std::string replaceExtension(const std::string& fileName, const std::string& extension) {
		const std::string::size_type dot = fileName.find_last_of('.');
		return (dot == std::string::npos ? fileName : fileName.substr(0, dot)) + extension;
	}

	bool fileExists(const std::string& fileName) {
		unsigned long size = 0;
		unsigned char* data = CCFileUtils::sharedFileUtils()->getFileData(fileName.c_str(), "rb", &size);
		const bool exists = data != NULL && size > 0;
		if (data) delete[] data;
		return exists;
	}

	bool isHDSheetName(const std::string& sheetName) {
		static const char* const noHD[] = {
			"ForestSheet_MG",
			"lightSheet",
			"FrostLevel_MGSheet",
			"FlameVillageMG",
			"particleImgSheet"
		};

		for (unsigned int index = 0; index < sizeof(noHD) / sizeof(noHD[0]); ++index) {
			if (sheetName == noHD[index]) return false;
		}
		return true;
	}

	float jsonNumber(const flrapidjson::Value& value, float fallback) {
		if (value.IsNumber()) return static_cast<float>(value.GetDouble());
		if (value.IsBool()) return value.GetBool() ? 1.0f : 0.0f;
		if (value.IsString()) {
			char* end = NULL;
			const double parsed = std::strtod(value.GetString(), &end);
			if (end && end != value.GetString()) return static_cast<float>(parsed);
		}
		return fallback;
	}

	float jsonNumber(const flrapidjson::Value& object, const char* key, float fallback) {
		if (!object.IsObject() || !object.HasMember(key)) return fallback;
		return jsonNumber(object[key], fallback);
	}

	int jsonInt(const flrapidjson::Value& object, const char* key, int fallback) {
		return static_cast<int>(jsonNumber(object, key, static_cast<float>(fallback)));
	}

	bool jsonBool(const flrapidjson::Value& value, bool fallback) {
		if (value.IsBool()) return value.GetBool();
		if (value.IsNumber()) return value.GetDouble() != 0.0;
		if (value.IsString()) {
			const std::string normalized = toLower(value.GetString());
			if (normalized == "true" || normalized == "yes" || normalized == "1") return true;
			if (normalized == "false" || normalized == "no" || normalized == "0") return false;
		}
		return fallback;
	}

	bool jsonBool(const flrapidjson::Value& object, const char* key, bool fallback) {
		if (!object.IsObject() || !object.HasMember(key)) return fallback;
		return jsonBool(object[key], fallback);
	}

	std::string jsonString(const flrapidjson::Value& object, const char* key, const std::string& fallback = std::string()) {
		if (!object.IsObject() || !object.HasMember(key)) return fallback;
		const flrapidjson::Value& value = object[key];
		if (value.IsString()) return value.GetString();
		return fallback;
	}

	std::string jsonScalarString(
		const flrapidjson::Value& object,
		const char* key,
		const std::string& fallback = std::string()
		) {
		if (!object.IsObject() || !object.HasMember(key)) return fallback;
		const flrapidjson::Value& value = object[key];
		if (value.IsString()) return value.GetString();
		if (value.IsBool()) return value.GetBool() ? "1" : "0";
		if (value.IsNumber()) {
			char buffer[64];
			/* VS2013 has no snprintf; _snprintf on MSVC<2015, std::snprintf elsewhere */
#if defined(_MSC_VER) && _MSC_VER < 1900
			_snprintf(buffer, sizeof(buffer), "%g", value.GetDouble());
			buffer[sizeof(buffer) - 1] = '\0';
#else
			std::snprintf(buffer, sizeof(buffer), "%g", value.GetDouble());
#endif
			return buffer;
		}
		return fallback;
	}

	CCPoint jsonPoint(const flrapidjson::Value& value, const CCPoint& fallback) {
		if (!value.IsArray() || value.Size() < 2) return fallback;
		return ccp(
			jsonNumber(value[(flrapidjson::SizeType)0], fallback.x),
			jsonNumber(value[(flrapidjson::SizeType)1], fallback.y)
			);
	}

	CCPoint jsonPoint(const flrapidjson::Value& object, const char* key, const CCPoint& fallback) {
		if (!object.IsObject() || !object.HasMember(key)) return fallback;
		return jsonPoint(object[key], fallback);
	}

	CCPoint jsonPointFlexible(const flrapidjson::Value& value, const CCPoint& fallback) {
		if (!value.IsArray() || value.Empty()) return fallback;
		const float x = jsonNumber(value[(flrapidjson::SizeType)0], fallback.x);
		const float y = value.Size() >= 2
			? jsonNumber(value[(flrapidjson::SizeType)1], fallback.y)
			: 0.0f;
		return ccp(x, y);
	}

	CCPoint jsonPointFlexible(const flrapidjson::Value& object, const char* key, const CCPoint& fallback) {
		if (!object.IsObject() || !object.HasMember(key)) return fallback;
		return jsonPointFlexible(object[key], fallback);
	}

	bool parseLevelObject(const std::string& id, const flrapidjson::Value& json, bool background, LevelObject& output) {
		if (!json.IsObject()) return false;

		const std::string spriteSheet = jsonString(json, "spriteSheet");
		const std::string texture = jsonString(json, "texture");
		const std::string sheetType = jsonString(json, background ? "bgType" : "sheetType");
		if (spriteSheet.empty() || texture.empty() || sheetType.empty()) return false;

		output.id = id;
		output.objectType = jsonString(json, "type", background ? "background" : "block");
		FL_Block::Data& data = output.data;
		data.spriteSheet = spriteSheet;
		data.texture = texture;
		data.sheetType = sheetType;
		data.position = jsonPoint(json, "position", CCPointZero);
		data.scale = jsonPoint(json, "scale", ccp(1.0f, 1.0f));
		data.zValue = jsonInt(json, "zValue", 0);
		data.p_uID = jsonInt(json, "p_uID", 0);

		if (json.HasMember("rotation")) {
			data.rotation = jsonNumber(json, "rotation", 0.0f);
			data.hasRotation = true;
		}
		if (json.HasMember("opacity")) {
			data.opacity = jsonNumber(json, "opacity", 1.0f);
			data.hasOpacity = true;
		}
		if (json.HasMember("red") && json.HasMember("green") && json.HasMember("blue")) {
			data.red = jsonNumber(json, "red", 1.0f);
			data.green = jsonNumber(json, "green", 1.0f);
			data.blue = jsonNumber(json, "blue", 1.0f);
			data.hasColor = true;
		}

		const CCPoint flipped = jsonPoint(json, "flipped", CCPointZero);
		data.flippedX = flipped.x != 0.0f;
		data.flippedY = flipped.y != 0.0f;
		data.customAnim = jsonString(json, "customAnim");
		data.animated = jsonBool(json, "animated", false);
		data.skipStartAnim = jsonBool(json, "skipStartAnim", false);
		data.skipEndAnim = jsonBool(json, "skipEndAnim", false);
		data.darken = jsonBool(json, "darken", false);
		output.passable = jsonBool(json, "passable", false);
		output.blocksMovement = objectTypeUsesSpriteCollision(output.objectType);

		if (json.HasMember("points") && json["points"].IsObject()) {
			std::vector<std::pair<int, CCPoint> > orderedPoints;
			const flrapidjson::Value& points = json["points"];
			for (flrapidjson::Value::ConstMemberIterator it = points.MemberBegin(); it != points.MemberEnd(); ++it) {
				if (!it->value.IsObject() || !it->value.HasMember("position")) continue;
				int pointIndex = -1;
				if (std::sscanf(it->name.GetString(), "point_%d", &pointIndex) != 1 || pointIndex < 0) continue;
				orderedPoints.push_back(std::make_pair(
					pointIndex,
					jsonPoint(it->value, "position", CCPointZero)
					));
			}
			std::sort(orderedPoints.begin(), orderedPoints.end(),
				[](const std::pair<int, CCPoint>& left, const std::pair<int, CCPoint>& right) {
				return left.first < right.first;
			});
			for (std::vector<std::pair<int, CCPoint> >::const_iterator point = orderedPoints.begin();
				point != orderedPoints.end(); ++point) {
				output.collisionPoints.push_back(point->second);
			}
		}
		return true;
	}

	bool parseCameraTrigger(
		const std::string& id,
		const flrapidjson::Value& json,
		CameraTriggerData& output
		) {
		if (!json.IsObject() || toLower(jsonString(json, "Type")) != "cameratrigger") return false;

		output.id = id;
		output.pUID = jsonInt(json, "p_uID", 0);
		output.calledByEvent = jsonBool(json, "calledByEvent", false);
		output.cameraLockType = toLower(jsonScalarString(json, "cameraLockType", "full"));
		output.lockCamera = jsonBool(json, "lockCamera", false);
		output.instantMove = jsonBool(json, "instantMove", false);
		output.instantZoom = jsonBool(json, "instantZoom", false);
		output.offsetDuration = std::max(0.0f, jsonNumber(json, "offsetDuration", 0.0f));
		output.zoomDuration = std::max(0.0f, jsonNumber(json, "zoomDuration", 0.0f));
		output.triggerDelay = std::max(0.0f, jsonNumber(json, "triggerDelay", 0.0f));
		output.offsetEasingType = jsonScalarString(json, "offsetEasingType", "InOut");
		output.zoomEasingType = jsonScalarString(json, "zoomEasingType", "InOut");

		output.hasCameraOffset = json.HasMember("cameraOffset") &&
			json["cameraOffset"].IsArray() && !json["cameraOffset"].Empty();
		output.cameraOffset = jsonPointFlexible(json, "cameraOffset", CCPointZero);

		output.hasTargetPosition = json.HasMember("targetPosition") &&
			json["targetPosition"].IsArray() && json["targetPosition"].Size() >= 2;
		output.targetPosition = jsonPointFlexible(json, "targetPosition", CCPointZero);

		output.hasZoomTarget = json.HasMember("zoomTarget");
		output.zoomTarget = jsonNumber(json, "zoomTarget", 0.0f);

		const CCPoint position = jsonPoint(json, "position", CCPointZero);
		const CCPoint scale = jsonPoint(json, "scale", ccp(1.0f, 1.0f));

		// Camera triggers in the editor use a 100x100 logical trigger tile. The
		// serialized scale stretches this tile into the actual activation region.
		const float width = 100.0f * std::max(0.01f, std::fabs(scale.x));
		const float height = 100.0f * std::max(0.01f, std::fabs(scale.y));
		output.bounds = CCRect(
			position.x - width * 0.5f,
			position.y - height * 0.5f,
			width,
			height
			);
		return true;
	}

	std::vector<CCPoint> transformedBlockCorners(FL_Block* block) {
		std::vector<CCPoint> corners;
		if (!block) return corners;

		const CCSize size = block->getContentSize();
		const CCAffineTransform transform = block->nodeToParentTransform();
		const CCPoint localCorners[] = {
			ccp(0.0f, 0.0f),
			ccp(size.width, 0.0f),
			ccp(size.width, size.height),
			ccp(0.0f, size.height)
		};
		for (unsigned int index = 0; index < sizeof(localCorners) / sizeof(localCorners[0]); ++index) {
			corners.push_back(CCPointApplyAffineTransform(localCorners[index], transform));
		}
		return corners;
	}

	bool readJsonFile(const std::string& fileName, std::string& contents) {
		unsigned long size = 0;
		unsigned char* buffer = CCFileUtils::sharedFileUtils()->getFileData(fileName.c_str(), "rb", &size);
		/* VS2013 debug runtime fills uninitialized pointers with 0xCCCCCCCC.
		Guard against that so a missing file returns false instead of crashing. */
#if defined(_MSC_VER) && _MSC_VER < 1900
		if (!buffer || size == 0 || reinterpret_cast<uintptr_t>(buffer) == 0xCCCCCCCCu) {
			if (buffer && reinterpret_cast<uintptr_t>(buffer) != 0xCCCCCCCCu) delete[] buffer;
			return false;
		}
#else
		if (!buffer || size == 0) {
			if (buffer) delete[] buffer;
			return false;
		}
#endif
		contents.assign(reinterpret_cast<const char*>(buffer), size);
		delete[] buffer;
		return true;
	}

	bool loadLevelData(LevelStruct& level, const std::string& requestedFile) {
		std::string actualFile = requestedFile;
		if (endsWithIgnoreCase(actualFile, ".plist")) {
			actualFile = replaceExtension(actualFile, ".json");
		}

		std::string jsonText;
		if (!readJsonFile(actualFile, jsonText)) {
			CCLog("Could not load level resource %s (requested as %s).", actualFile.c_str(), requestedFile.c_str());
			return false;
		}

		const char* text = jsonText.c_str();
		if (jsonText.size() >= 3 &&
			static_cast<unsigned char>(text[0]) == 0xEF &&
			static_cast<unsigned char>(text[1]) == 0xBB &&
			static_cast<unsigned char>(text[2]) == 0xBF) {
			text += 3;
		}

		flrapidjson::Document document;
		document.Parse<0>(text);
		if (document.HasParseError() || !document.IsObject()) {
			CCLog("Error parsing %s: code %d at offset %lu.", actualFile.c_str(),
				document.GetParseError(), static_cast<unsigned long>(document.GetErrorOffset()));
			return false;
		}

		level.playerSpawn = jsonPoint(document, "playerSpawn", level.playerSpawn);
		const CCPoint levelSize = jsonPoint(document, "levelSize", ccp(level.levelSize.width, level.levelSize.height));
		level.levelSize = CCSizeMake(std::max(1.0f, levelSize.x), std::max(1.0f, levelSize.y));

		if (document.HasMember("settings") && document["settings"].IsObject()) {
			const flrapidjson::Value& settings = document["settings"];
			level.backgroundImage = jsonString(settings, "bgImage");
			level.backgroundMusic = jsonString(settings, "bgMusic");
			level.backgroundOffsetY = jsonNumber(settings, "offsetBGY", 0.0f);
		}

		if (document.HasMember("sheetContainer") && document["sheetContainer"].IsObject()) {
			const flrapidjson::Value& sheets = document["sheetContainer"];
			for (flrapidjson::Value::ConstMemberIterator it = sheets.MemberBegin(); it != sheets.MemberEnd(); ++it) {
				if (!it->value.IsObject()) continue;
				LevelStruct::SheetData sheet;
				sheet.texture = jsonString(it->value, "texture");
				sheet.type = jsonString(it->value, "type");
				if (!sheet.texture.empty()) level.sheets.insert(sheet);
			}
		}

		const char* containerNames[] = { "blockContainer", "bgContainer" };
		for (int containerIndex = 0; containerIndex < 2; ++containerIndex) {
			const char* containerName = containerNames[containerIndex];
			if (!document.HasMember(containerName) || !document[containerName].IsObject()) continue;
			const bool background = containerIndex == 1;
			const flrapidjson::Value& container = document[containerName];
			for (flrapidjson::Value::ConstMemberIterator it = container.MemberBegin(); it != container.MemberEnd(); ++it) {
				LevelObject object;
				if (!parseLevelObject(it->name.GetString(), it->value, background, object)) continue;
				if (background) level.backgrounds.push_back(object);
				else {
					level.blocks.push_back(object);
					if (object.collisionPoints.size() >= 2) {
						level.collisionWorld.addShape(object.collisionPoints, object.passable);
					}
				}

				LevelStruct::SheetData sheet;
				sheet.texture = object.data.spriteSheet;
				sheet.type = object.data.sheetType;
				level.sheets.insert(sheet);
			}
		}

		if (document.HasMember("npcContainer") && document["npcContainer"].IsObject()) {
			const flrapidjson::Value& container = document["npcContainer"];
			for (flrapidjson::Value::ConstMemberIterator it = container.MemberBegin(); it != container.MemberEnd(); ++it) {
				if (!it->value.IsObject()) continue;
				NpcSpawn spawn;
				spawn.id = it->name.GetString();
				spawn.npcType = jsonString(it->value, "npcType");
				spawn.position = jsonPoint(it->value, "position", CCPointZero);
				spawn.scale = jsonPoint(it->value, "scale", ccp(1.0f, 1.0f));
				const CCPoint flipped = jsonPoint(it->value, "flipped", CCPointZero);
				spawn.flippedX = flipped.x != 0.0f;
				spawn.flippedY = flipped.y != 0.0f;
				spawn.rotation = jsonNumber(it->value, "rotation", 0.0f);
				spawn.layerType = jsonString(it->value, "customZLayer", "NPC");
				if (spawn.layerType.empty()) spawn.layerType = "NPC";
				spawn.zOrder = jsonInt(it->value, "p_uID", static_cast<int>(level.npcSpawns.size()));
				if (!spawn.npcType.empty()) level.npcSpawns.push_back(spawn);
			}
		}

		if (document.HasMember("triggerContainer") && document["triggerContainer"].IsObject()) {
			const flrapidjson::Value& container = document["triggerContainer"];
			for (flrapidjson::Value::ConstMemberIterator it = container.MemberBegin(); it != container.MemberEnd(); ++it) {
				CameraTriggerData trigger;
				if (!parseCameraTrigger(it->name.GetString(), it->value, trigger)) continue;
				// Event-driven triggers require the original event/group system and
				// must not activate merely because the player overlaps their editor
				// marker. Spatial camera triggers are handled directly here.
				if (!trigger.calledByEvent) level.cameraTriggers.push_back(trigger);
			}
			std::sort(level.cameraTriggers.begin(), level.cameraTriggers.end(),
				[](const CameraTriggerData& left, const CameraTriggerData& right) {
				if (left.pUID != right.pUID) return left.pUID < right.pUID;
				return left.id < right.id;
			});
		}

		CCLog("Loaded %s through %s: %u blocks, %u backgrounds, %u NPC spawns, %u camera triggers, %u solid collision shapes, %u one-way surfaces.",
			requestedFile.c_str(), actualFile.c_str(),
			static_cast<unsigned int>(level.blocks.size()),
			static_cast<unsigned int>(level.backgrounds.size()),
			static_cast<unsigned int>(level.npcSpawns.size()),
			static_cast<unsigned int>(level.cameraTriggers.size()),
			static_cast<unsigned int>(level.collisionWorld.solidShapeCount()),
			static_cast<unsigned int>(level.collisionWorld.oneWaySegmentCount()));
		return true;
	}

	int getZValueForType(const std::string& type) {
		int value = 0;
		if (type == "F1") value = 160;
		else if (type == "F2") value = 150;
		else if (type == "F3") value = 140;
		else if (type == "F4") value = 130;
		else if (type == "P1") value = 120;
		else if (type == "NPC") value = 90;
		else if (type == "B1") value = 50;
		else if (type == "B2") value = 40;
		else if (type == "B3") value = 30;
		else if (type == "B4") value = 20;
		else if (type == "mg") value = 10;
		else if (type == "BG") value = 5;
		else if (type == "fg") value = 170;

		if (type.find('+') != std::string::npos) ++value;
		else if (type.find('-') != std::string::npos) --value;
		return value;
	}

	bool loadAtlas(const std::string& sheetName, std::set<std::string>& loadedAtlases) {
		if (sheetName.empty() || sheetName == "particle" || sheetName == "light") return false;
		if (loadedAtlases.find(sheetName) != loadedAtlases.end()) return true;

		std::vector<std::string> candidates;
		if (isHDSheetName(sheetName)) candidates.push_back(sheetName + "-hd.plist");
		candidates.push_back(sheetName + ".plist");

		for (std::vector<std::string>::const_iterator it = candidates.begin(); it != candidates.end(); ++it) {
			if (!fileExists(*it)) continue;
			CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(it->c_str());
			loadedAtlases.insert(sheetName);
			CCLog("Loaded atlas %s for sheet %s.", it->c_str(), sheetName.c_str());
			return true;
		}

		CCLog("Atlas not found for sheet %s.", sheetName.c_str());
		return false;
	}

	float objectToFloat(CCObject* object, float fallback) {
		CCString* value = dynamic_cast<CCString*>(object);
		return value ? value->floatValue() : fallback;
	}

	std::string dictionaryString(CCDictionary* dictionary, const char* key, const std::string& fallback = std::string()) {
		if (!dictionary) return fallback;
		CCString* value = dynamic_cast<CCString*>(dictionary->objectForKey(key));
		return value ? value->getCString() : fallback;
	}

	float dictionaryFloat(CCDictionary* dictionary, const char* key, float fallback) {
		if (!dictionary) return fallback;
		return objectToFloat(dictionary->objectForKey(key), fallback);
	}

	bool dictionaryBool(CCDictionary* dictionary, const char* key, bool fallback) {
		const std::string raw = toLower(dictionaryString(dictionary, key));
		if (raw.empty()) return fallback;
		if (raw == "yes" || raw == "true" || raw == "1") return true;
		if (raw == "no" || raw == "false" || raw == "0") return false;
		return fallback;
	}

	CCPoint parsePointString(const std::string& value, const CCPoint& fallback) {
		std::vector<float> values;
		const char* cursor = value.c_str();
		while (*cursor && values.size() < 2) {
			while (*cursor && !std::isdigit(static_cast<unsigned char>(*cursor)) && *cursor != '-' && *cursor != '+') ++cursor;
			if (!*cursor) break;
			char* numberEnd = NULL;
			const double number = std::strtod(cursor, &numberEnd);
			if (!numberEnd || numberEnd == cursor) break;
			values.push_back(static_cast<float>(number));
			cursor = numberEnd;
		}
		return values.size() >= 2 ? ccp(values[0], values[1]) : fallback;
	}

	CCDictionary* dictionaryForKey(CCDictionary* dictionary, const char* key) {
		return dictionary ? dynamic_cast<CCDictionary*>(dictionary->objectForKey(key)) : NULL;
	}

	CCObject* findCaseInsensitive(CCDictionary* dictionary, const std::string& requestedName, std::string* actualName = NULL) {
		if (!dictionary || requestedName.empty()) return NULL;
		CCObject* exact = dictionary->objectForKey(requestedName);
		if (exact) {
			if (actualName) *actualName = requestedName;
			return exact;
		}

		const std::string target = toLower(requestedName);
		CCDictElement* element = NULL;
		CCDICT_FOREACH(dictionary, element) {
			if (toLower(element->getStrKey()) == target) {
				if (actualName) *actualName = element->getStrKey();
				return element->getObject();
			}
		}
		return NULL;
	}

	std::vector<int> parseFrameNumbers(const std::string& text) {
		std::vector<int> numbers;
		const bool hasSeparator = text.find(' ') != std::string::npos || text.find(',') != std::string::npos;
		const char* cursor = text.c_str();
		while (*cursor) {
			while (*cursor && !std::isdigit(static_cast<unsigned char>(*cursor))) ++cursor;
			if (!*cursor) break;
			char* end = NULL;
			const long number = std::strtol(cursor, &end, 10);
			if (!end || end == cursor) break;
			numbers.push_back(static_cast<int>(number));
			cursor = end;
		}

		if (numbers.size() == 1 && !hasSeparator) {
			const int count = std::max(1, numbers[0]);
			numbers.clear();
			for (int index = 1; index <= count; ++index) numbers.push_back(index);
		}
		if (numbers.empty()) numbers.push_back(1);
		return numbers;
	}

	std::string stripFrameNumber(const std::string& texture) {
		std::string base = texture;
		if (endsWithIgnoreCase(base, ".png")) base.resize(base.size() - 4);
		if (base.size() > 4 && base[base.size() - 4] == '_') {
			const std::string suffix = base.substr(base.size() - 3);
			if (std::isdigit(static_cast<unsigned char>(suffix[0])) &&
				std::isdigit(static_cast<unsigned char>(suffix[1])) &&
				std::isdigit(static_cast<unsigned char>(suffix[2]))) {
				base.resize(base.size() - 4);
			}
		}
		return base;
	}

	std::string formatFrameName(const std::string& base, int number) {
		char buffer[32];
		std::sprintf(buffer, "_%03d.png", number);
		return base + buffer;
	}

	std::shared_ptr<EntityDescriptor> buildEntityDescriptor(
		const std::string& npcType,
		std::set<std::string>& loadedAtlases
		) {
		CCDictionary* definitions = CCDictionary::createWithContentsOfFile("enemyDefinitions.plist");
		if (!definitions) {
			CCLog("enemyDefinitions.plist could not be loaded.");
			return std::shared_ptr<EntityDescriptor>();
		}

		CCDictionary* definition = dynamic_cast<CCDictionary*>(findCaseInsensitive(definitions, npcType));
		if (!definition) {
			CCLog("No enemy definition for %s.", npcType.c_str());
			return std::shared_ptr<EntityDescriptor>();
		}

		std::shared_ptr<EntityDescriptor> descriptor(new EntityDescriptor());
		descriptor->npcType = npcType;
		descriptor->sheetName = dictionaryString(definition, "spriteSheet");
		descriptor->displayScale = isHDSheetName(descriptor->sheetName) ? 0.5f : 1.0f;
		descriptor->boundsSize = CCSizeMake(
			std::max(32.0f, dictionaryFloat(definition, "width", 128.0f)),
			std::max(32.0f, dictionaryFloat(definition, "height", 128.0f))
			);
		descriptor->contactDamage = std::max(0, static_cast<int>(
			dictionaryFloat(
				definition,
				"damagePoints",
				dictionaryFloat(definition, "damage", 0.0f)
			)
		));

		if (!loadAtlas(descriptor->sheetName, loadedAtlases)) return std::shared_ptr<EntityDescriptor>();

		CCDictionary* animations = dictionaryForKey(definition, "animations");
		const char* preferredAnimations[] = { "walk", "fly", "run", "idle", "chase" };
		std::string animationName;
		CCDictionary* animation = NULL;
		for (unsigned int index = 0; index < sizeof(preferredAnimations) / sizeof(preferredAnimations[0]); ++index) {
			animation = dynamic_cast<CCDictionary*>(findCaseInsensitive(animations, preferredAnimations[index], &animationName));
			if (animation) break;
		}
		if (!animation && animations) {
			CCDictElement* first = NULL;
			CCDICT_FOREACH(animations, first) {
				animation = dynamic_cast<CCDictionary*>(first->getObject());
				animationName = first->getStrKey();
				if (animation) break;
			}
		}

		const std::string definitionTexture = dictionaryString(definition, "texture");
		descriptor->frameDelay = std::max(0.016f, dictionaryFloat(animation, "delay", 0.1f));
		const bool usesParts = dictionaryBool(animation, "usesParts", false);
		const std::string singleFrame = dictionaryString(animation, "singleFrame");
		const std::string explicitBase = dictionaryString(animation, "animFrameName");
		const std::vector<int> frameNumbers = parseFrameNumbers(dictionaryString(animation, "frames", "1"));

		std::vector<std::string> requestedNames;
		if (!singleFrame.empty()) {
			requestedNames.push_back(singleFrame);
		}
		else {
			const std::string base = !explicitBase.empty()
				? explicitBase
				: (!definitionTexture.empty() ? stripFrameNumber(definitionTexture) : npcType + "_" + animationName);
			for (std::vector<int>::const_iterator it = frameNumbers.begin(); it != frameNumbers.end(); ++it) {
				requestedNames.push_back(formatFrameName(base, *it));
			}
		}

		if (usesParts) {
			const std::string animationFile = dictionaryString(definition, "animDesc");
			CCDictionary* animationDescription = animationFile.empty()
				? NULL
				: CCDictionary::createWithContentsOfFile(animationFile.c_str());
			CCDictionary* frameContainer = dictionaryForKey(animationDescription, "animationContainer");

			if (requestedNames.size() == 1 && !findCaseInsensitive(frameContainer, requestedNames[0]) && !definitionTexture.empty()) {
				requestedNames[0] = definitionTexture;
			}

			for (std::vector<std::string>::const_iterator frameName = requestedNames.begin(); frameName != requestedNames.end(); ++frameName) {
				CCDictionary* frameDictionary = dynamic_cast<CCDictionary*>(findCaseInsensitive(frameContainer, *frameName));
				if (!frameDictionary) continue;

				EntityFrame frame;
				CCDictElement* partElement = NULL;
				CCDICT_FOREACH(frameDictionary, partElement) {
					CCDictionary* partDictionary = dynamic_cast<CCDictionary*>(partElement->getObject());
					if (!partDictionary) continue;
					EntityPart part;
					part.texture = dictionaryString(partDictionary, "texture");
					if (part.texture.empty() || part.texture == "0") continue;
					part.position = parsePointString(dictionaryString(partDictionary, "position"), CCPointZero);
					part.scale = parsePointString(dictionaryString(partDictionary, "scale"), ccp(1.0f, 1.0f));
					part.scale = ccpMult(part.scale, descriptor->displayScale);
					const CCPoint flipped = parsePointString(dictionaryString(partDictionary, "flipped"), CCPointZero);
					part.flippedX = flipped.x != 0.0f;
					part.flippedY = flipped.y != 0.0f;
					part.rotation = dictionaryFloat(partDictionary, "rotation", 0.0f);
					part.zOrder = static_cast<int>(dictionaryFloat(partDictionary, "zValue", 0.0f));
					if (CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(part.texture.c_str())) {
						frame.parts.push_back(part);
					}
				}
				std::sort(frame.parts.begin(), frame.parts.end(), [](const EntityPart& left, const EntityPart& right) {
					return left.zOrder < right.zOrder;
				});
				if (!frame.parts.empty()) descriptor->frames.push_back(frame);
			}
		}
		else {
			for (std::vector<std::string>::const_iterator frameName = requestedNames.begin(); frameName != requestedNames.end(); ++frameName) {
				if (!CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName->c_str())) continue;
				EntityFrame frame;
				EntityPart part;
				part.texture = *frameName;
				part.scale = ccp(descriptor->displayScale, descriptor->displayScale);
				frame.parts.push_back(part);
				descriptor->frames.push_back(frame);
			}
		}

		if (descriptor->frames.empty() && !definitionTexture.empty() &&
			CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(definitionTexture.c_str())) {
			EntityFrame frame;
			EntityPart part;
			part.texture = definitionTexture;
			part.scale = ccp(descriptor->displayScale, descriptor->displayScale);
			frame.parts.push_back(part);
			descriptor->frames.push_back(frame);
		}

		if (descriptor->frames.empty()) {
			CCLog("No visual frames resolved for entity %s.", npcType.c_str());
			return std::shared_ptr<EntityDescriptor>();
		}

		CCLog("Entity %s uses %u frames from %s.", npcType.c_str(),
			static_cast<unsigned int>(descriptor->frames.size()), descriptor->sheetName.c_str());
		return descriptor;
	}

	bool rectsIntersect(const CCRect& left, const CCRect& right) {
		return left.getMaxX() >= right.getMinX() && left.getMinX() <= right.getMaxX() &&
			left.getMaxY() >= right.getMinY() && left.getMinY() <= right.getMaxY();
	}

	CCRect expandedRect(const CCRect& rect, float amount) {
		return CCRect(
			rect.origin.x - amount,
			rect.origin.y - amount,
			rect.size.width + amount * 2.0f,
			rect.size.height + amount * 2.0f
			);
	}

	bool pointInsideRect(const CCRect& rect, const CCPoint& point) {
		return point.x >= rect.getMinX() && point.x <= rect.getMaxX() &&
			point.y >= rect.getMinY() && point.y <= rect.getMaxY();
	}

	CCPoint cameraCenterWithScreenOffset(
		const CCPoint& basePosition,
		const CCPoint& screenOffset,
		float zoom
		) {
		// cameraOffset in level files is authored in screen points. Converting it
		// back to world units keeps the requested composition stable while zooming.
		const float safeZoom = std::max(0.01f, zoom);
		return ccp(
			basePosition.x + screenOffset.x / safeZoom,
			basePosition.y + screenOffset.y / safeZoom
			);
	}

	float cameraEaseProgress(float progress, const std::string& easingType) {
		const float t = std::max(0.0f, std::min(1.0f, progress));
		const std::string easing = toLower(easingType);
		if (easing == "in" || easing == "easein") return t * t;
		if (easing == "out" || easing == "easeout") {
			const float inverse = 1.0f - t;
			return 1.0f - inverse * inverse;
		}
		if (easing == "inout" || easing == "easeinout") {
			// Smoothstep is stable at both ends and matches the level editor's
			// generic InOut camera easing closely enough without action objects.
			return t * t * (3.0f - 2.0f * t);
		}
		return t;
	}

	float cameraZoomFromLevelValue(float defaultZoom, float zoomTarget) {
		if (zoomTarget <= 0.0f) return defaultZoom;

		// The exported value is a camera-distance setting, not a direct node scale:
		// 50 is the normal gameplay distance, smaller values move closer and larger
		// values move farther away. Keep the conversion continuous and bounded.
		const float converted = defaultZoom * 200.0f / (150.0f + zoomTarget);
		return std::max(0.45f, std::min(2.5f, converted));
	}

	CCPoint clampCameraCenter(
		const CCPoint& requested,
		const CCSize& levelSize,
		const CCSize& windowSize,
		float zoom
		) {
		const float safeZoom = std::max(0.01f, zoom);
		const float halfWidth = windowSize.width * 0.5f / safeZoom;
		const float halfHeight = windowSize.height * 0.5f / safeZoom;
		CCPoint result = requested;

		if (levelSize.width > halfWidth * 2.0f) {
			result.x = std::max(halfWidth, std::min(levelSize.width - halfWidth, result.x));
		}
		else {
			result.x = levelSize.width * 0.5f;
		}

		if (levelSize.height > halfHeight * 2.0f) {
			result.y = std::max(halfHeight, std::min(levelSize.height - halfHeight, result.y));
		}
		else {
			result.y = levelSize.height * 0.5f;
		}
		return result;
	}

	void readGameplayKeys(bool& left, bool& right, bool& jump) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		left = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
		right = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
		jump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
		left = glfwGetKey(GLFW_KEY_LEFT) == GLFW_PRESS;
		right = glfwGetKey(GLFW_KEY_RIGHT) == GLFW_PRESS;
		jump = glfwGetKey(GLFW_KEY_SPACE) == GLFW_PRESS;
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
		left = CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, 123);
		right = CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, 124);
		jump = CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, 49);
#else
		left = right = jump = false;
#endif
	}

} // namespace

struct FL_PlayLayer::Members {
	FL_PlayLayer::Args sceneArgs;
	CCPoint cameraPos;
	CCPoint spawnPos;
	CCSize levelSize;
	float zoom;
	float defaultZoom;
	bool previewMode;

	CCPoint defaultCameraOffset;
	CCPoint cameraOffset;
	CCPoint cameraOffsetStart;
	CCPoint cameraOffsetTarget;
	float cameraOffsetElapsed;
	float cameraOffsetDuration;
	std::string cameraOffsetEasing;

	float zoomStart;
	float zoomTarget;
	float zoomElapsed;
	float zoomDuration;
	std::string zoomEasing;

	bool cameraLocked;
	CCPoint cameraLockPosition;
	bool snapCameraPosition;
	std::vector<CameraTriggerData> cameraTriggers;
	std::string activeCameraTriggerId;
	float activeCameraTriggerTime;
	bool activeCameraTriggerFired;

	bool up;
	bool down;
	bool left;
	bool right;
	bool jumpInputWasDown;
	bool pausedByUI;
	bool soundEnabled;
	FL_Player* player;

	CCPoint lastTouchPos;
	bool isDragging;
	float cullingTimer;

	CCSprite* fixedBackground;
	float backgroundOffsetY;

	std::set<std::string> loadedAtlases;
	std::vector<RuntimeNode> runtimeNodes;
	std::vector<EntityRuntime> entities;
	std::map<std::string, std::shared_ptr<EntityDescriptor> > entityDescriptors;

	Members()
		: cameraPos(CCPointZero)
		, spawnPos(CCPointZero)
		, levelSize(CCSizeMake(512.0f, 399.0f))
		, zoom(1.3f)
		, defaultZoom(1.3f)
		, previewMode(false)
		, defaultCameraOffset(CCPointZero)
		, cameraOffset(CCPointZero)
		, cameraOffsetStart(CCPointZero)
		, cameraOffsetTarget(CCPointZero)
		, cameraOffsetElapsed(0.0f)
		, cameraOffsetDuration(0.0f)
		, zoomStart(1.3f)
		, zoomTarget(1.3f)
		, zoomElapsed(0.0f)
		, zoomDuration(0.0f)
		, cameraLocked(false)
		, cameraLockPosition(CCPointZero)
		, snapCameraPosition(false)
		, activeCameraTriggerTime(0.0f)
		, activeCameraTriggerFired(false)
		, up(false)
		, down(false)
		, left(false)
		, right(false)
		, jumpInputWasDown(false)
		, pausedByUI(false)
		, soundEnabled(true)
		, player(NULL)
		, lastTouchPos(CCPointZero)
		, isDragging(false)
		, cullingTimer(0.0f)
		, fixedBackground(NULL)
		, backgroundOffsetY(0.0f) {
	}

	void beginOffsetTransition(const CCPoint& target, float duration, const std::string& easing) {
		cameraOffsetStart = cameraOffset;
		cameraOffsetTarget = target;
		cameraOffsetElapsed = 0.0f;
		cameraOffsetDuration = std::max(0.0f, duration);
		cameraOffsetEasing = easing;
		if (cameraOffsetDuration <= 0.0f) cameraOffset = cameraOffsetTarget;
	}

	void beginZoomTransition(float target, float duration, const std::string& easing) {
		zoomStart = zoom;
		zoomTarget = target;
		zoomElapsed = 0.0f;
		zoomDuration = std::max(0.0f, duration);
		zoomEasing = easing;
		if (zoomDuration <= 0.0f) zoom = zoomTarget;
	}

	void applyCameraTrigger(const CameraTriggerData& trigger, const CCPoint& playerPosition) {
		const bool resetCamera = trigger.cameraLockType.empty() ||
			trigger.cameraLockType == "0" || trigger.cameraLockType == "none";

		CCPoint requestedOffset = defaultCameraOffset;
		float requestedZoom = defaultZoom;
		bool requestedLock = false;
		CCPoint requestedLockPosition = playerPosition;

		if (!resetCamera) {
			requestedOffset = trigger.hasCameraOffset ? trigger.cameraOffset : defaultCameraOffset;
			requestedZoom = trigger.hasZoomTarget
				? cameraZoomFromLevelValue(defaultZoom, trigger.zoomTarget)
				: zoomTarget;
			requestedLock = trigger.lockCamera;
			requestedLockPosition = trigger.hasTargetPosition
				? trigger.targetPosition
				: playerPosition;
		}

		const float moveDuration = trigger.instantMove
			? 0.0f
			: (trigger.offsetDuration > 0.0f ? trigger.offsetDuration : 0.28f);
		const float targetZoomDuration = trigger.instantZoom
			? 0.0f
			: (trigger.zoomDuration > 0.0f ? trigger.zoomDuration : 0.34f);

		beginOffsetTransition(requestedOffset, moveDuration, trigger.offsetEasingType);
		beginZoomTransition(requestedZoom, targetZoomDuration, trigger.zoomEasingType);
		cameraLocked = requestedLock;
		cameraLockPosition = requestedLockPosition;

		if (trigger.instantMove) {
			cameraOffset = cameraOffsetTarget;
			cameraPos = cameraCenterWithScreenOffset(
				cameraLocked ? cameraLockPosition : playerPosition,
				cameraOffset,
				zoom
				);
			snapCameraPosition = true;
		}

		CCLog(
			"Camera trigger %s: offset=(%.1f, %.1f), zoom=%.3f, lock=%d.",
			trigger.id.c_str(), requestedOffset.x, requestedOffset.y,
			requestedZoom, requestedLock ? 1 : 0
			);
	}

	void updateCameraTransitions(float dt) {
		if (cameraOffsetDuration > 0.0f) {
			cameraOffsetElapsed += std::max(0.0f, dt);
			const float progress = cameraEaseProgress(
				cameraOffsetElapsed / cameraOffsetDuration,
				cameraOffsetEasing
				);
			cameraOffset = ccpAdd(
				cameraOffsetStart,
				ccpMult(ccpSub(cameraOffsetTarget, cameraOffsetStart), progress)
				);
			if (cameraOffsetElapsed >= cameraOffsetDuration) {
				cameraOffset = cameraOffsetTarget;
				cameraOffsetDuration = 0.0f;
			}
		}

		if (zoomDuration > 0.0f) {
			zoomElapsed += std::max(0.0f, dt);
			const float progress = cameraEaseProgress(zoomElapsed / zoomDuration, zoomEasing);
			zoom = zoomStart + (zoomTarget - zoomStart) * progress;
			if (zoomElapsed >= zoomDuration) {
				zoom = zoomTarget;
				zoomDuration = 0.0f;
			}
		}
	}
};

namespace {

	void applyEntityFrame(EntityRuntime& runtime, unsigned int frameIndex) {
		if (!runtime.root || !runtime.descriptor || runtime.descriptor->frames.empty()) return;
		frameIndex %= runtime.descriptor->frames.size();
		const EntityFrame& frame = runtime.descriptor->frames[frameIndex];

		for (unsigned int index = 0; index < frame.parts.size(); ++index) {
			const EntityPart& part = frame.parts[index];
			CCSpriteFrame* spriteFrame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(part.texture.c_str());
			if (!spriteFrame) continue;

			CCSprite* sprite = NULL;
			if (index < runtime.sprites.size()) {
				sprite = runtime.sprites[index];
				sprite->setDisplayFrame(spriteFrame);
			}
			else {
				sprite = CCSprite::createWithSpriteFrame(spriteFrame);
				if (!sprite) continue;
				runtime.root->addChild(sprite, part.zOrder);
				runtime.sprites.push_back(sprite);
			}

			sprite->setVisible(true);
			sprite->setPosition(part.position);
			sprite->setScaleX(part.scale.x * (part.flippedX ? -1.0f : 1.0f));
			sprite->setScaleY(part.scale.y * (part.flippedY ? -1.0f : 1.0f));
			sprite->setRotation(part.rotation);
			runtime.root->reorderChild(sprite, part.zOrder);
		}

		for (unsigned int index = frame.parts.size(); index < runtime.sprites.size(); ++index) {
			runtime.sprites[index]->setVisible(false);
		}
		runtime.frameIndex = frameIndex;
	}

} // namespace

CCScene* FL_PlayLayer::scene(const Args& args) {
	CCScene* scene = CCScene::create();
	FL_PlayLayer* playLayer = FL_PlayLayer::create(args);
	FL_UILayer* uiLayer = FL_UILayer::create();

	if (!scene || !playLayer || !uiLayer) return NULL;
	uiLayer->setDelegate(playLayer);
	scene->addChild(playLayer, 0);
	playLayer->attachFixedBackground(scene, -1000000);
	scene->addChild(uiLayer, 10000000);
	return scene;
}

FL_PlayLayer* FL_PlayLayer::create(const Args& args) {
	FL_PlayLayer* result = new FL_PlayLayer();
	if (result && result->init(args)) {
		result->autorelease();
		return result;
	}
	delete result;
	return NULL;
}

FL_PlayLayer::FL_PlayLayer() : m(new Members()) {
}

FL_PlayLayer::~FL_PlayLayer() {
	delete m;
}

bool FL_PlayLayer::init(const Args& args) {
	if (!CCLayer::init()) return false;

	m->sceneArgs = args;

	// CCLayer defaults to a centered anchor. Camera math below is origin-based;
	// leaving the default anchor would add an extra zoom-dependent shift and put
	// the player left of center whenever a camera trigger changes the scale.
	setAnchorPoint(CCPointZero);
	ignoreAnchorPointForPosition(false);

	m->previewMode = args.previewMode;
	m->zoom = std::max(0.1f, args.initialZoom);
	m->defaultZoom = m->zoom;
	m->zoomStart = m->zoom;
	m->zoomTarget = m->zoom;
	m->defaultCameraOffset = args.cameraOffset;
	m->cameraOffset = args.cameraOffset;
	m->cameraOffsetStart = args.cameraOffset;
	m->cameraOffsetTarget = args.cameraOffset;

	// Gameplay camera follows the player; world dragging is intentionally disabled.
	// Do not register with CCScheduler until onEnter(). init() can still fail while
	// parsing a level or constructing resources; scheduling here would leave a
	// dangling scheduler target when create() deletes the partially initialized node.
	setTouchEnabled(false);
	setKeypadEnabled(!m->previewMode);

	LevelStruct level;
	if (!loadLevelData(level, args.levelFile)) return false;

	m->spawnPos = level.playerSpawn;
	m->cameraPos = cameraCenterWithScreenOffset(
		level.playerSpawn,
		args.cameraOffset,
		m->zoom
		);
	m->lastTouchPos = m->cameraPos;
	m->levelSize = level.levelSize;
	m->backgroundOffsetY = level.backgroundOffsetY;
	m->cameraTriggers = level.cameraTriggers;

	for (std::set<LevelStruct::SheetData>::const_iterator it = level.sheets.begin(); it != level.sheets.end(); ++it) {
		loadAtlas(it->texture, m->loadedAtlases);
	}

	if (!m->previewMode && !level.backgroundMusic.empty() && level.backgroundMusic != "0" && fileExists(level.backgroundMusic)) {
		SimpleAudioEngine::sharedEngine()->playBackgroundMusic(level.backgroundMusic.c_str(), true);
	}

	if (!level.backgroundImage.empty()) {
		const std::string imageFile = level.backgroundImage + ".png";
		if (fileExists(imageFile)) {
			m->fixedBackground = CCSprite::create(imageFile.c_str());
			if (m->fixedBackground) {
				m->fixedBackground->setAnchorPoint(ccp(0.5f, 0.5f));
				addChild(m->fixedBackground, -2000000);
			}
		}
	}

	const std::vector<LevelObject>* containers[] = { &level.backgrounds, &level.blocks };
	for (int containerIndex = 0; containerIndex < 2; ++containerIndex) {
		const std::vector<LevelObject>& objects = *containers[containerIndex];
		for (std::vector<LevelObject>::const_iterator it = objects.begin(); it != objects.end(); ++it) {
			const LevelObject& object = *it;
			if (toLower(object.objectType) == "particle" ||
				object.data.spriteSheet == "particle" ||
				object.data.texture.empty() ||
				object.data.texture == "NULL" ||
				object.data.texture == "BlockSectionReference.png" ||
				endsWithIgnoreCase(object.data.texture, ".plist")) {
				continue;
			}

			FL_Block* block = FL_Block::create(object.data);
			if (!block) {
				CCLog("Object %s skipped: frame %s is unresolved.", object.id.c_str(), object.data.texture.c_str());
				continue;
			}

			const int baseZ = getZValueForType(object.data.sheetType) * 10000;
			addChild(block, baseZ + object.data.zValue);

			if (containerIndex == 1 && object.collisionPoints.empty() &&
				(object.passable || object.blocksMovement)) {
				// `solid` and related block types do not carry a points dictionary.
				// Their collision is the fully transformed sprite rectangle. A
				// passable object remains a one-way platform; otherwise every side
				// blocks the player, including doors, walls, crates and obstacles.
				level.collisionWorld.addShape(
					transformedBlockCorners(block),
					object.passable
					);
			}

			RuntimeNode runtime;
			runtime.node = block;
			// Sprite frames in one animation may have different trim offsets and
			// dimensions. The culling rectangle therefore follows the current
			// transformed sprite bounds instead of being frozen on frame zero.
			runtime.boundsPadding = object.data.animated || !object.data.customAnim.empty() ? 48.0f : 12.0f;
			runtime.bounds = expandedRect(block->boundingBox(), runtime.boundsPadding);
			runtime.active = true;
			runtime.visible = true;
			runtime.pauseActions = object.data.animated || !object.data.customAnim.empty();
			runtime.trackNodeBounds = true;
			m->runtimeNodes.push_back(runtime);
		}
	}

	if (!m->previewMode) {
		// Prefer the resolution-independent atlas name. Cocos2d-x will select an
		// HD variant automatically when appropriate. Explicitly try the supplied
		// -hd plist only when the normal atlas did not register the required frame.
		CCSpriteFrameCache* playerFrames = CCSpriteFrameCache::sharedSpriteFrameCache();
		if (!playerFrames->spriteFrameByName("Frost_Idle1_Anim_001.png")) {
			if (fileExists("Frost_Main_Character_spritesheet_01.plist")) {
				playerFrames->addSpriteFramesWithFile("Frost_Main_Character_spritesheet_01.plist");
			}
			if (!playerFrames->spriteFrameByName("Frost_Idle1_Anim_001.png") &&
				fileExists("Frost_Main_Character_spritesheet_01-hd.plist")) {
				playerFrames->addSpriteFramesWithFile("Frost_Main_Character_spritesheet_01-hd.plist");
			}
		}

		m->player = FL_Player::create(level.playerSpawn, level.levelSize, level.collisionWorld);
		if (!m->player) {
			// A missing player resource must not invalidate the whole level scene.
			// Keep the camera at playerSpawn and leave a diagnostic in the console.
			CCLog("Player could not be created; opening level without the player. Check Frost_Main_Character_spritesheet_01 resources.");
		}
		else {
			const int playerLayerZ = getZValueForType("P1") * 10000;
			addChild(m->player, playerLayerZ + 5000);
			m->cameraPos = cameraCenterWithScreenOffset(
				m->player->getPosition(),
				m->cameraOffset,
				m->zoom
				);
		}
	}

	for (std::vector<NpcSpawn>::const_iterator spawn = level.npcSpawns.begin(); spawn != level.npcSpawns.end(); ++spawn) {
		std::shared_ptr<EntityDescriptor> descriptor;
		std::map<std::string, std::shared_ptr<EntityDescriptor> >::iterator cached = m->entityDescriptors.find(spawn->npcType);
		if (cached != m->entityDescriptors.end()) {
			descriptor = cached->second;
		}
		else {
			descriptor = buildEntityDescriptor(spawn->npcType, m->loadedAtlases);
			m->entityDescriptors[spawn->npcType] = descriptor;
		}
		if (!descriptor) continue;

		EntityRuntime entity;
		entity.root = CCNode::create();
		entity.descriptor = descriptor;
		entity.root->setPosition(spawn->position);
		entity.root->setScaleX(spawn->scale.x * (spawn->flippedX ? -1.0f : 1.0f));
		entity.root->setScaleY(spawn->scale.y * (spawn->flippedY ? -1.0f : 1.0f));
		entity.root->setRotation(spawn->rotation);
		// NPC is a real map layer between P1 and B1. A level may explicitly
		// override it (for example B2 for cinematic/background characters).
		const int entityLayerZ = getZValueForType(spawn->layerType) * 10000;
		addChild(entity.root, entityLayerZ + spawn->zOrder);
		applyEntityFrame(entity, 0);

		const int entityIndex = static_cast<int>(m->entities.size());
		m->entities.push_back(entity);

		const float width = descriptor->boundsSize.width * std::fabs(spawn->scale.x);
		const float height = descriptor->boundsSize.height * std::fabs(spawn->scale.y);
		RuntimeNode runtime;
		runtime.node = entity.root;
		runtime.bounds = CCRect(
			spawn->position.x - width * 0.5f,
			spawn->position.y - height * 0.5f,
			width,
			height
			);
		runtime.active = true;
		runtime.visible = true;
		runtime.pauseActions = false;
		runtime.trackNodeBounds = false;
		runtime.boundsPadding = 24.0f;
		runtime.entityIndex = entityIndex;
		m->runtimeNodes.push_back(runtime);
	}

	const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
	setPosition(ccp(
		winSize.width * 0.5f - m->cameraPos.x * m->zoom,
		winSize.height * 0.5f - m->cameraPos.y * m->zoom
		));
	setScale(m->zoom);

	update(0.0f);
	return true;
}

void FL_PlayLayer::registerWithTouchDispatcher() {
	CCDirector::sharedDirector()->getTouchDispatcher()->addStandardDelegate(this, 0);
}

void FL_PlayLayer::attachFixedBackground(CCNode* parent, int zOrder) {
	if (!parent || !m->fixedBackground) return;
	if (m->fixedBackground->getParent() == parent) {
		parent->reorderChild(m->fixedBackground, zOrder);
		return;
	}

	// Preserve the autoreleased sprite while moving it out of the camera layer.
	m->fixedBackground->retain();
	m->fixedBackground->removeFromParentAndCleanup(false);
	parent->addChild(m->fixedBackground, zOrder);
	m->fixedBackground->release();

	const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
	const CCSize textureSize = m->fixedBackground->getContentSize();
	const float cover = textureSize.width > 0.0f && textureSize.height > 0.0f
		? std::max(winSize.width / textureSize.width, winSize.height / textureSize.height)
		: 1.0f;
	m->fixedBackground->setPosition(ccp(
		winSize.width * 0.5f,
		winSize.height * 0.5f + m->backgroundOffsetY
		));
	m->fixedBackground->setScale(cover);
}

void FL_PlayLayer::onEnter() {
	CCLayer::onEnter();
	// Register only after the node belongs to a running scene. This keeps the
	// scheduler retain/release pair aligned with the scene lifecycle.
	scheduleUpdate();
}

void FL_PlayLayer::onExit() {
	// Unschedule before CCLayer/CCNode begins removing children and releasing
	// scene-owned objects. This is important on cocos2d-x 2.2.x.
	unscheduleUpdate();
	CCLayer::onExit();
}

void FL_PlayLayer::update(float dt) {
	if (!m->previewMode && m->player) {
		bool keyboardLeft = false;
		bool keyboardRight = false;
		bool keyboardJump = false;
		readGameplayKeys(keyboardLeft, keyboardRight, keyboardJump);

		float moveInput = 0.0f;
		if (m->left || keyboardLeft) moveInput -= 1.0f;
		if (m->right || keyboardRight) moveInput += 1.0f;
		m->player->setMoveInput(moveInput);

		const bool jumpInputDown = m->up || keyboardJump;
		if (jumpInputDown && !m->jumpInputWasDown) m->player->requestJump();
		m->jumpInputWasDown = jumpInputDown;
		m->player->step(dt);

		CCPoint playerPosition = m->player->getPosition();
		const CCRect playerBounds = m->player->getCollisionBounds();
		for (std::vector<EntityRuntime>::iterator entity = m->entities.begin();
			entity != m->entities.end(); ++entity) {
			if (!entity->active || !entity->visible || !entity->root ||
				!entity->descriptor || entity->descriptor->contactDamage <= 0) {
				continue;
			}

			const CCPoint entityPosition = entity->root->getPosition();
			const float width = entity->descriptor->boundsSize.width *
				std::fabs(entity->root->getScaleX());
			const float height = entity->descriptor->boundsSize.height *
				std::fabs(entity->root->getScaleY());
			const CCRect entityBounds(
				entityPosition.x - width * 0.5f,
				entityPosition.y - height * 0.5f,
				width,
				height
			);
			if (!rectsIntersect(playerBounds, entityBounds)) continue;

			if (m->player->takeDamage(
				entity->descriptor->contactDamage,
				entityPosition,
				230.0f,
				300.0f
			)) {
				playerPosition = m->player->getPosition();
			}
			break;
		}

		// A spatial trigger is selected by the player's center point. Testing the
		// whole body made adjacent/overlapping camera zones fire too early and, in
		// the same frame, overwrite each other. Higher p_uID keeps the original
		// editor ordering when two regions genuinely contain the same point.
		CameraTriggerData* selectedTrigger = NULL;
		for (std::vector<CameraTriggerData>::iterator trigger = m->cameraTriggers.begin();
			trigger != m->cameraTriggers.end(); ++trigger) {
			if (!pointInsideRect(trigger->bounds, playerPosition)) continue;
			if (!selectedTrigger || trigger->pUID > selectedTrigger->pUID) {
				selectedTrigger = &(*trigger);
			}
		}

		if (!selectedTrigger) {
			m->activeCameraTriggerId.clear();
			m->activeCameraTriggerTime = 0.0f;
			m->activeCameraTriggerFired = false;
		}
		else {
			if (m->activeCameraTriggerId != selectedTrigger->id) {
				m->activeCameraTriggerId = selectedTrigger->id;
				m->activeCameraTriggerTime = 0.0f;
				m->activeCameraTriggerFired = false;
			}

			m->activeCameraTriggerTime += std::max(0.0f, dt);
			if (!m->activeCameraTriggerFired &&
				m->activeCameraTriggerTime + 0.0001f >= selectedTrigger->triggerDelay) {
				m->applyCameraTrigger(*selectedTrigger, playerPosition);
				m->activeCameraTriggerFired = true;
			}
		}

		m->updateCameraTransitions(dt);

		const CCPoint cameraBase = m->cameraLocked
			? m->cameraLockPosition
			: playerPosition;
		const CCPoint target = cameraCenterWithScreenOffset(
			cameraBase,
			m->cameraOffset,
			m->zoom
			);
		if (m->snapCameraPosition) {
			m->cameraPos = target;
			m->snapCameraPosition = false;
		}
		else {
			// A light exponential follow gives a small, stable delay without
			// oscillation. Trigger offsets and lock positions use the same follow,
			// so camera changes never produce a one-frame jump.
			const float follow = dt > 0.0f
				? 1.0f - std::exp(-12.0f * std::min(dt, 0.1f))
				: 1.0f;
			m->cameraPos = ccpAdd(
				m->cameraPos,
				ccpMult(ccpSub(target, m->cameraPos), follow)
				);
		}
	}

	const CCSize winSize = CCDirector::sharedDirector()->getWinSize();
	if (!m->previewMode) {
		m->cameraPos = clampCameraCenter(m->cameraPos, m->levelSize, winSize, m->zoom);
	}
	setPosition(ccp(
		winSize.width * 0.5f - m->cameraPos.x * m->zoom,
		winSize.height * 0.5f - m->cameraPos.y * m->zoom
		));
	setScale(m->zoom);

	if (m->fixedBackground) {
		const CCSize textureSize = m->fixedBackground->getContentSize();
		const float cover = textureSize.width > 0.0f && textureSize.height > 0.0f
			? std::max(winSize.width / textureSize.width, winSize.height / textureSize.height)
			: 1.0f;

		// The background is attached to the untransformed scene/menu layer.
		// Keeping it in screen coordinates prevents sub-pixel camera drift.
		m->fixedBackground->setPosition(ccp(
			winSize.width * 0.5f,
			winSize.height * 0.5f + m->backgroundOffsetY
			));
		m->fixedBackground->setScale(cover);
	}

	// Visibility is updated every frame. Objects are made drawable in a small
	// off-screen guard band, so the GPU clips them naturally at the viewport
	// edge instead of setVisible() switching them on in the first visible pixel.
	// This is especially important on the right edge when the camera moves or is
	// dragged quickly. Animated sprite bounds are also refreshed every frame,
	// because trimmed atlas frames do not necessarily share one rectangle.
	const float safeZoom = std::max(0.01f, m->zoom);
	const float viewWidth = winSize.width / safeZoom;
	const float viewHeight = winSize.height / safeZoom;
	const CCRect viewport(
		m->cameraPos.x - viewWidth * 0.5f,
		m->cameraPos.y - viewHeight * 0.5f,
		viewWidth,
		viewHeight
		);
	const float renderPadding = 112.0f / safeZoom;
	const float activationPadding = 288.0f / safeZoom;
	const CCRect renderViewport = expandedRect(viewport, renderPadding);
	const CCRect activationViewport = expandedRect(viewport, activationPadding);

	for (std::vector<RuntimeNode>::iterator runtime = m->runtimeNodes.begin(); runtime != m->runtimeNodes.end(); ++runtime) {
		if (runtime->trackNodeBounds && runtime->node) {
			const CCRect currentBounds = runtime->node->boundingBox();
			if (currentBounds.size.width > 0.0f && currentBounds.size.height > 0.0f) {
				runtime->bounds = expandedRect(currentBounds, runtime->boundsPadding);
			}
		}

		const bool shouldBeActive = rectsIntersect(runtime->bounds, activationViewport);
		if (shouldBeActive != runtime->active) {
			runtime->active = shouldBeActive;
			if (runtime->pauseActions) {
				if (shouldBeActive) {
					runtime->node->resumeSchedulerAndActions();
					// If a fast drag skipped the normal off-screen warm area,
					// allow two action-manager ticks before exposing the node.
					runtime->warmupFrames = 2;
				}
				else {
					runtime->node->pauseSchedulerAndActions();
					runtime->warmupFrames = 0;
				}
			}
		}

		const bool intersectsRenderArea = rectsIntersect(runtime->bounds, renderViewport);
		const bool shouldBeVisible = intersectsRenderArea && runtime->warmupFrames == 0;
		if (shouldBeVisible != runtime->visible) {
			runtime->visible = shouldBeVisible;
			runtime->node->setVisible(shouldBeVisible);
		}

		if (runtime->entityIndex >= 0 && runtime->entityIndex < static_cast<int>(m->entities.size())) {
			EntityRuntime& entity = m->entities[runtime->entityIndex];
			entity.active = shouldBeActive;
			entity.visible = shouldBeVisible;
		}

		if (runtime->active && runtime->warmupFrames > 0) {
			--runtime->warmupFrames;
		}
	}

	for (std::vector<EntityRuntime>::iterator entity = m->entities.begin(); entity != m->entities.end(); ++entity) {
		if (!entity->active || !entity->descriptor || entity->descriptor->frames.size() <= 1) continue;
		entity->elapsed += dt;
		while (entity->elapsed >= entity->descriptor->frameDelay) {
			entity->elapsed -= entity->descriptor->frameDelay;
			applyEntityFrame(*entity, entity->frameIndex + 1);
		}
	}
}

void FL_PlayLayer::uiUpPressed() { m->up = true; }
void FL_PlayLayer::uiUpReleased() { m->up = false; }
void FL_PlayLayer::uiDownPressed() {
	m->down = true;
	if (m->player) m->player->requestAttack();
}
void FL_PlayLayer::uiDownReleased() { m->down = false; }
void FL_PlayLayer::uiLeftPressed() { m->left = true; }
void FL_PlayLayer::uiLeftReleased() { m->left = false; }
void FL_PlayLayer::uiRightPressed() { m->right = true; }
void FL_PlayLayer::uiRightReleased() { m->right = false; }

void FL_PlayLayer::uiMenuPressed() {
	m->pausedByUI = !m->pausedByUI;
	m->up = false;
	m->down = false;
	m->left = false;
	m->right = false;
	m->jumpInputWasDown = false;
	if (m->player) m->player->setMoveInput(0.0f);

	if (m->pausedByUI) {
		pauseSchedulerAndActions();
		if (m->player) m->player->pauseSchedulerAndActions();
		for (std::vector<RuntimeNode>::iterator runtime = m->runtimeNodes.begin();
			runtime != m->runtimeNodes.end(); ++runtime) {
			if (runtime->node) runtime->node->pauseSchedulerAndActions();
		}
	}
	else {
		resumeSchedulerAndActions();
		if (m->player) m->player->resumeSchedulerAndActions();
		for (std::vector<RuntimeNode>::iterator runtime = m->runtimeNodes.begin();
			runtime != m->runtimeNodes.end(); ++runtime) {
			if (runtime->node && runtime->active) runtime->node->resumeSchedulerAndActions();
		}
	}
}

void FL_PlayLayer::uiPauseRestartPressed() {
	CCScene* gameScene = FL_PlayLayer::scene(m->sceneArgs);
	if (gameScene) {
		CCDirector::sharedDirector()->replaceScene(
			CCTransitionFade::create(0.25f, gameScene)
		);
	}
}

void FL_PlayLayer::uiPauseQuitPressed() {
	keyBackClicked();
}

void FL_PlayLayer::uiPauseSoundPressed() {
	m->soundEnabled = !m->soundEnabled;
	SimpleAudioEngine* audio = SimpleAudioEngine::sharedEngine();
	if (!audio) return;
	const float volume = m->soundEnabled ? 1.0f : 0.0f;
	audio->setBackgroundMusicVolume(volume);
	audio->setEffectsVolume(volume);
}

bool FL_PlayLayer::uiSoundEnabled() const {
	return m->soundEnabled;
}

int FL_PlayLayer::uiCurrentHealth() const {
	return m->player ? m->player->getHealth() : 0;
}

int FL_PlayLayer::uiMaximumHealth() const {
	return m->player ? m->player->getMaxHealth() : 12;
}

bool FL_PlayLayer::ccTouchBegan(CCTouch* touch, CCEvent*) {
	m->isDragging = true;
	m->lastTouchPos = touch->getLocation();
	return true;
}

void FL_PlayLayer::ccTouchMoved(CCTouch* touch, CCEvent*) {
	if (!m->isDragging) return;
	const CCPoint location = touch->getLocation();
	const CCPoint delta = ccpSub(location, m->lastTouchPos);
	m->cameraPos.x -= delta.x / m->zoom;
	m->cameraPos.y -= delta.y / m->zoom;
	m->lastTouchPos = location;
}

void FL_PlayLayer::ccTouchEnded(CCTouch*, CCEvent*) {
	m->isDragging = false;
}

void FL_PlayLayer::keyBackClicked() {
	CCLayer* menuLayer = FL_MenuLayer::create();
	CCScene* scene = CCScene::create();
	scene->addChild(menuLayer);
	CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.25f, scene));
}