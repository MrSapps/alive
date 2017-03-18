#pragma once

#include <functional>
#include "types.hpp"

struct Debug
{
    bool mAnimBoundingBoxes = false;
    bool mAnimDebugStrings = false;
    bool mCollisionLines = true;
    bool mGrid = true;
    bool mObjectBoundingBoxes = false;
    bool mRayCasts = true;
    bool mShowDebugUi = true;
    bool mShowBrowserUi = false;

    struct BrowserUi
    {
        BrowserUi()
        {

        }
        bool fmvBrowserOpen;
        bool soundBrowserOpen;
        bool levelBrowserOpen;
        bool animationBrowserOpen;
        bool guiLayoutEditorOpen;
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

    void Update(class InputState& input);
    void Render(class Renderer& renderer, struct GuiContext& gui);
};

Debug& Debugging();
