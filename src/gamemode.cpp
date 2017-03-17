#include "gamemode.hpp"
#include "engine.hpp"
#include "gui.h"

GameMode::GameMode(GridMapState& mapState)
    : mMapState(mapState)
{

}

void GameMode::Update(const InputState& input, CoordinateSpace& coords)
{
    if (input.mKeys[SDL_SCANCODE_E].IsPressed())
    {
        mMapState.mState = GridMapState::eStates::eToEditor;
        coords.mSmoothCameraPosition = true;

        mMapState.mModeSwitchTimeout = SDL_GetTicks() + kSwitchTimeMs;

        //mCameraPosition.x = mPlayer.mXPos;
        //mCameraPosition.y = mPlayer.mYPos;
    }

    coords.SetScreenSize(mMapState.kVirtualScreenSize);


    for (auto& obj : mMapState.mObjs)
    {
        obj->Update(input);
    }


    if (mMapState.mCameraSubject)
    {
        const s32 camX = static_cast<s32>(mMapState.mCameraSubject->mXPos / mMapState.kCameraBlockSize.x);
        const s32 camY = static_cast<s32>(mMapState.mCameraSubject->mYPos / mMapState.kCameraBlockSize.y);

        const glm::vec2 camPos = glm::vec2(
            (camX * mMapState.kCameraBlockSize.x) + mMapState.kCameraBlockImageOffset.x,
            (camY * mMapState.kCameraBlockSize.y) + mMapState.kCameraBlockImageOffset.y) + glm::vec2(mMapState.kVirtualScreenSize.x / 2, mMapState.kVirtualScreenSize.y / 2);

        if (mMapState.mCameraPosition != camPos)
        {
            LOG_INFO("TODO: Screen change");
            coords.mSmoothCameraPosition = false;
            mMapState.mCameraPosition = camPos;
        }

        coords.SetCameraPosition(mMapState.mCameraPosition);
    }
}


void GameMode::Render(Renderer& rend, GuiContext& gui) const
{
    if (Debugging().mShowDebugUi)
    {
        // Debug ui
        gui_begin_window(&gui, "Script debug");
        if (gui_button(&gui, "Reload abe script"))
        {
            // TODO: Debug hack
            //const_cast<MapObject&>(mPlayer).ReloadScript();
        }
        gui_end_window(&gui);
    }

    if (mMapState.mCameraSubject)
    {
        const s32 camX = static_cast<s32>(mMapState.mCameraSubject->mXPos / mMapState.kCameraBlockSize.x);
        const s32 camY = static_cast<s32>(mMapState.mCameraSubject->mYPos / mMapState.kCameraBlockSize.y);


        // Culling is disabled until proper camera position updating order is fixed
        // ^ not sure what this means, but rendering things at negative cam index seems to go wrong
        if (camX >= 0 && camY >= 0 &&
            camX < static_cast<s32>(mMapState.mScreens.size()) &&
            camY < static_cast<s32>(mMapState.mScreens[camX].size()))
        {
            GridScreen* screen = mMapState.mScreens[camX][camY].get();
            if (screen->hasTexture())
            {
                screen->Render(
                    (camX * mMapState.kCameraBlockSize.x) + mMapState.kCameraBlockImageOffset.x,
                    (camY * mMapState.kCameraBlockSize.y) + mMapState.kCameraBlockImageOffset.y,
                    mMapState.kVirtualScreenSize.x, mMapState.kVirtualScreenSize.y);
            }
        }
    }

    mMapState.RenderDebug(rend);

    for (const auto& obj : mMapState.mObjs)
    {
        obj->Render(rend, gui, 0, 0, 1.0f, Renderer::eForegroundLayer0);
    }

    if (mMapState.mCameraSubject)
    {
        // Test raycasting for shadows
        mMapState.DebugRayCast(rend,
            glm::vec2(mMapState.mCameraSubject->mXPos, mMapState.mCameraSubject->mYPos),
            glm::vec2(mMapState.mCameraSubject->mXPos, mMapState.mCameraSubject->mYPos + 500),
            0,
            glm::vec2(0, -10)); // -10 so when we are *ON* a line you can see something

        mMapState.DebugRayCast(rend,
            glm::vec2(mMapState.mCameraSubject->mXPos, mMapState.mCameraSubject->mYPos - 2),
            glm::vec2(mMapState.mCameraSubject->mXPos, mMapState.mCameraSubject->mYPos - 60),
            3,
            glm::vec2(0, 0));

        if (mMapState.mCameraSubject->mFlipX)
        {
            mMapState.DebugRayCast(rend,
                glm::vec2(mMapState.mCameraSubject->mXPos, mMapState.mCameraSubject->mYPos - 20),
                glm::vec2(mMapState.mCameraSubject->mXPos - 25, mMapState.mCameraSubject->mYPos - 20), 1);

            mMapState.DebugRayCast(rend,
                glm::vec2(mMapState.mCameraSubject->mXPos, mMapState.mCameraSubject->mYPos - 50),
                glm::vec2(mMapState.mCameraSubject->mXPos - 25, mMapState.mCameraSubject->mYPos - 50), 1);
        }
        else
        {
            mMapState.DebugRayCast(rend,
                glm::vec2(mMapState.mCameraSubject->mXPos, mMapState.mCameraSubject->mYPos - 20),
                glm::vec2(mMapState.mCameraSubject->mXPos + 25, mMapState.mCameraSubject->mYPos - 20), 2);

            mMapState.DebugRayCast(rend,
                glm::vec2(mMapState.mCameraSubject->mXPos, mMapState.mCameraSubject->mYPos - 50),
                glm::vec2(mMapState.mCameraSubject->mXPos + 25, mMapState.mCameraSubject->mYPos - 50), 2);
        }
    }
}
