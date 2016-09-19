#include "debug.hpp"
#include "engine.hpp"
#include "renderer.hpp"
#include "gui.h"

Debug& Debugging()
{
    static Debug d;
    return d;
}

void Debug::Update(class InputState& input)
{
    if (input.mKeys[SDL_SCANCODE_F1].mIsPressed)
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

    gui_end_window(&gui);
}
