#pragma once

#include <functional>
#include <vector>
#include "types.hpp"

struct Debug
{
    bool mDrawCameras = true;
    bool mDrawObjects = true;
    bool mAnimBoundingBoxes = false;
    bool mAnimDebugStrings = false;
    bool mCollisionLines = true;
    bool mGrid = true;
    bool mObjectBoundingBoxes = false;
    bool mRayCasts = true;
    bool mSubtitleTestRegular = false;
    bool mSubtitleTestItalic = false;
    bool mSubtitleTestBold = false;
    bool mDrawFontAtlas = false;
    bool mVsync = true;
    bool mShowDebugUi = true;
    bool mChangeVSync = false;

    struct BrowserUi
    {
        BrowserUi()
        {

        }
        bool animationBrowserOpen;
    };

    BrowserUi mBrowserUi;

    bool mSingleStepObject = false;
    bool mDoSingleStepObject = false;
    class MapObject* mDebugObj = nullptr;
    struct MapObjectInfo
    {
        f32 mXPos;
        f32 mYPos;
        s32 mFrameToRender;
    };
    MapObjectInfo mInfo = {};

    class InputState* mInput = nullptr;

    std::function<void()> mFnReloadPath;
    std::function<void()> mFnNextPath;
    std::function<void(const char*)> fnLoadPath;

    void Update(class InputState& input);
    void Render(class AbstractRenderer& renderer);

    void AddSection(std::function<void()> fnSection)
    {
        mSections.push_back(fnSection);
    }

private:
    std::vector<std::function<void()>> mSections;
};

Debug& Debugging();
