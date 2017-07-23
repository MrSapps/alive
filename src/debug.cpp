#include "debug.hpp"
#include "engine.hpp"
#include "abstractrenderer.hpp"

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

    if (input.ActiveController() && input.ActiveController()->mGamePadButtons[SDL_CONTROLLER_BUTTON_GUIDE].IsPressed())
    {
        mFnNextPath();
    }

    if (mShowDebugUi)
    {
        if (ImGui::Begin("Debugging"))
        {
            ImGui::TextUnformatted("Press F1 to toggle debugging window");

            if (ImGui::CollapsingHeader("Debug draws"))
            {
                ImGui::Checkbox("Cameras", &mDrawCameras);
                ImGui::Checkbox("Objects", &mDrawObjects);
                ImGui::Checkbox("Anim bounding boxes", &mAnimBoundingBoxes);
                ImGui::Checkbox("Anim debug strings", &mAnimDebugStrings);
                ImGui::Checkbox("Collision lines", &mCollisionLines);
                ImGui::Checkbox("Grid", &mGrid);
                ImGui::Checkbox("Object bounding boxes", &mObjectBoundingBoxes);
                ImGui::Checkbox("Ray casts", &mRayCasts);
                ImGui::Checkbox("Display font atlas", &mDrawFontAtlas);
                if (ImGui::Checkbox("VSync", &mVsync))
                {
                    mChangeVSync = true;
                }
            }

            if (ImGui::CollapsingHeader("Subtitle test"))
            {
                ImGui::Checkbox("Regular", &mSubtitleTestRegular);
                ImGui::Checkbox("Italic", &mSubtitleTestItalic);
                ImGui::Checkbox("Bold", &mSubtitleTestBold);
            }

            if (ImGui::CollapsingHeader("Browsers"))
            {
                ImGui::Checkbox("Sound browser", &Debugging().mBrowserUi.soundBrowserOpen);
                ImGui::Checkbox("Level browser", &Debugging().mBrowserUi.levelBrowserOpen);
                ImGui::Checkbox("Animation browser", &Debugging().mBrowserUi.animationBrowserOpen);
            }

            for (auto& section : mSections)
            {
                section();
            }

            if (ImGui::CollapsingHeader("Paths"))
            {
                if (ImGui::Button("Load next path"))
                {
                    mFnNextPath();
                }

                if (ImGui::Button("Reload path"))
                {
                    mFnReloadPath();
                }
            }

            if (ImGui::CollapsingHeader("Object debug"))
            {
                if (ImGui::Checkbox("Single step object", &mSingleStepObject))
                {
                    if (!mSingleStepObject)
                    {
                        mDoSingleStepObject = false;
                    }
                }

                if (ImGui::Button("Do step"))
                {
                    if (mSingleStepObject)
                    {
                        mDoSingleStepObject = true;
                    }
                }

                if (mDebugObj)
                {
                    ImGui::TextUnformatted(("mXPos: " + std::to_string(mInfo.mXPos)).c_str());
                    ImGui::TextUnformatted(("mYPos: " + std::to_string(mInfo.mYPos)).c_str());
                    ImGui::TextUnformatted(("Frame: " + std::to_string(mInfo.mFrameToRender)).c_str());
                }

                if (mInput)
                {
                    for (Key& key : gkeys)
                    {
                        if (ImGui::Checkbox(key.mName, &key.mFakeDown))
                        {
                            SDL_KeyboardEvent e = {};
                            e.keysym.scancode = key.mScanCode;
                            e.type = key.mFakeDown ? SDL_KEYDOWN : SDL_KEYUP;
                            mInput->OnKeyEvent(e);
                        }
                    }
                }
            }
        }
        ImGui::End();
    }
}

void Debug::Render(AbstractRenderer& rend)
{
    if (mChangeVSync)
    {
        mChangeVSync = false;
        rend.SetVSync(mVsync);
    }
}
