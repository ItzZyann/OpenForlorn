#ifndef __APPMACROS_H__
#define __APPMACROS_H__

#include "cocos2d.h"

#define DESIGN_RESOLUTION_960X520    0

#define TARGET_DESIGN_RESOLUTION_SIZE  DESIGN_RESOLUTION_960X520

typedef struct tagResource
{
    cocos2d::CCSize size;
    char directory[100];
}Resource;

static Resource smallResource  =  { cocos2d::CCSizeMake(480, 260),   "iphone" };
static Resource mediumResource =  { cocos2d::CCSizeMake(960, 520),   "ipad"   };
static Resource largeResource  =  { cocos2d::CCSizeMake(1920, 1040), "ipadhd" };

#if (TARGET_DESIGN_RESOLUTION_SIZE == DESIGN_RESOLUTION_960X520)
static cocos2d::CCSize designResolutionSize = cocos2d::CCSizeMake(960, 520);
#else
#error unknown target design resolution!
#endif

// Font size scaled relative to the 960x520 design resolution
#define TITLE_FONT_SIZE  (cocos2d::CCEGLView::sharedOpenGLView()->getDesignResolutionSize().width / 960.0f * 24)

#endif /* __APPMACROS_H__ */
