#include "gamemode.hpp"
#include "engine.hpp"
#include "debug.hpp"
#include "world.hpp"
#include "gridmap.hpp"
#include "fmv.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"

// TODO: Add to common header with same func from fmv.cpp
static f32 Percent2(f32 max, f32 percent)
{
    return (max / 100.0f) * percent;
}

GameMode::GameMode(WorldState& mapState)
    : mWorldState(mapState)
{

}

void GameMode::UpdateMenu(const InputReader& /*input*/, CoordinateSpace& /*coords*/)
{
    const auto cameraSystem = mWorldState.mEntityManager.GetSystem<CameraSystem>();
    switch (mMenuState)
    {
    case GameMode::MenuStates::eInit:
        mMenuState = MenuStates::eCameraRoll;
        mWorldState.SetCurrentCamera("STP01C25.CAM");
        cameraSystem->SetGameCameraToCameraAt(mWorldState.CurrentCameraX(), mWorldState.CurrentCameraY());
        break;

    case GameMode::MenuStates::eCameraRoll:
        if ((mWorldState.mGlobalFrameCounter % 100) == 0)
        {
            mWorldState.mPlayFmvState->Play("AE_Intro");
            mWorldState.mReturnToState = mWorldState.mState;
            mWorldState.mState = WorldState::States::ePlayFmv;
        }
        break;

    case GameMode::MenuStates::eFmv:
        break;

    case GameMode::MenuStates::eUserMenu:
        break;

    default:
        break;
    }

}

void GameMode::Update(const InputReader& input, CoordinateSpace& coords)
{
	const auto cameraSystem = mWorldState.mEntityManager.GetSystem<CameraSystem>();

    coords.SetScreenSize(cameraSystem->mVirtualScreenSize);

    if (mState == eMenu)
    {
        UpdateMenu(input, coords);
        coords.SetCameraPosition(cameraSystem->mCameraPosition);
        return;
    }

    if (mState == eRunning)
    {
        if (input.KeyboardKey(SDL_SCANCODE_ESCAPE).Pressed())
        {
            // TODO: Stop or pause music? Check what real game does
            mState = ePaused;
        }
    }
    else if (mState == ePaused)
    {
        if (input.KeyboardKey(SDL_SCANCODE_ESCAPE).Pressed())
        {
            mState = eRunning;
        }
    }

    if (input.KeyboardKey(SDL_SCANCODE_E).Pressed() && mState != ePaused)
    {
        mWorldState.mState = WorldState::States::eToEditor;
        coords.mSmoothCameraPosition = true;

        mWorldState.mModeSwitchTimeout = SDL_GetTicks() + kSwitchTimeMs;

        //mCameraPosition.x = mPlayer.mXPos;
        //mCameraPosition.y = mPlayer.mYPos;
    }

    if (mState == eRunning)
    {
        /*
        for (auto& obj : mWorldState.mObjs)
        {
            obj->Update(input);
        }
        */
    }

    if (cameraSystem->mTarget)
    {
        auto pos = cameraSystem->mTarget.GetComponent<TransformComponent>();

        const s32 camX = static_cast<s32>(pos->GetX() / cameraSystem->mCameraBlockSize.x);
        const s32 camY = static_cast<s32>(pos->GetY() / cameraSystem->mCameraBlockSize.y);

        const glm::vec2 camPos = glm::vec2(
            (camX * cameraSystem->mCameraBlockSize.x) + cameraSystem->mCameraBlockImageOffset.x,
            (camY * cameraSystem->mCameraBlockSize.y) + cameraSystem->mCameraBlockImageOffset.y) + glm::vec2(cameraSystem->mVirtualScreenSize.x / 2, cameraSystem->mVirtualScreenSize.y / 2);

        if (cameraSystem->mCameraPosition != camPos)
        {
            LOG_INFO("TODO: Screen change");
            coords.mSmoothCameraPosition = false;
            cameraSystem->mCameraPosition = camPos;
        }

        coords.SetCameraPosition(camPos);
    }
}

void GameMode::Render(AbstractRenderer& rend) const
{
    const auto cameraSystem = mWorldState.mEntityManager.GetSystem<CameraSystem>();
    if (cameraSystem->mTarget && Debugging().mDrawCameras)
    {
        auto pos = cameraSystem->mTarget.GetComponent<TransformComponent>();

        const s32 camX = mState == eMenu ? static_cast<s32>(mWorldState.CurrentCameraX()) : static_cast<s32>(pos->GetX() / cameraSystem->mCameraBlockSize.x);
        const s32 camY = mState == eMenu ? static_cast<s32>(mWorldState.CurrentCameraY()) : static_cast<s32>(pos->GetY() / cameraSystem->mCameraBlockSize.y);

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
                        (camX * cameraSystem->mCameraBlockSize.x) + cameraSystem->mCameraBlockImageOffset.x,
                        (camY * cameraSystem->mCameraBlockSize.y) + cameraSystem->mCameraBlockImageOffset.y,
                        cameraSystem->mVirtualScreenSize.x, cameraSystem->mVirtualScreenSize.y);
                }
            }
        }
    }

    mWorldState.RenderDebug(rend);

    if (cameraSystem->mTarget)
    {
        auto pos = cameraSystem->mTarget.GetComponent<TransformComponent>();
        auto anim = cameraSystem->mTarget.GetComponent<AnimationComponent>();

        // Test raycasting for shadows
        mWorldState.DebugRayCast(rend,
            glm::vec2(pos->GetX(), pos->GetY()),
            glm::vec2(pos->GetX(), pos->GetY() + 500),
            0,
            glm::vec2(0, -10)); // -10 so when we are *ON* a line you can see something

        mWorldState.DebugRayCast(rend,
            glm::vec2(pos->GetX(), pos->GetY() - 2),
            glm::vec2(pos->GetX(), pos->GetY() - 60),
            3,
            glm::vec2(0, 0));

        if (anim->mFlipX)
        {
            mWorldState.DebugRayCast(rend,
                glm::vec2(pos->GetX(), pos->GetY() - 20),
                glm::vec2(pos->GetX() - 25, pos->GetY() - 20), 1);

            mWorldState.DebugRayCast(rend,
                glm::vec2(pos->GetX(), pos->GetY() - 50),
                glm::vec2(pos->GetX() - 25, pos->GetY() - 50), 1);
        }
        else
        {
            mWorldState.DebugRayCast(rend,
                glm::vec2(pos->GetX(), pos->GetY() - 20),
                glm::vec2(pos->GetX() + 25, pos->GetY() - 20), 2);

            mWorldState.DebugRayCast(rend,
                glm::vec2(pos->GetX(), pos->GetY() - 50),
                glm::vec2(pos->GetX() + 25, pos->GetY() - 50), 2);
        }
    }

    if (mState == ePaused)
    {
        rend.PathBegin();
        f32 w = static_cast<f32>(rend.Width());
        f32 h = static_cast<f32>(rend.Height());

        rend.PathLineTo(0.0f, 0.0f);
        rend.PathLineTo(0.0f, h);
        rend.PathLineTo(w, h);
        rend.PathLineTo(w, 0.0f);
        rend.PathFill(ColourU8{ 0, 0, 0, 200 }, AbstractRenderer::eLayers::ePauseMenu, AbstractRenderer::eNormal, AbstractRenderer::eScreen);

        f32 textSize = Percent2(h, 8.0f);
        f32 bounds[4] = {};
        rend.TextBounds((w / 2.0f), (h / 2.0f), textSize, "Paused", bounds);
        rend.Text(
            (w / 2.0f) - ((bounds[2] - bounds[0]) / 2.0f),
            (h / 2.0f) - ((bounds[3] - bounds[1]) / 2.0f),
            textSize, "Paused", ColourU8{ 255, 255, 255, 255 }, AbstractRenderer::eLayers::ePauseMenu, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
    }
}

GameMode::GameModeStates GameMode::State() const
{
    return mState;
}