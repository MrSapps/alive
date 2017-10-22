#include "gamemode.hpp"
#include "engine.hpp"
#include "debug.hpp"
#include "world.hpp"
#include "gridmap.hpp"

GameMode::GameMode(WorldState& mapState)
    : mWorldState(mapState)
{

}

void GameMode::Update(const InputState& input, CoordinateSpace& coords)
{
    /*
    if (Debugging().mShowDebugUi)
    {
        // Debug ui
        if (ImGui::Begin("Script debug"))
        {
            if (ImGui::Button("Reload abe script"))
            {
                // TODO: Debug hack
                //const_cast<MapObject&>(mPlayer).ReloadScript();
            }
        }
        ImGui::End();
    }*/

    if (input.mKeys[SDL_SCANCODE_E].IsPressed())
    {
        mWorldState.mState = WorldState::States::eToEditor;
        coords.mSmoothCameraPosition = true;

        mWorldState.mModeSwitchTimeout = SDL_GetTicks() + kSwitchTimeMs;

        //mCameraPosition.x = mPlayer.mXPos;
        //mCameraPosition.y = mPlayer.mYPos;
    }

    coords.SetScreenSize(mWorldState.kVirtualScreenSize);


    for (auto& obj : mWorldState.mObjs)
    {
        obj->Update(input);
    }


    if (mWorldState.mCameraSubject)
    {
        const s32 camX = static_cast<s32>(mWorldState.mCameraSubject->mXPos / mWorldState.kCameraBlockSize.x);
        const s32 camY = static_cast<s32>(mWorldState.mCameraSubject->mYPos / mWorldState.kCameraBlockSize.y);

        const glm::vec2 camPos = glm::vec2(
            (camX * mWorldState.kCameraBlockSize.x) + mWorldState.kCameraBlockImageOffset.x,
            (camY * mWorldState.kCameraBlockSize.y) + mWorldState.kCameraBlockImageOffset.y) + glm::vec2(mWorldState.kVirtualScreenSize.x / 2, mWorldState.kVirtualScreenSize.y / 2);

        if (mWorldState.mCameraPosition != camPos)
        {
            LOG_INFO("TODO: Screen change");
            coords.mSmoothCameraPosition = false;
            mWorldState.mCameraPosition = camPos;
        }

        coords.SetCameraPosition(mWorldState.mCameraPosition);
    }
}


void GameMode::Render(AbstractRenderer& rend) const
{
    if (mWorldState.mCameraSubject && Debugging().mDrawCameras)
    {
        const s32 camX = static_cast<s32>(mWorldState.mCameraSubject->mXPos / mWorldState.kCameraBlockSize.x);
        const s32 camY = static_cast<s32>(mWorldState.mCameraSubject->mYPos / mWorldState.kCameraBlockSize.y);


        // Culling is disabled until proper camera position updating order is fixed
        // ^ not sure what this means, but rendering things at negative cam index seems to go wrong
        if (camX >= 0 && camY >= 0 &&
            camX < static_cast<s32>(mWorldState.mScreens.size()) &&
            camY < static_cast<s32>(mWorldState.mScreens[camX].size()))
        {
            GridScreen* screen = mWorldState.mScreens[camX][camY].get();
            if (screen)
            {
                if (screen->hasTexture())
                {
                    screen->Render(rend,
                        (camX * mWorldState.kCameraBlockSize.x) + mWorldState.kCameraBlockImageOffset.x,
                        (camY * mWorldState.kCameraBlockSize.y) + mWorldState.kCameraBlockImageOffset.y,
                        mWorldState.kVirtualScreenSize.x, mWorldState.kVirtualScreenSize.y);
                }
            }
        }
    }

    mWorldState.RenderDebug(rend);

    if (Debugging().mDrawObjects)
    {
        for (const auto& obj : mWorldState.mObjs)
        {
            obj->Render(rend, 0, 0, 1.0f, AbstractRenderer::eForegroundLayer0);
        }
    }

    if (mWorldState.mCameraSubject)
    {
        // Test raycasting for shadows
        mWorldState.DebugRayCast(rend,
            glm::vec2(mWorldState.mCameraSubject->mXPos, mWorldState.mCameraSubject->mYPos),
            glm::vec2(mWorldState.mCameraSubject->mXPos, mWorldState.mCameraSubject->mYPos + 500),
            0,
            glm::vec2(0, -10)); // -10 so when we are *ON* a line you can see something

        mWorldState.DebugRayCast(rend,
            glm::vec2(mWorldState.mCameraSubject->mXPos, mWorldState.mCameraSubject->mYPos - 2),
            glm::vec2(mWorldState.mCameraSubject->mXPos, mWorldState.mCameraSubject->mYPos - 60),
            3,
            glm::vec2(0, 0));

        if (mWorldState.mCameraSubject->mFlipX)
        {
            mWorldState.DebugRayCast(rend,
                glm::vec2(mWorldState.mCameraSubject->mXPos, mWorldState.mCameraSubject->mYPos - 20),
                glm::vec2(mWorldState.mCameraSubject->mXPos - 25, mWorldState.mCameraSubject->mYPos - 20), 1);

            mWorldState.DebugRayCast(rend,
                glm::vec2(mWorldState.mCameraSubject->mXPos, mWorldState.mCameraSubject->mYPos - 50),
                glm::vec2(mWorldState.mCameraSubject->mXPos - 25, mWorldState.mCameraSubject->mYPos - 50), 1);
        }
        else
        {
            mWorldState.DebugRayCast(rend,
                glm::vec2(mWorldState.mCameraSubject->mXPos, mWorldState.mCameraSubject->mYPos - 20),
                glm::vec2(mWorldState.mCameraSubject->mXPos + 25, mWorldState.mCameraSubject->mYPos - 20), 2);

            mWorldState.DebugRayCast(rend,
                glm::vec2(mWorldState.mCameraSubject->mXPos, mWorldState.mCameraSubject->mYPos - 50),
                glm::vec2(mWorldState.mCameraSubject->mXPos + 25, mWorldState.mCameraSubject->mYPos - 50), 2);
        }
    }
}
