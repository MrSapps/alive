#include "gamemode.hpp"
#include "engine.hpp"
#include "debug.hpp"
#include "world.hpp"
#include "gridmap.hpp"
#include "fmv.hpp"

#include "core/systems/debugsystem.hpp"
#include "core/systems/inputsystem.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/systems/resourcesystem.hpp"
#include "core/systems/collisionsystem.hpp"

#include "core/components/cameracomponent.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

// TODO: Add to common header with same func from fmv.cpp
static f32 Percent2(f32 max, f32 percent)
{
    return (max / 100.0f) * percent;
}

GameMode::GameMode(World& mapState) : mWorld(mapState)
{

}

void GameMode::UpdateMenu(const InputReader& /*input*/, CoordinateSpace& /*coords*/)
{
    const auto cameraSystem = mWorld.mEntityManager.GetSystem<CameraSystem>();
    switch (mMenuState)
    {
    case GameMode::MenuStates::eInit:
        mMenuState = MenuStates::eCameraRoll;
        mWorld.SetCurrentGridScreenFromCAM("STP01C25.CAM");
        cameraSystem->SetGameCameraToCameraAt(mWorld.CurrentGridScreenX(), mWorld.CurrentGridScreenY());
        break;
    case GameMode::MenuStates::eCameraRoll:
        if ((mWorld.mGlobalFrameCounter % 100) == 0)
        {
            mWorld.mPlayFmvState->Play("AE_Intro");
            mWorld.mReturnToState = mWorld.mState;
            mWorld.mState = World::States::ePlayFmv;
        }
        break;
    case GameMode::MenuStates::eFmv:
        break;
    case GameMode::MenuStates::eUserMenu:
        break;
    }

}

void GameMode::Update(const InputReader& input, CoordinateSpace& coords)
{
    const auto cameraSystem = mWorld.mEntityManager.GetSystem<CameraSystem>();

    coords.SetScreenSize(cameraSystem->mVirtualScreenSize);

    if (mState == eMenu)
    {
        UpdateMenu(input, coords);
        coords.SetCameraPosition(cameraSystem->mCameraPosition);
        return;
    }

    if (mState == eRunning)
    {
        // TODO: This needs to be arranged to match the real game which has an ordering of:
        // TODO: Update all -> Animate all -> Render all ->  LoadResources/SwitchMap -> ReadInput -> update gnFrame value

        // Physics System
        mWorld.mEntityManager.With<PhysicsComponent, TransformComponent>([](auto, auto physics, auto transform)
        {
            transform->Add(physics->GetXSpeed(), physics->GetYSpeed());
        });
        // Animation System
        mWorld.mEntityManager.With<AnimationComponent>([](auto, auto animation)
        {
            animation->Update();
        });
        // Abe system
        mWorld.mEntityManager.With<AbePlayerControllerComponent, AbeMovementComponent>([](auto, auto controller, auto abe)
        {
            controller->Update();
            abe->Update();
        });
        // Slig system
        mWorld.mEntityManager.With<SligPlayerControllerComponent, SligMovementComponent>([](auto, auto controller, auto slig)
        {
            controller->Update();
            slig->Update();
        });
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
		mWorld.mState = World::States::eToEditor;
		coords.mSmoothCameraPosition = true;

		mWorld.mModeSwitchTimeout = SDL_GetTicks() + kSwitchTimeMs;

		//mCameraPosition.x = mPlayer.mXPos;
		//mCameraPosition.y = mPlayer.mYPos;
	}

	// Input system
	mWorld.mEntityManager.GetSystem<InputSystem>()->Update();

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
    const auto cameraSystem = mWorld.mEntityManager.GetSystem<CameraSystem>();
    if (cameraSystem->mTarget && Debugging().mDrawCameras)
    {
        auto pos = cameraSystem->mTarget.GetComponent<TransformComponent>();

        const s32 camX = mState == eMenu ? static_cast<s32>(mWorld.CurrentGridScreenX()) : static_cast<s32>(pos->GetX() / cameraSystem->mCameraBlockSize.x);
        const s32 camY = mState == eMenu ? static_cast<s32>(mWorld.CurrentGridScreenY()) : static_cast<s32>(pos->GetY() / cameraSystem->mCameraBlockSize.y);

        if (camX >= 0 && camY >= 0 &&
            camX < static_cast<s32>(mWorld.mScreens.size()) &&
            camY < static_cast<s32>(mWorld.mScreens[camX].size()))
        {
            GridScreen* screen = mWorld.mScreens[camX][camY].get();
            if (screen)
            {
                if (screen->HasTexture())
                {
                    screen->Render(rend,
                        (camX * cameraSystem->mCameraBlockSize.x) + cameraSystem->mCameraBlockImageOffset.x,
                        (camY * cameraSystem->mCameraBlockSize.y) + cameraSystem->mCameraBlockImageOffset.y,
                        cameraSystem->mVirtualScreenSize.x, cameraSystem->mVirtualScreenSize.y);
                }
            }
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

    // Debug system render
    mWorld.mEntityManager.GetSystem<DebugSystem>()->Render(rend);
    // Animation system render
    mWorld.mEntityManager.With<AnimationComponent>([&rend](auto, auto animation)
    {
        animation->Render(rend);
    });
}

GameMode::GameModeStates GameMode::State() const
{
    return mState;
}