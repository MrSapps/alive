#pragma once

#include <functional>

struct Debug
{
    bool mAnimBoundingBoxes = true;
    bool mAnimDebugStrings = true;
    bool mCollisionLines = true;
    bool mGrid = true;
    bool mObjectBoundingBoxes = true;
    bool mRayCasts = true;
    bool mShowDebugUi = true;
    bool mShowBrowserUi = false;

    std::function<void()> mFnReloadPath;
    std::function<void()> mFnNextPath;

    void Update(class InputState& input);
    void Render(class Renderer& renderer, struct GuiContext& gui);
};

Debug& Debugging();
