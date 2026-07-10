#pragma once
#include "incl.h"
#include "FL_Block.h"
#include "FL_UILayerDelegate.h"

#include <string>

class FL_PlayLayer : public CCLayer, public FL_UILayerDelegate {
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

    virtual void onEnter();
    virtual void onExit();
    virtual void update(float dt);

    void attachFixedBackground(CCNode* parent, int zOrder);

private:
    struct Members;
    Members* m;
};
