#include "LagrangeDescriptor.h"

USING_NS_CC;

LagrangeDescriptor* LagrangeDescriptor::createLagrangeDescriptor(
	CCPoint endPos,
	CCPoint cp1,
	CCPoint cp2,
	float maxDistance,
	CCPoint startPos)
{
	LagrangeDescriptor* ld = new LagrangeDescriptor();
	ld->m_points = CCArray::create();
	ld->m_points->retain();

	ld->m_points->addObject(CCString::createWithFormat("{%f, %f}", startPos.x, startPos.y));

	const CCPoint pts[4] = { startPos, cp1, cp2, endPos };
	const float   ts[4] = { 0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f };

	const int kSamples = 100;
	float arcLen = 0.0f;
	CCPoint prev = startPos;
	for (int s = 1; s <= kSamples; ++s) {
		float t = (float)s / (float)kSamples;
		float x = 0.0f, y = 0.0f;
		for (int i = 0; i < 4; ++i) {
			float basis = 1.0f;
			for (int j = 0; j < 4; ++j) {
				if (j != i)
					basis *= (t - ts[j]) / (ts[i] - ts[j]);
			}
			x += pts[i].x * basis;
			y += pts[i].y * basis;
		}
		CCPoint cur = ccp(x, y);
		float dx = cur.x - prev.x;
		float dy = cur.y - prev.y;
		arcLen += sqrtf(dx * dx + dy * dy);
		prev = cur;
	}

	int steps = (int)(arcLen / maxDistance);
	if (steps < 1) steps = 1;

	for (int s = 1; s <= steps; ++s) {
		float t = (float)s / (float)(steps + 1);
		float x = 0.0f, y = 0.0f;
		for (int i = 0; i < 4; ++i) {
			float basis = 1.0f;
			for (int j = 0; j < 4; ++j) {
				if (j != i)
					basis *= (t - ts[j]) / (ts[i] - ts[j]);
			}
			x += pts[i].x * basis;
			y += pts[i].y * basis;
		}
		ld->m_points->addObject(CCString::createWithFormat("{%f, %f}", x, y));
	}

	ld->autorelease();
	return ld;
}

CCArray* LagrangeDescriptor::pointContainer() {
	return m_points;
}