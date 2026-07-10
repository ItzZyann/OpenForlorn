#pragma once

class FL_UILayerDelegate {
public:
	virtual ~FL_UILayerDelegate() {}

	virtual void uiLeftPressed() = 0;
	virtual void uiLeftReleased() = 0;

	virtual void uiRightPressed() = 0;
	virtual void uiRightReleased() = 0;

	virtual void uiUpPressed() = 0;   // jump
	virtual void uiUpReleased() = 0;

	virtual void uiDownPressed() = 0; // attack
	virtual void uiDownReleased() = 0;
};
