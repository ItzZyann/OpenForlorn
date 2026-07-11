#pragma once

class FL_UILayerDelegate {
public:
	virtual ~FL_UILayerDelegate() {}

	virtual void uiLeftPressed() = 0;
	virtual void uiLeftReleased() = 0;

	virtual void uiRightPressed() = 0;
	virtual void uiRightReleased() = 0;

	virtual void uiUpPressed() = 0;
	virtual void uiUpReleased() = 0;

	virtual void uiDownPressed() = 0;
	virtual void uiDownReleased() = 0;

	virtual void uiMenuPressed() = 0;
	virtual void uiPauseRestartPressed() = 0;
	virtual void uiPauseQuitPressed() = 0;
	virtual void uiPauseSoundPressed() = 0;
	virtual bool uiSoundEnabled() const = 0;
	virtual int uiCurrentHealth() const = 0;
	virtual int uiMaximumHealth() const = 0;
};
