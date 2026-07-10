#pragma once
#include "incl.h"

class FL_MenuLayer : public CCLayer {
public:
    virtual bool init();
    virtual void keyBackClicked();

    void onExitGame(CCObject* sender);
    void onSelectLevel(CCObject* sender);

    CREATE_FUNC(FL_MenuLayer);
};
