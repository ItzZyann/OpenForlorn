#pragma once

#include "../incl.h"
#include "../rapidjson/document.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class FL_TriggerHost {
public:
    virtual ~FL_TriggerHost() {}
    virtual void triggerCamera(const CCPoint& offset, bool hasOffset, float zoomTarget,
        bool hasZoom, bool lockCamera, const CCPoint& lockPosition, bool hasLockPosition,
        bool resetCamera, float moveDuration, float zoomDuration,
        const std::string& moveEasing, const std::string& zoomEasing,
        bool instantMove, bool instantZoom) = 0;
    virtual void triggerCameraMove(const std::vector<CCPoint>& points,
        const std::vector<float>& durations, const std::vector<float>& delays,
        const std::vector<std::string>& easing, bool dontReturnToPlayer) = 0;
    virtual void triggerTeleport(const CCPoint& target, bool fade) = 0;
    virtual void triggerShake(float duration, const CCPoint& strength) = 0;
    virtual void triggerExitLevel(bool dontMove) = 0;
    virtual void triggerPlaySound(const std::string& sound) = 0;
    virtual void triggerReleaseCamera() = 0;
    virtual void triggerCommand(const std::string& type, const std::string& value,
        const std::string& secondary, const CCPoint& point, float amount,
        bool enabled, bool alternate) = 0;
};

namespace FLTriggers {

enum GroupCallType {
    GroupActivate = 0,
    GroupDeactivate = 1,
    GroupResume = 2,
    GroupPause = 3,
    GroupSwitchState = 4
};

class TriggerSystem;

class Trigger {
public:
    Trigger();
    virtual ~Trigger() {}

    virtual void load(const std::string& objectId, const flrapidjson::Value& json);
    virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
    virtual void update(float dt, const CCPoint& playerPosition, TriggerSystem& system, FL_TriggerHost& host);
    virtual void resetObject();
    virtual void activateObject(bool deactivating, TriggerSystem& system, FL_TriggerHost& host);

    const std::string& id() const { return id_; }
    const std::string& groupID() const { return groupID_; }
    int priority() const { return priority_; }
    bool calledByEvent() const { return calledByEvent_; }
    bool contains(const CCPoint& point) const;
    bool sleeping() const { return sleeping_; }
    void setSleeping(bool value) { sleeping_ = value; }
    void execute(TriggerSystem& system, FL_TriggerHost& host);

protected:
    friend class TriggerSystem;
    std::string id_;
    std::string groupID_;
    std::string callGroupID_;
    std::string triggeredSound_;
    CCRect bounds_;
    int priority_;
    float triggerDelay_;
    float delayRemaining_;
    float cooldownRemaining_;
    bool callOnlyOnce_;
    bool calledByEvent_;
    bool hasBeenExecuted_;
    bool sleeping_;
    bool defaultSleeping_;
    bool pending_;
    bool insideLastFrame_;
};

class TriggerOnOff : public Trigger {
public:
    TriggerOnOff();
    virtual void load(const std::string& id, const flrapidjson::Value& json);
    virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
private:
    bool activateTarget_, deactivateTarget_, pauseTarget_, resumeTarget_;
    bool sleepTarget_, wakeTarget_, switchStateOnly_;
};

class TriggerDeactivate : public Trigger {
public:
    virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
};

class TriggerExitLevel : public Trigger {
public:
    TriggerExitLevel();
    virtual void load(const std::string& id, const flrapidjson::Value& json);
    virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
private: bool dontMove_;
};

class TeleportTrigger : public Trigger {
public:
    TeleportTrigger();
    virtual void load(const std::string& id, const flrapidjson::Value& json);
    virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
private: CCPoint target_; bool fade_;
};

class TriggerEvent : public Trigger {
public: virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
protected: virtual void runEvent(TriggerSystem& system, FL_TriggerHost& host) {}
};

class TriggerEventShake : public TriggerEvent {
public:
    TriggerEventShake();
    virtual void load(const std::string& id, const flrapidjson::Value& json);
protected: virtual void runEvent(TriggerSystem& system, FL_TriggerHost& host);
private: float duration_; CCPoint strength_;
};

class TriggerCamera : public Trigger {
public:
    TriggerCamera();
    virtual void load(const std::string& id, const flrapidjson::Value& json);
    virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
    virtual void update(float dt, const CCPoint& playerPosition, TriggerSystem& system, FL_TriggerHost& host);
    void leaveSpatialZone(FL_TriggerHost& host);
private:
    CCPoint offset_, target_; std::string lockType_, offsetEasing_, zoomEasing_;
    float zoomTarget_, offsetDuration_, zoomDuration_;
    bool hasOffset_, hasTarget_, hasZoom_, lock_, instantMove_, instantZoom_;
};

class TriggerCameraMove : public Trigger {
public:
    TriggerCameraMove();
    virtual void load(const std::string& id, const flrapidjson::Value& json);
    virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
private:
    std::vector<CCPoint> points_; std::vector<float> durations_, delays_;
    std::vector<std::string> easing_; std::vector<std::string> checkpointGroups_;
    bool dontReturnToPlayer_;
};

class TriggerCommand : public Trigger {
public:
    TriggerCommand();
    virtual void load(const std::string& id, const flrapidjson::Value& json);
    virtual void activateTrigger(TriggerSystem& system, FL_TriggerHost& host);
private:
    std::string type_, value_, secondary_;
    CCPoint point_;
    float amount_;
    bool enabled_, alternate_;
};

// The original game exposes these concrete trigger classes. They share the
// base lifecycle here and can be extended as their corresponding level data is used.
class ButtonTrigger : public Trigger { public: virtual void activateTrigger(TriggerSystem&, FL_TriggerHost&); };
class CheckpointTrigger : public Trigger { public: virtual void activateTrigger(TriggerSystem&, FL_TriggerHost&); };
class TriggerAICommand : public Trigger {
public: virtual void load(const std::string&, const flrapidjson::Value&); virtual void activateTrigger(TriggerSystem&, FL_TriggerHost&);
private: std::string target_, command_;
};
class TriggerInfoText : public Trigger { public: virtual void activateTrigger(TriggerSystem&, FL_TriggerHost&); };
class TriggerGroupController : public Trigger { public: virtual void activateTrigger(TriggerSystem&, FL_TriggerHost&); };

class TriggerSystem {
public:
    TriggerSystem();
    bool load(const flrapidjson::Value& container);
    void update(float dt, const CCPoint& playerPosition, FL_TriggerHost& host);
    void activateGroup(const std::string& group, GroupCallType type, bool wake, bool sleep,
        FL_TriggerHost& host);
    void reset();
    size_t size() const { return triggers_.size(); }

private:
    std::vector<std::shared_ptr<Trigger> > triggers_;
    std::map<std::string, std::vector<Trigger*> > groups_;
    TriggerCamera* activeSpatialCamera_;
};

} // namespace FLTriggers
