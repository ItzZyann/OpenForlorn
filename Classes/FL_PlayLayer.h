#pragma once
#include "incl.h"
#include "FL_Block.h"
#include "FL_UILayerDelegate.h"
#include "FL_PlayerStance.h"
#include "Triggers/FL_TriggerSystem.h"

#include <string>
#include <vector>

class FL_UILayer;

class FL_PlayLayer : public CCLayer, public FL_UILayerDelegate, public FL_TriggerHost {
public:
    struct Args {
        std::string levelFile;
        bool previewMode;
        float initialZoom;
        CCPoint cameraOffset;

        Args()
            : levelFile("Level001.plist")
            , previewMode(false)
            , initialZoom(1.3f)
            , cameraOffset(CCPointZero) {
        }
    };

    static CCScene* scene(const Args& args);
    static FL_PlayLayer* create(const Args& args);

    virtual bool init(const Args& args);
    virtual ~FL_PlayLayer();
    FL_PlayLayer();

    virtual void registerWithTouchDispatcher();
    virtual bool ccTouchBegan(CCTouch* touch, CCEvent* event);
    virtual void ccTouchMoved(CCTouch* touch, CCEvent* event);
    virtual void ccTouchEnded(CCTouch* touch, CCEvent* event);

    virtual void keyBackClicked();

    virtual void uiUpPressed() override;
    virtual void uiUpReleased() override;
    virtual void uiDownPressed() override;
    virtual void uiDownReleased() override;
    virtual void uiLeftPressed() override;
    virtual void uiLeftReleased() override;
    virtual void uiRightPressed() override;
    virtual void uiRightReleased() override;
    virtual void uiMenuPressed() override;
    virtual void uiStancePressed() override;
    virtual void uiPauseRestartPressed() override;
    virtual void uiPauseQuitPressed() override;
    virtual void uiPauseSoundPressed() override;
    virtual bool uiSoundEnabled() const override;
    virtual int uiCurrentHealth() const override;
    virtual int uiMaximumHealth() const override;

    virtual void onEnter();
    virtual void onExit();
    virtual void update(float dt);

    void setUILayer(FL_UILayer* uiLayer);

    void attachFixedBackground(CCNode* parent, int zOrder);
    void spawnAttackProjectile(const CCPoint& playerPosition, bool facingRight, bool airborne, FL_PlayerStanceType stance);
    bool hasMeleeTarget(const CCRect& attackBounds) const;
    bool findMeleeTarget(const CCPoint& playerPosition, bool currentFacingRight, bool& targetFacingRight) const;
    bool performMeleeStrike(const CCRect& attackBounds, bool facingRight, FL_PlayerStanceType stance, int damage);

    virtual void triggerCamera(const CCPoint& offset, bool hasOffset, float zoomTarget,
        bool hasZoom, bool lockCamera, const CCPoint& lockPosition, bool hasLockPosition,
        bool resetCamera, float moveDuration, float zoomDuration,
        const std::string& moveEasing, const std::string& zoomEasing,
        bool instantMove, bool instantZoom);
    virtual void triggerCameraMove(const std::vector<CCPoint>& points,
        const std::vector<float>& durations, const std::vector<float>& delays,
        const std::vector<std::string>& easing, bool dontReturnToPlayer);
    virtual void triggerTeleport(const CCPoint& target, bool fade);
    virtual void triggerShake(float duration, const CCPoint& strength);
    virtual void triggerExitLevel(bool dontMove);
    virtual void triggerPlaySound(const std::string& sound);
    virtual void triggerReleaseCamera();
    virtual void triggerCommand(const std::string& type, const std::string& value,
        const std::string& secondary, const CCPoint& point, float amount,
        bool enabled, bool alternate);

private:
    struct Members;
    Members* m;
};
