#include "FL_PlayLayer.h"
#include "FL_MenuLayer.h"
#include "FL_UILayer.h"

#include "JsonUtils.h"
#include "rapidjson/document.h"
#include "rapidjson/error/error.h" 

#include <cstring> 

struct LevelStruct {
	struct SheetData {
		std::string texture;
		std::string type;

		bool operator<(const SheetData& b) const {
			if (texture != b.texture) return texture < b.texture;
			return type < b.type;
		}
	};

	std::map<std::string, FL_Block::Data> blockContainer;
	std::map<std::string, FL_Block::Data> bgContainer;
	std::map<SheetData, std::string> sheetContainer;
	CCPoint playerSpawn;
};

struct FL_PlayLayer::Members {
	CCPoint playerPos;
	CCPoint spawnPos;
	float camSpeed;
	float zoom;

	bool up;
	bool down;
	bool left;
	bool right;

    CCPoint lastTouchPos;
    bool isDragging;
    
	std::map<LevelStruct::SheetData, CCSpriteBatchNode*> batchNodes;

	Members()
		: playerPos(ccp(0, 0))
		, spawnPos(ccp(0, 0))
		, camSpeed(7.0f)
		, zoom(1.3f)
		, up(false), down(false), left(false), right(false)
		, lastTouchPos(ccp(0, 0))
		, isDragging(false)
	{}
};

CCPoint parsePoint(const flrapidjson::Value& value) {
	if(value.IsArray() && value.Size() == 2) {
		return ccp(value[(flrapidjson::SizeType)0].GetDouble(),
			value[(flrapidjson::SizeType)1].GetDouble());
	}

	CCLog("Invalid position in JSON.");
	
    return ccp(0, 0);
}

bool loadLevelData(LevelStruct& lvl, const std::string& filename) {
	unsigned long bufferSize = 0;

	char* buffer = (char*)CCFileUtils::sharedFileUtils()->getFileData(filename.c_str(), "rb", &bufferSize);

	if(!buffer || bufferSize == 0) {
		CCLog("Could not load file %s. buf size: %lu", filename.c_str(), bufferSize);
		
        if(buffer) free(buffer);
		return false;
	}

	std::string jsonString(buffer, bufferSize);
	free(buffer);

	const char* jsonText = jsonString.c_str();
	size_t jsonTextLength = jsonString.length();

	if(jsonTextLength >= 3 &&
		(unsigned char)jsonText[0] == 0xEF &&
		(unsigned char)jsonText[1] == 0xBB &&
		(unsigned char)jsonText[2] == 0xBF) {
		jsonText += 3;
	}

	flrapidjson::Document doc;
	doc.Parse<0>(jsonText);

	if(doc.HasParseError()) {
		CCLog("Error parsing json file %s: rapidjson err no. %d (Offset %lu)",
			filename.c_str(), doc.GetParseError(), doc.GetErrorOffset());
		
        return false;
	}

	if(doc.HasMember("playerSpawn") && doc["playerSpawn"].IsArray()) {
		lvl.playerSpawn = parsePoint(doc["playerSpawn"]);
	} else {
		lvl.playerSpawn = ccp(300, 300);
	}

	size_t blockCount = 0;

	if(doc.HasMember("blockContainer")) {
		const flrapidjson::Value& blockContainer = doc["blockContainer"];

		if(blockContainer.IsObject()) {
			for(
                flrapidjson::Value::ConstMemberIterator itr = blockContainer.MemberBegin();
                itr != blockContainer.MemberEnd();
                ++itr
            ) {
				const flrapidjson::Value& json = itr->value;

				if(!json.IsObject()) {
					CCLog("Block skipped '%s': not a json object.", itr->name.GetString());
					continue;
				}

				FL_Block::Data bd;

				if(
                    !json.HasMember("spriteSheet") || !json.HasMember("texture") ||
					!json.HasMember("sheetType") || !json.HasMember("position") ||
					!json.HasMember("scale") || !json.HasMember("zValue")
                ) {
					CCLog("Block skipped '%s' due to missing mandatory field", itr->name.GetString());
					continue;
				}

				bd.spriteSheet = json["spriteSheet"].GetString();
				bd.texture = json["texture"].GetString();
				bd.sheetType = json["sheetType"].GetString();

				if(json["position"].IsArray()) {
					bd.position = parsePoint(json["position"]);
				} else {
					CCLog("Block skipped '%s': 'position' is not an array.", itr->name.GetString());
					continue;
				}

				if(json["scale"].IsArray()) {
					bd.scale = parsePoint(json["scale"]);
				} else {
					CCLog("block skipped '%s': 'scale' is not an array.", itr->name.GetString());
					continue;
				}

				bd.zValue = json["zValue"].GetInt();

				if(json.HasMember("red") && json.HasMember("green") && json.HasMember("blue")) {
					bd.red = json["red"].GetDouble();
					bd.green = json["green"].GetDouble();
					bd.blue = json["blue"].GetDouble();
					bd.hasColor = true;
				}
				if(json.HasMember("opacity")) {
					bd.opacity = json["opacity"].GetDouble();
					bd.hasOpacity = true;
				}

				if(json.HasMember("flipped") && json["flipped"].IsArray() && json["flipped"].Size() >= 2) {
					const flrapidjson::Value& flippedArr = json["flipped"];

					const flrapidjson::Value& v0 = flippedArr[(flrapidjson::SizeType)0];
					if(v0.IsBool()) bd.flippedX = v0.GetBool();
					else if(v0.IsInt()) bd.flippedX = v0.GetInt() != 0;

					const flrapidjson::Value& v1 = flippedArr[(flrapidjson::SizeType)1];
					if(v1.IsBool()) bd.flippedY = v1.GetBool();
					else if(v1.IsInt()) bd.flippedY = v1.GetInt() != 0;
				}

				if(json.HasMember("rotation")) {
					bd.rotation = json["rotation"].GetDouble();
					bd.hasRotation = true;
				}

				if(json.HasMember("customAnim")) {
					bd.customAnim = json["customAnim"].GetString();
				}

				if(json.HasMember("animated")) {
					bd.animated = GetBoolFlexible(json, "animated");
				}

				LevelStruct::SheetData sheetData;
				sheetData.texture = bd.spriteSheet;
				sheetData.type = bd.sheetType;

				lvl.sheetContainer.insert(std::make_pair(sheetData, bd.spriteSheet));
				lvl.blockContainer[itr->name.GetString()] = bd;
				blockCount++;
			}
		}
		CCLog("Parsed %u total blocks from blockContainer.", (unsigned int)blockCount);
	}


	size_t bgElementCount = 0;
	if(doc.HasMember("bgContainer") && doc["bgContainer"].IsObject()) {
		const flrapidjson::Value& bgContainer = doc["bgContainer"];

		if(bgContainer.IsObject()) {
			for(
                flrapidjson::Value::ConstMemberIterator itr = bgContainer.MemberBegin();
				itr != bgContainer.MemberEnd();
                ++itr
            ) {
				const flrapidjson::Value& json = itr->value;

				if(!json.IsObject()) {
					CCLog("Background element skipped '%s': not a json object.", itr->name.GetString());
					continue;
				}

				FL_Block::Data bd;

				if(
                    !json.HasMember("spriteSheet") || !json.HasMember("texture") ||
					!json.HasMember("bgType") || !json.HasMember("position") ||
					!json.HasMember("scale") || !json.HasMember("zValue")
                ) {
					CCLog("Background element skipped '%s' due to missing mandatory field", itr->name.GetString());
					continue;
				}

				bd.spriteSheet = json["spriteSheet"].GetString();
				bd.texture = json["texture"].GetString();
				bd.sheetType = json["bgType"].GetString();

				if(json["position"].IsArray()) {
					bd.position = parsePoint(json["position"]);
				} else { continue; }

				if(json["scale"].IsArray()) {
					bd.scale = parsePoint(json["scale"]);
				} else { continue; }

				bd.zValue = json["zValue"].GetInt();

				if(json.HasMember("red") && json.HasMember("green") && json.HasMember("blue")) {
					bd.red = json["red"].GetDouble();
					bd.green = json["green"].GetDouble();
					bd.blue = json["blue"].GetDouble();
					bd.hasColor = true;
				}
				if(json.HasMember("opacity")) {
					bd.opacity = json["opacity"].GetDouble();
					bd.hasOpacity = true;
				}

				if(json.HasMember("flipped") && json["flipped"].IsArray() && json["flipped"].Size() >= 2) {
					const flrapidjson::Value& flippedArr = json["flipped"];

					const flrapidjson::Value& v0 = flippedArr[(flrapidjson::SizeType)0];
					if(v0.IsBool()) bd.flippedX = v0.GetBool();
					else if(v0.IsInt()) bd.flippedX = v0.GetInt() != 0;

					const flrapidjson::Value& v1 = flippedArr[(flrapidjson::SizeType)1];
					if(v1.IsBool()) bd.flippedY = v1.GetBool();
					else if(v1.IsInt()) bd.flippedY = v1.GetInt() != 0;
				}

				if(json.HasMember("rotation")) {
					bd.rotation = json["rotation"].GetDouble();
					bd.hasRotation = true;
				}

				LevelStruct::SheetData sheetData;
				sheetData.texture = bd.spriteSheet;
				sheetData.type = bd.sheetType;

				lvl.sheetContainer.insert(std::make_pair(sheetData, bd.spriteSheet));
				lvl.bgContainer[itr->name.GetString()] = bd;
				bgElementCount++;
			}
		}
		CCLog("Parsed %u total background elements from bgContainer.", (unsigned int)bgElementCount);
	}


	if(doc.HasMember("entityContainer")) {
		const flrapidjson::Value& entityContainer = doc["entityContainer"];
		
        if(entityContainer.IsObject()) {
			if(entityContainer.HasMember("player_0") && entityContainer["player_0"].IsObject()) {
				const flrapidjson::Value& player_0 = entityContainer["player_0"];
				if(player_0.HasMember("position") && player_0["position"].IsArray()) {
					lvl.playerSpawn = parsePoint(player_0["position"]);
				}
			}
		}
	}

	return true;
}

int getZValueForType(const std::string& type) {
	int ret = 0;
	if(type == "F1") ret = 160;
	else if(type == "F2") ret = 150;
	else if(type == "F3") ret = 140;
	else if(type == "F4") ret = 130;
	else if(type == "B1") ret = 50;
	else if(type == "B2") ret = 40;
	else if(type == "B3") ret = 30;
	else if(type == "B4") ret = 20;
	else if(type == "mg") ret = 10;     // mg
	else if(type == "fg") ret = 170;    // foreground
	else if(type == "BG") ret = 5;      // bg
	else if(type == "NPC") ret = 90;
	else if(type == "particle") ret = 0;
	else if(type == "P1") ret = 120;

	if(type.find('+') != std::string::npos) ret++;
	else if(type.find('-') != std::string::npos) ret--;

	return ret;
}

CCScene* FL_PlayLayer::scene(const Args& args) {
	auto scene = CCScene::create();
	auto playLayer = FL_PlayLayer::create(args);
	auto uiLayer = FL_UILayer::create();

	if(playLayer && uiLayer) {
		uiLayer->setDelegate(playLayer);
		scene->addChild(playLayer);
		scene->addChild(uiLayer);
		return scene;
	}

	return NULL;
}

FL_PlayLayer* FL_PlayLayer::create(const Args& args) {
	FL_PlayLayer *pRet = new FL_PlayLayer();
	if(pRet && pRet->init(args)) {
		pRet->autorelease();
		return pRet;
    } else {
		delete pRet;
		pRet = NULL;
		return NULL;
	}
}

FL_PlayLayer::FL_PlayLayer() : m(new Members()) {}
FL_PlayLayer::~FL_PlayLayer() { delete m; }

bool FL_PlayLayer::init(const Args& args) {
	if (!CCLayer::init()) return false;

    SimpleAudioEngine::sharedEngine()
        ->playBackgroundMusic("loop_01.mp3", true);
    
	setTouchEnabled(true);
	setKeypadEnabled(true);
	scheduleUpdate();

	LevelStruct lvl;

	if(!loadLevelData(lvl, "Level001.json")) {
		return false;
	}

	m->spawnPos = lvl.playerSpawn;
	m->playerPos = lvl.playerSpawn;
    m->lastTouchPos = lvl.playerSpawn;
    
	auto sprch = CCSpriteFrameCache::sharedSpriteFrameCache();

	std::map<LevelStruct::SheetData, std::string>::iterator sheetIt = lvl.sheetContainer.begin();

    while(sheetIt != lvl.sheetContainer.end()) {
        LevelStruct::SheetData sheetData = sheetIt->first;

        auto plist = CCString::createWithFormat("%s.plist", sheetData.texture.c_str());
        auto png = CCString::createWithFormat("%s.png", sheetData.texture.c_str());

        unsigned long size = 0;
        unsigned char* pData = CCFileUtils::sharedFileUtils()->getFileData(png->getCString(), "rb", &size);
        
        if(!pData || size == 0) {
            CCLog("[ERROR] Sheet asset missing or unreadable: %s", png->getCString());

            if(pData) delete[] pData; 

            sheetIt = lvl.sheetContainer.erase(sheetIt);
            continue;
        }

        delete[] pData;

        sprch->addSpriteFramesWithFile(plist->getCString());

        auto batch = CCSpriteBatchNode::create(png->getCString());
        if(batch) {
            m->batchNodes[sheetData] = batch;
            addChild(batch, getZValueForType(sheetData.type));
            ++sheetIt;
        } else {
            sheetIt = lvl.sheetContainer.erase(sheetIt);
        }
    }


	std::map<std::string, FL_Block::Data>::iterator bgIt;
	for(bgIt = lvl.bgContainer.begin(); bgIt != lvl.bgContainer.end(); ++bgIt) {
		FL_Block::Data& bd = bgIt->second;

		CCSpriteBatchNode* bn = NULL;
		std::map<LevelStruct::SheetData, CCSpriteBatchNode*>::iterator batchIt;

		for(batchIt = m->batchNodes.begin(); batchIt != m->batchNodes.end(); ++batchIt) {
			if(batchIt->first.texture == bd.spriteSheet && batchIt->first.type == bd.sheetType) {
				bn = batchIt->second;
				break;
			}
		}

		if(!bn) {
			CCLog("BG ERROR: BatchNode NOT found for element '%s' (Sheet: %s, Type: %s). Skipping.",
				bgIt->first.c_str(), bd.spriteSheet.c_str(), bd.sheetType.c_str());
			
            continue;
		}

		auto bgElement = FL_Block::create(bd);
		if(bgElement) {
			bn->addChild(bgElement, bd.zValue);
		} else {
			CCLog("BG ERROR: Failed to create element '%s' (Texture: %s). Check if sprite frame exists in the .plist.",
				bgIt->first.c_str(), bd.texture.c_str());
		}
	}

	// kinda messy, but hey it works.
	std::map<std::string, FL_Block::Data>::iterator blockIt;
	for(blockIt = lvl.blockContainer.begin(); blockIt != lvl.blockContainer.end(); ++blockIt) {
		FL_Block::Data& bd = blockIt->second;

		CCSpriteBatchNode* bn = NULL;
		std::map<LevelStruct::SheetData, CCSpriteBatchNode*>::iterator batchIt;

		for(batchIt = m->batchNodes.begin(); batchIt != m->batchNodes.end(); ++batchIt) {
			if(batchIt->first.texture == bd.spriteSheet && batchIt->first.type == bd.sheetType) {
				bn = batchIt->second;
				break;
			}
		}

		if(!bn) {
			CCLog("FL_Block ERROR: BatchNode NOT found for element '%s' (Sheet: %s, Type: %s). Skipping.",
				blockIt->first.c_str(), bd.spriteSheet.c_str(), bd.sheetType.c_str());
			continue;
		}

		auto block = FL_Block::create(bd);
		if(block) {
			bn->addChild(block, bd.zValue);
		}
		else {
			CCLog("FL_Block ERROR: Failed to create element '%s' (Texture: %s). Check if sprite frame exists in the .plist.",
				blockIt->first.c_str(), bd.texture.c_str());
		}
	}

	return true;
}


void FL_PlayLayer::registerWithTouchDispatcher() {
	CCDirector::sharedDirector()
		->getTouchDispatcher()
		    ->addStandardDelegate(this, 0);
}

void FL_PlayLayer::onEnter() { CCLayer::onEnter(); }
void FL_PlayLayer::onExit() { CCLayer::onExit(); }

void FL_PlayLayer::update(float dt) {
	CCSize winSize = CCDirector::sharedDirector()->getWinSize();

	if(m->left)  m->playerPos.x -= m->camSpeed;
	if(m->right) m->playerPos.x += m->camSpeed;
	if(m->up)    m->playerPos.y += m->camSpeed;
	if(m->down)  m->playerPos.y -= m->camSpeed;

	float x = winSize.width / 2.0f - m->playerPos.x * m->zoom;
	float y = winSize.height / 2.0f - m->playerPos.y * m->zoom;
	
	setPosition(ccp(x, y));
	setScale(m->zoom);
}

void FL_PlayLayer::uiUpPressed() { m->up = true; }
void FL_PlayLayer::uiUpReleased() { m->up = false; }
void FL_PlayLayer::uiDownPressed() { m->down = true; }
void FL_PlayLayer::uiDownReleased() { m->down = false; }
void FL_PlayLayer::uiLeftPressed() { m->left = true; }
void FL_PlayLayer::uiLeftReleased() { m->left = false; }
void FL_PlayLayer::uiRightPressed() { m->right = true; }
void FL_PlayLayer::uiRightReleased() { m->right = false; }

bool FL_PlayLayer::ccTouchBegan(CCTouch* touch, CCEvent* event) { return true; }
void FL_PlayLayer::ccTouchMoved(CCTouch* touch, CCEvent* event) { }
void FL_PlayLayer::ccTouchEnded(CCTouch* touch, CCEvent* event) { }

void FL_PlayLayer::keyBackClicked() {
	auto layer = FL_MenuLayer::create();
	auto scene = CCScene::create();
	scene->addChild(layer);

	CCDirector::sharedDirector()
		->replaceScene(scene);
}