#pragma once
#include "cocos2d.h"

USING_NS_CC;

class LagrangeDescriptor : public CCObject {
public:
	static LagrangeDescriptor* createLagrangeDescriptor(
		CCPoint endPos,
		CCPoint cp1,
		CCPoint cp2,
		float maxDistance,
		CCPoint startPos);

	CCArray* pointContainer();

	virtual bool init() { return true; }

	CREATE_FUNC(LagrangeDescriptor);

private:
	CCArray* m_points;
};