#if defined(_WIN32) || defined(_WIN64)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#include "FL_Block.h"
#include <algorithm>
#include <cstring>

USING_NS_CC;

FL_Block* FL_Block::create(const Data& data) {
	FL_Block* result = new FL_Block();
	if (result && result->init(data)) {
		result->autorelease();
		return result;
	}
	delete result;
	return NULL;
}

bool FL_Block::init(const FL_Block::Data& data) {
	if (!CCSprite::initWithSpriteFrameName(data.texture.c_str())) {
		CCLog("Could not create block %s", data.texture.c_str());
		return false;
	}

	setPosition(data.position);

	CCPoint scale = data.scale;
	const std::string hdPlist = data.spriteSheet + "-hd.plist";
	unsigned long hdSize = 0;
	unsigned char* hdData = CCFileUtils::sharedFileUtils()->getFileData(hdPlist.c_str(), "rb", &hdSize);
	const bool usesHDAtlas = hdData != NULL && hdSize > 0;
	if (hdData) delete[] hdData;
	if (usesHDAtlas) scale = ccpMult(scale, 0.5f);

	setScaleX(scale.x * (data.flippedX ? -1.0f : 1.0f));
	setScaleY(scale.y * (data.flippedY ? -1.0f : 1.0f));

	if (data.hasRotation) setRotation(data.rotation);
	if (data.hasOpacity) setOpacity(static_cast<GLubyte>(std::max(0.0f, std::min(255.0f, data.opacity * 255.0f))));
	if (data.hasColor) {
		setColor(ccc3(
			static_cast<GLubyte>(std::max(0.0f, std::min(255.0f, data.red * 255.0f))),
			static_cast<GLubyte>(std::max(0.0f, std::min(255.0f, data.green * 255.0f))),
			static_cast<GLubyte>(std::max(0.0f, std::min(255.0f, data.blue * 255.0f)))
			));
	}

	if (!data.customAnim.empty()) runCustomAnimation(data);
	else if (data.animated) runAnimationLooped(data);

	return true;
}

void FL_Block::runCustomAnimation(const Data&) {
	CCMoveBy* up = CCMoveBy::create(1.0f, ccp(0.0f, 10.0f));
	CCActionInterval* down = up->reverse();
	CCSequence* sequence = CCSequence::create(up, down, NULL);
	runAction(CCRepeatForever::create(sequence));
}

void FL_Block::runAnimationLooped(const Data& data) {
	std::string baseName = data.texture;
	const std::string::size_type dot = baseName.find_last_of('.');
	if (dot != std::string::npos) baseName = baseName.substr(0, dot);

	CCSpriteFrameCache* cache = CCSpriteFrameCache::sharedSpriteFrameCache();
	CCArray* frames = CCArray::create();

	if (!data.skipStartAnim) {
		for (int index = 1;; ++index) {
			CCString* name = CCString::createWithFormat("%s_start_%03d.png", baseName.c_str(), index);
			CCSpriteFrame* frame = cache->spriteFrameByName(name->getCString());
			if (!frame) break;
			frames->addObject(frame);
		}
	}

	for (int index = 1;; ++index) {
		CCString* name = CCString::createWithFormat("%s_looped_%03d.png", baseName.c_str(), index);
		CCSpriteFrame* frame = cache->spriteFrameByName(name->getCString());
		if (!frame) break;
		frames->addObject(frame);
	}

	if (!data.skipEndAnim) {
		for (int index = 1;; ++index) {
			CCString* name = CCString::createWithFormat("%s_end_%03d.png", baseName.c_str(), index);
			CCSpriteFrame* frame = cache->spriteFrameByName(name->getCString());
			if (!frame) break;
			frames->addObject(frame);
		}
	}

	if (frames->count() > 0) {
		CCAnimate* animate = CCAnimate::create(
			CCAnimation::createWithSpriteFrames(frames, 1.0f / 12.0f));
		runAction(CCRepeatForever::create(animate));
	}
	else {
		CCLog("Skipping animation for block %s: No frames found.", data.texture.c_str());
	}
}

void FL_Block::updateTweenAction(float, const char*) {
}
