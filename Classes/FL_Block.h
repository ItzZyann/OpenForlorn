#pragma once
#include "incl.h"
#include <string>
#include <map>

class FL_Block : public CCSprite, public CCActionTweenDelegate {
public:
	struct Data {
		float rotation;
		bool hasRotation;

		float opacity;
		bool hasOpacity;

		float red;
		float green;
		float blue;
		bool hasColor;

		CCPoint scale;
		CCPoint position;

		int p_uID;
		int zValue;

		std::string spriteSheet;
		std::string texture;
		std::string sheetType;
		std::string customAnim;

		bool darken;
		bool animated;
		bool skipStartAnim;
		bool skipEndAnim;

		bool flippedX;
		bool flippedY;

		Data()
			: rotation(0), hasRotation(false)
			, opacity(1.0f), hasOpacity(false)
			, red(1.0f), green(1.0f), blue(1.0f), hasColor(false)
			, scale(ccp(1, 1)), position(ccp(0, 0))
			, p_uID(0), zValue(0)
			, darken(false), animated(false)
			, skipStartAnim(false), skipEndAnim(false)
			, flippedX(false), flippedY(false)
		{}
	};

	static FL_Block* create(const Data& data);
	bool init(const Data& data);

	void runPulsingAnimationForever();
	void runAnimationLooped(const Data& d);
	void runCustomAnimation(const Data& d);

	virtual void updateTweenAction(float value, const char* key);
};
