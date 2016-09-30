#include "debug.hpp"
#include "engine.hpp"
#include "renderer.hpp"
#include "gui.h"

struct Key
{
    const char* mName;
    SDL_Scancode mScanCode;
    bool mFakeDown;
};

static Key gkeys[] =
{
    { "Left", SDL_SCANCODE_LEFT, false },
    { "Right", SDL_SCANCODE_RIGHT, false }
};

Debug& Debugging()
{
    static Debug d;
    return d;
}

void Debug::Update(class InputState& input)
{
    if (input.mKeys[SDL_SCANCODE_F1].IsPressed())
    {
        mShowDebugUi = !mShowDebugUi;
    }
}

void Debug::Render(Renderer& /*renderer*/, GuiContext& gui)
{
    if (!mShowDebugUi)
    {
        return;
    }

    gui_begin_window(&gui, "Debugging");

    gui_label(&gui, "Press F1 to toggle debugging window");

    gui_checkbox(&gui, "Anim bounding boxes", &mAnimBoundingBoxes);
    gui_checkbox(&gui, "Anim debug strings", &mAnimDebugStrings);
    gui_checkbox(&gui, "Collision lines", &mCollisionLines);
    gui_checkbox(&gui, "Grid", &mGrid);
    gui_checkbox(&gui, "Object bounding boxes", &mObjectBoundingBoxes);
    gui_checkbox(&gui, "Ray casts", &mRayCasts);
    gui_checkbox(&gui, "Browser UI", &mShowBrowserUi);
    
    if (gui_button(&gui, "Load next path"))
    {
        mFnNextPath();
    }

    if (gui_button(&gui, "Reload path"))
    {
        mFnReloadPath();
    }

    // Map object debugging
    if (gui_checkbox(&gui, "Single step object", &mSingleStepObject))
    {
        if (!mSingleStepObject)
        {
            mDoSingleStepObject = false;
        }
    }

    if (gui_button(&gui, "Do step"))
    {
        if (mSingleStepObject)
        {
            mDoSingleStepObject = true;
        }
    }

    if (mDebugObj)
    {
        gui_label(&gui, ("mXPos: " + std::to_string(mInfo.mXPos)).c_str());
        gui_label(&gui, ("mYPos: " + std::to_string(mInfo.mYPos)).c_str());
        gui_label(&gui, ("Frame: " + std::to_string(mInfo.mFrameToRender)).c_str());
    }

    if (mInput)
    {
        for (Key& key : gkeys)
        {
            if (gui_checkbox(&gui, key.mName, &key.mFakeDown))
            {
                SDL_KeyboardEvent e = {};
                e.keysym.scancode = key.mScanCode;
                e.type = key.mFakeDown ? SDL_KEYDOWN : SDL_KEYUP;
                mInput->OnKeyEvent(e);
            }
        }
    }

    gui_end_window(&gui);
}
