#include "core/systems/inputsystem.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/systems/collisionsystem.hpp"
#include "core/systems/resourcelocatorsystem.hpp"
#include "core/components/physicscomponent.hpp"

#include "core/components/cameracomponent.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

#include "oddlib/bits_factory.hpp"
#include "gridmap.hpp"
#include "engine.hpp"
#include "sound.hpp"
#include "world.hpp"
#include "fmv.hpp"

GridScreen::GridScreen(const Oddlib::Path::Camera& camera, ResourceLocator& locator)
    : mFileName(camera.mName), mCamera(camera), mLocator(locator)
{

}

GridScreen::~GridScreen()
{
    assert(mCameraTexture.IsValid() == false);
    assert(mFG1Texture.IsValid() == false);
}

void GridScreen::LoadTextures(AbstractRenderer& rend)
{
    if (!mCameraTexture.IsValid())
    {
        mCam = mLocator.LocateCamera(mFileName).get();
        if (mCam) // One path trys to load BRP08C10.CAM which exists in no data sets anywhere!
        {
            SDL_Surface* surf = mCam->GetSurface();
            mCameraTexture = rend.CreateTexture(AbstractRenderer::eTextureFormats::eRGB, surf->w, surf->h, AbstractRenderer::eTextureFormats::eRGB, surf->pixels, true);

            if (!mFG1Texture.IsValid())
            {
                if (mCam->GetFg1())
                {
                    SDL_Surface* fg1Surf = mCam->GetFg1()->GetSurface();
                    if (fg1Surf)
                    {
                        mFG1Texture = rend.CreateTexture(AbstractRenderer::eTextureFormats::eRGBA, fg1Surf->w, fg1Surf->h, AbstractRenderer::eTextureFormats::eRGBA, fg1Surf->pixels, true);
                    }
                }
            }
        }
    }
}

void GridScreen::UnLoadTextures(AbstractRenderer& rend)
{
    if (mCameraTexture.IsValid())
    {
        rend.DestroyTexture(mCameraTexture);
        mCameraTexture.mData = nullptr;
    }
    if (mFG1Texture.IsValid())
    {
        rend.DestroyTexture(mFG1Texture);
        mFG1Texture.mData = nullptr;
    }
}

bool GridScreen::hasTexture() const
{
    bool onlySpaces = true;
    for (size_t i = 0; i < mFileName.size(); ++i)
    {
        if (mFileName[i] != ' ' && mFileName[i] != '\0')
        {
            onlySpaces = false;
            break;
        }
    }
    return !onlySpaces;
}

void GridScreen::Render(AbstractRenderer& rend, float x, float y, float w, float h)
{
    LoadTextures(rend);

    if (mCameraTexture.IsValid())
    {
        rend.TexturedQuad(mCameraTexture, x, y, w, h, AbstractRenderer::eForegroundLayer0, ColourU8{ 255, 255, 255, 255 });
    }

    if (mFG1Texture.IsValid())
    {
        rend.TexturedQuad(mFG1Texture, x, y, w, h, AbstractRenderer::eForegroundLayer1, ColourU8{ 255, 255, 255, 255 });
    }
}

GridMap::GridMap(CoordinateSpace& coords, WorldState& state, EntityManager &entityManager) : mRoot(entityManager), mLoader(*this), mWorldState(state)
{
    auto cameraSystem = mRoot.GetSystem<CameraSystem>();

    // Set up the screen size and camera pos so that the grid is drawn correctly during init
    cameraSystem->mVirtualScreenSize = glm::vec2(368.0f, 240.0f);
    cameraSystem->mCameraBlockSize = glm::vec2(375.0f, 260.0f);
    cameraSystem->mCamGapSize = glm::vec2(375.0f, 260.0f);
    cameraSystem->mCameraBlockImageOffset = glm::vec2(0.0f, 0.0f);

    coords.SetScreenSize(cameraSystem->mVirtualScreenSize);

    const int camX = 0;
    const int camY = 0;
    const glm::vec2 camPos = glm::vec2(
        (camX * cameraSystem->mCameraBlockSize.x) + cameraSystem->mCameraBlockImageOffset.x,
        (camY * cameraSystem->mCameraBlockSize.y) + cameraSystem->mCameraBlockImageOffset.y) +
        glm::vec2(cameraSystem->mVirtualScreenSize.x / 2, cameraSystem->mVirtualScreenSize.y / 2);

    cameraSystem->mCameraPosition = camPos;
    coords.SetCameraPosition(camPos);
}

GridMap::~GridMap()
{
    TRACE_ENTRYEXIT;
}

bool GridMap::LoadMap(const Oddlib::Path& path, ResourceLocator& locator)
{
#if defined(_DEBUG)
    while (!mLoader.Load(path, locator))
    {

    }
    return true;
#else
    return mLoader.Load(path, locator);
#endif
}

void GridMap::UnloadMap(AbstractRenderer& renderer)
{
    for (auto x = 0u; x < mWorldState.mScreens.size(); x++)
    {
        for (auto y = 0u; y < mWorldState.mScreens[x].size(); y++)
        {
            GridScreen* screen = mWorldState.mScreens[x][y].get();
            if (!screen)
            {
                continue;
            }
            screen->UnLoadTextures(renderer);
        }
    }
    auto collisionSystem = mRoot.GetSystem<CollisionSystem>();
    if (collisionSystem)
    {
        for (auto &entity : mRoot.mEntities)
        {
            entity->Destroy();
        }
        mRoot.DestroyEntities();
        collisionSystem->Clear();
    }
    mWorldState.mScreens.clear();
}

GridMap::Loader::Loader(GridMap& gm)
    : mGm(gm)
{

}

void GridMap::Loader::SetupAndConvertCollisionItems(const Oddlib::Path& path)
{
    auto cameraSystem = mGm.mRoot.GetSystem<CameraSystem>();
    auto collisionSystem = mGm.mRoot.GetSystem<CollisionSystem>();

    // The "block" or grid square that a camera fits into, it never usually fills the grid
    cameraSystem->mCameraBlockSize = (path.IsAo()) ? glm::vec2(1024, 480) : glm::vec2(375, 260);
    cameraSystem->mCamGapSize = (path.IsAo()) ? glm::vec2(1024, 480) : glm::vec2(375, 260);

    // Since the camera won't fill a block it can be offset so the camera image is in the middle
    // of the block or else where.
    cameraSystem->mCameraBlockImageOffset = (path.IsAo()) ? glm::vec2(257, 114) : glm::vec2(0, 0);

    // Clear out existing collisions from previous map
    collisionSystem->Clear();
    // Convert collisions items from AO/AE to ALIVE format
    collisionSystem->ConvertCollisionItems(path.CollisionItems());

    // Move to next state
    SetState(LoaderStates::eAllocateCameraMemory);
}

void GridMap::Loader::HandleAllocateCameraMemory(const Oddlib::Path& path)
{
    mGm.mWorldState.mScreens.resize(path.XSize());
    for (auto& col : mGm.mWorldState.mScreens)
    {
        col.resize(path.YSize());
    }
    SetState(LoaderStates::eLoadCameras);
}

void GridMap::Loader::HandleLoadCameras(const Oddlib::Path& path, ResourceLocator& locator)
{
    if (mXForLoop.IterateTimeBoxedIf(kMaxExecutionTimeMs, path.XSize(), [&]()
    {
        return mYForLoop.Iterate(path.YSize(), [&]()
        {
            mGm.mWorldState.mScreens[mXForLoop.Value()][mYForLoop.Value()] = std::make_unique<GridScreen>(path.CameraByPosition(mXForLoop.Value(), mYForLoop.Value()), locator);
        });
    }))
    {
        SetState(LoaderStates::eLoadEntities);
    }
}

void GridMap::Loader::HandleLoadEntities(const Oddlib::Path& path)
{
    if (mXForLoop.IterateIf(path.XSize(), [&]()
    {
        return mYForLoop.IterateIf(path.YSize(), [&]()
        {
            GridScreen* screen = mGm.mWorldState.mScreens[mXForLoop.Value()][mYForLoop.Value()].get();
            const Oddlib::Path::Camera& cam = screen->getCamera();
            return mIForLoop.Iterate(static_cast<u32>(cam.mObjects.size()), [&]()
            {
                const auto& object = cam.mObjects[mIForLoop.Value()];
                Oddlib::MemoryStream ms(std::vector<u8>(object.mData.data(), object.mData.data() + object.mData.size()));
                switch (object.mType)
                {
                case ObjectTypesAe::eContinuePoint:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // saveFileId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eHoist:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // hoistType
                    ReadU16(ms); // edgeType
                    ReadU16(ms); // id
                    ReadU16(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eDoor:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // level
                    ReadU16(ms); // path
                    ReadU16(ms); // camera
                    ReadU16(ms); // scale
                    ReadU16(ms); // doorNumber
                    ReadU16(ms); // id
                    ReadU16(ms); // targetDoorNumber
                    ReadU16(ms); // skin
                    ReadU16(ms); // startOpen
                    ReadU16(ms); // hubId1
                    ReadU16(ms); // hubId2
                    ReadU16(ms); // hubId3
                    ReadU16(ms); // hubId4
                    ReadU16(ms); // hubId5
                    ReadU16(ms); // hubId6
                    ReadU16(ms); // hubId7
                    ReadU16(ms); // hubId8
                    ReadU16(ms); // wipeEffect
                    auto xoffset = ReadU16(ms); // xoffset
                    auto yoffset = ReadU16(ms); // yoffset
                    ReadU16(ms); // wipeXStart
                    ReadU16(ms); // wipeYStart
                    ReadU16(ms); // abeFaceLeft
                    ReadU16(ms); // closeAfterUse
                    ReadU16(ms); // removeThrowables
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + xoffset + 5, static_cast<float>(object.mRectTopLeft.mY) + object.mRectBottomRight.mY - static_cast<float>(object.mRectTopLeft.mY) + yoffset);
                    entity->GetComponent<AnimationComponent>()->Change("DoorClosed_Barracks");
                    break;
                }
                case ObjectTypesAe::eShadow:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // centerW
                    ReadU16(ms); // centerH
                    ReadU8(ms);  // r1
                    ReadU8(ms);  // g1
                    ReadU8(ms);  // b1
                    ReadU8(ms);  // r2
                    ReadU8(ms);  // g2
                    ReadU8(ms);  // b2
                    ReadU16(ms); // id
                    ReadU32(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eLiftPoint:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // id
                    ReadU16(ms); // isStartPosition
                    ReadU16(ms); // liftType
                    ReadU16(ms); // liftStopType
                    ReadU16(ms); // scale
                    ReadU16(ms); // ignoreLiftMover
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eWellLocal:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // triggerId
                    ReadU16(ms); // wellId
                    ReadU16(ms); // resourceId
                    ReadU16(ms); // offDeltaX
                    ReadU16(ms); // offDeltaY
                    ReadU16(ms); // onDeltaX
                    ReadU16(ms); // onDeltaY
                    ReadU16(ms); // leavesWhenOn
                    ReadU16(ms); // leafX
                    ReadU32(ms); // leafY
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eDove:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // numberOfBirds
                    ReadU16(ms); // pixelPerfect
                    ReadU16(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("DOVBASIC.BAN_60_AePc_2");
                    break;
                }
                case ObjectTypesAe::eRockSack:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // side
                    ReadU16(ms); // xVelocity
                    ReadU16(ms); // yVelocity
                    ReadU16(ms); // scale
                    ReadU16(ms); // numberOfRocks
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("MIP04C01.CAM_1002_AePc_2");
                    break;
                }
                case ObjectTypesAe::eFallingItem:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // id
                    ReadU16(ms); // scale
                    ReadU16(ms); // delayTime
                    ReadU16(ms); // numberOfItems
                    ReadU16(ms); // resetId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("FALLBONZ.BAN_2007_AePc_0");
                    break;
                }
                case ObjectTypesAe::ePullRingRope:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // id
                    ReadU16(ms); // targetAction
                    ReadU16(ms); // lengthOfRope
                    ReadU16(ms); // scale
                    ReadU16(ms); // onSound
                    ReadU16(ms); // offSound
                    ReadU32(ms); // soundDirection
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("PULLRING.BAN_1014_AePc_1");
                    break;
                }
                case ObjectTypesAe::eBackgroundAnimation:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    auto animId = ReadU32(ms);
                    switch (animId)
                    {
                    case 1201:
                        entity->GetComponent<AnimationComponent>()->Change("BAP01C06.CAM_1201_AePc_0");
                        entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + 5, static_cast<float>(object.mRectTopLeft.mY) + 3);
                        break;
                    case 1202:
                        entity->GetComponent<AnimationComponent>()->Change("FARTFAN.BAN_1202_AePc_0");
                        break;
                    default:
                        entity->GetComponent<AnimationComponent>()->Change("AbeStandSpeak1");
                    }
                    break;
                }
                case ObjectTypesAe::eTimedMine:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // id
                    ReadU16(ms); // state
                    ReadU16(ms); // scale
                    ReadU16(ms); // ticksBeforeExplode
                    ReadU32(ms); // disableResources
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("BOMB.BND_1005_AePc_1");
                    break;
                }
                case ObjectTypesAe::eSlig:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // startState
                    ReadU16(ms); // pauseTime
                    ReadU16(ms); // pauseLeftMin
                    ReadU16(ms); // pauseLeftMax
                    ReadU16(ms); // pauseRightMin
                    ReadU16(ms); // pauseRightMax
                    ReadU16(ms); // chalNumber
                    ReadU16(ms); // chalTimer
                    ReadU16(ms); // numberOfShots
                    ReadU16(ms); // [unknown]
                    ReadU16(ms); // code1
                    ReadU16(ms); // code2
                    ReadU16(ms); // chaseAbe
                    ReadU16(ms); // startDirection
                    ReadU16(ms); // panicTimeout
                    ReadU16(ms); // numberofPanicSounds
                    ReadU16(ms); // panicSoundTimeout
                    ReadU16(ms); // stopChaseDelay
                    ReadU16(ms); // timeBeforeChase
                    ReadU16(ms); // sligId
                    ReadU16(ms); // listenTime
                    ReadU16(ms); // chanceToSayWhat
                    ReadU16(ms); // chanceToBeatMud
                    ReadU16(ms); // talkToAbe
                    ReadU16(ms); // dontShoot
                    ReadU16(ms); // zShootDelay
                    ReadU16(ms); // stayAwake
                    ReadU16(ms); // disableResources
                    ReadU16(ms); // noiseWakeDistance
                    ReadU32(ms); // id
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("SligStandIdle");
                    break;
                }
                case ObjectTypesAe::eSlog:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // direction
                    ReadU16(ms); // asleep
                    ReadU16(ms); // wakeUpAngry
                    ReadU16(ms); // barkAnger
                    ReadU16(ms); // chaseAnger
                    ReadU16(ms); // jumpDelay
                    ReadU16(ms); // disableResources
                    ReadU16(ms); // angryId
                    ReadU16(ms); // boneEatingTime
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("SLOG.BND_570_AePc_3");
                    break;
                }
                case ObjectTypesAe::eSwitch:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU32(ms); // scale
                    ReadU16(ms); // onSound
                    ReadU16(ms); // offSound
                    ReadU16(ms); // soundDirection
                    ReadU16(ms); // id
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + 37, static_cast<float>(object.mRectTopLeft.mY) + object.mRectBottomRight.mY - static_cast<float>(object.mRectTopLeft.mY) - 5);
                    entity->GetComponent<AnimationComponent>()->Change("SwitchIdle");
                    break;
                }
                case ObjectTypesAe::eSecurityEye:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU32(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("MAIMORB.BAN_2006_AePc_0");
                    break;
                }
                case ObjectTypesAe::ePulley:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU32(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eAbeStart:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU32(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eWellExpress:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // id
                    ReadU16(ms); // wellId
                    ReadU16(ms); // resourceId
                    ReadU16(ms); // exitX
                    ReadU16(ms); // exitY
                    ReadU16(ms); // offLevel
                    ReadU16(ms); // offPath
                    ReadU16(ms); // offCamera
                    ReadU16(ms); // offWell
                    ReadU16(ms); // onLevel
                    ReadU16(ms); // onPath
                    ReadU16(ms); // onCamera
                    ReadU16(ms); // onWell
                    ReadU16(ms); // leavesWhenOn
                    ReadU16(ms); // leafX
                    ReadU16(ms); // leafY
                    ReadU16(ms); // movie
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eMine:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU32(ms); // Skip unused "num patterns"
                    ReadU32(ms); // Skip unused "patterns"
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + 10, static_cast<float>(object.mRectTopLeft.mY) + 22);
                    entity->GetComponent<AnimationComponent>()->Change("LANDMINE.BAN_1036_AePc_0");
                    break;
                }
                case ObjectTypesAe::eUxb:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // numberOfPatterns
                    ReadU16(ms); // pattern
                    ReadU16(ms); // scale
                    ReadU16(ms); // state
                    ReadU32(ms); // disableResources
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("TBOMB.BAN_1037_AePc_0");
                    break;
                }
                case ObjectTypesAe::eParamite:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // entrance
                    ReadU16(ms); // attackDelay
                    ReadU16(ms); // dropDelay
                    ReadU16(ms); // meatEatingTime
                    ReadU16(ms); // attackDuration
                    ReadU16(ms); // disableResources
                    ReadU16(ms); // id
                    ReadU16(ms); // hissBeforeAttack
                    ReadU16(ms); // deleteWhenFarAway
                    ReadU16(ms); // deadlyScratch
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("PARAMITE.BND_600_AePc_4");
                    break;
                }
                case ObjectTypesAe::eMovieStone:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // movieNumber
                    ReadU16(ms); // scale
                    ReadU16(ms); // id
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eBirdPortal:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // side
                    ReadU16(ms); // destinationLevel
                    ReadU16(ms); // destinationPath
                    ReadU16(ms); // destinationCamera
                    ReadU16(ms); // scale
                    ReadU16(ms); // movie
                    ReadU16(ms); // portalType
                    ReadU16(ms); // numberOfMudsForShrykll
                    ReadU16(ms); // createId
                    ReadU16(ms); // deleteId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::ePortalExit:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // side
                    ReadU16(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("PORTAL.BND_351_AePc_0");
                    break;
                }
                case ObjectTypesAe::eTrapDoor:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // id
                    ReadU16(ms); // startState
                    ReadU16(ms); // selfClosing
                    ReadU16(ms); // scale
                    ReadU16(ms); // destinationLevel
                    ReadU16(ms); // direction
                    ReadU16(ms); // animationOffset
                    ReadU16(ms); // openDuration
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("TRAPDOOR.BAN_1004_AePc_1");
                    break;
                }
                case ObjectTypesAe::eRollingBall:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // rollDirection
                    ReadU16(ms); // id
                    ReadU16(ms); // speed
                    ReadU32(ms); // acceleration
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eSligLeftBound:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // sligId
                    ReadU16(ms); // disableResources
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eInvisibleZone:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();

                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eFootSwitch:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // id
                    ReadU16(ms); // scale
                    ReadU16(ms); // action
                    ReadU16(ms); // triggerBy
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("TRIGGER.BAN_2010_AePc_0");
                    break;
                }
                case ObjectTypesAe::eSecurityOrb:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU32(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eMotionDetector:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // deviceX
                    ReadU16(ms); // deviceY
                    ReadU16(ms); // speedFactor (x256)
                    ReadU16(ms); // startOn
                    ReadU16(ms); // drawFlare
                    ReadU16(ms); // disableId
                    ReadU16(ms); // alarmId
                    ReadU16(ms); // alarmTicks
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("MOTION.BAN_6001_AePc_0");
                    break;
                }
                case ObjectTypesAe::eSligSpawner:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // startState
                    ReadU16(ms); // pauseLeftMin
                    ReadU16(ms); // pauseLeftMax
                    ReadU16(ms); // pauseRightMin
                    ReadU16(ms); // pauseRightMax
                    ReadU16(ms); // chalNumber
                    ReadU16(ms); // chalTimer
                    ReadU16(ms); // numberOfShots
                    ReadU16(ms); // [unknown]
                    ReadU16(ms); // code1
                    ReadU16(ms); // code2
                    ReadU16(ms); // chaseAbe
                    ReadU16(ms); // startDirection
                    ReadU16(ms); // panicTimeout
                    ReadU16(ms); // numberofPanicSounds
                    ReadU16(ms); // panicSoundTimeout
                    ReadU16(ms); // stopChaseDelay
                    ReadU16(ms); // timeBeforeChase
                    ReadU16(ms); // sligId
                    ReadU16(ms); // listenTime
                    ReadU16(ms); // chanceToSayWhat
                    ReadU16(ms); // chanceToBeatMud
                    ReadU16(ms); // talkToAbe
                    ReadU16(ms); // dontShoot
                    ReadU16(ms); // zShootDelay
                    ReadU16(ms); // stayAwake
                    ReadU16(ms); // disableResources
                    ReadU16(ms); // noiseWakeDistance
                    ReadU16(ms); // id
                    ReadU16(ms); // spawnMany
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eElectricWall:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // id
                    ReadU16(ms); // enabled
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("ELECWALL.BAN_6000_AePc_0");
                    break;
                }
                case ObjectTypesAe::eLiftMover:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // switchId
                    ReadU16(ms); // liftId
                    ReadU16(ms); // direction
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("BALIFT.BND_1001_AePc_1");
                    break;
                }
                case ObjectTypesAe::eMeatSack:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // side
                    ReadU16(ms); // xVelocity
                    ReadU16(ms); // yVelocity
                    ReadU16(ms); // scale
                    ReadU16(ms); // numberOfItems
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("BONEBAG.BAN_590_AePc_2");
                    break;
                }
                case ObjectTypesAe::eScrab:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // attackDelay
                    ReadU16(ms); // patrolType
                    ReadU16(ms); // leftMinDelay
                    ReadU16(ms); // leftMaxDelay
                    ReadU16(ms); // rightMinDelay
                    ReadU16(ms); // rightMaxDelay
                    ReadU16(ms); // disableResources
                    ReadU16(ms); // roarRandomly
                    ReadU16(ms); // persistant
                    ReadU16(ms); // whirlAttackDuration
                    ReadU16(ms); // whirlAttackRecharge
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("SCRAB.BND_700_AePc_2");
                    break;
                }
                case ObjectTypesAe::eScrabLeftBound:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eScrabRightBound:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eSligRightBound:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //SligId
                    ReadU16(ms); //Disableresources
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eSligPersist:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //SligId
                    ReadU16(ms); //Disableresources
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eEnemyStopper:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Stopdirection
                    ReadU16(ms); //Id
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eInvisibleSwitch:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Id
                    ReadU16(ms); //Idaction
                    ReadU16(ms); //Delay
                    ReadU16(ms); //Setoffalarm
                    ReadU16(ms); //Scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eMud:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //State
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Voicepitch
                    ReadU16(ms); //RescueId
                    ReadU16(ms); //Deaf
                    ReadU16(ms); //Disableresources
                    ReadU16(ms); //Savestate
                    ReadU16(ms); //Emotion
                    ReadU16(ms); //Blind
                    ReadU16(ms); //Angrytrigger
                    ReadU16(ms); //Stoptrigger
                    ReadU16(ms); //Getsdepressed
                    ReadU16(ms); //Ringtimeout
                    ReadU32(ms); //Instantpowerup
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("MUDCHSL.BAN_511_AePc_1");
                    break;
                }
                case ObjectTypesAe::eZSligCover:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eDoorFlame:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Id
                    ReadU16(ms); //Scale
                    ReadU32(ms); //Colour
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("FIRE.BAN_304_AePc_0");
                    break;
                }
                case ObjectTypesAe::eMovingBomb:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Speed
                    ReadU16(ms); //Id
                    ReadU16(ms); //Starttype
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Maxrise
                    ReadU16(ms); //Disableresources
                    ReadU16(ms); //Startspeed
                    ReadU16(ms); //Persistoffscreen
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("MOVEBOMB.BAN_3006_AePc_0");
                    break;
                }
                case ObjectTypesAe::eMenuController:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eTimerTrigger:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Id
                    ReadU16(ms); //Delayticks
                    ReadU16(ms); //Id1
                    ReadU16(ms); //Id2
                    ReadU16(ms); //Id3
                    ReadU16(ms); //Id4
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eSecurityDoor:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Id
                    ReadU16(ms); //Code1
                    ReadU16(ms); //Code2
                    ReadU16(ms); //XPos
                    ReadU16(ms); //YPos
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("SECDOOR.BAN_6027_AePc_1");
                    break;
                }
                case ObjectTypesAe::eGrenadeMachine:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Spoutside
                    ReadU16(ms); //Disabledresources
                    ReadU16(ms); //Numberofgrenades
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eLcdScreen:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Message1Id
                    ReadU16(ms); //Messagerandmin
                    ReadU16(ms); //Messagerandmax
                    ReadU16(ms); //Message2Id
                    ReadU32(ms); //Idtoswitchmessagesets
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eHandStone:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Camera1
                    ReadU16(ms); //Camera2
                    ReadU16(ms); //Camera3
                    ReadU32(ms); //TriggerId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eCreditsController:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eLcdStatusBoard:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Numberofmuds
                    ReadU16(ms); //Zulagnumber
                    ReadU32(ms); //Hidden
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eWheelSyncer:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Id1
                    ReadU16(ms); //Id2
                    ReadU16(ms); //TriggerId
                    ReadU16(ms); //Action
                    ReadU16(ms); //Id3
                    ReadU16(ms); //Id4
                    ReadU16(ms); //Id5
                    ReadU16(ms); //Id6
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eMusic:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Type
                    ReadU16(ms); //Start
                    ReadU32(ms); //Timeticks
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eLightEffect:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Type
                    ReadU16(ms); //Size
                    ReadU16(ms); //Id
                    ReadU16(ms); //FlipX
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eSlogSpawner:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Numberofslogs
                    ReadU16(ms); //Atatime
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Ticksbetweenslogs
                    ReadU16(ms); //Id
                    ReadU16(ms); //Listentosligs
                    ReadU16(ms); //Jumpattackdelay
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eDeathClock:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //StarttriggerId
                    ReadU16(ms); //Time
                    ReadU32(ms); //StoptriggerId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eGasEmitter:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //PortId
                    ReadU16(ms); //Gascolour
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eSlogHut:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Id
                    ReadU32(ms); //TicksbetweenZ's
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eGlukkon:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Calmmotion
                    ReadU16(ms); //Prealarmdelay
                    ReadU16(ms); //Postalarmdelay
                    ReadU16(ms); //SpawnId
                    ReadU16(ms); //Spawndirection
                    ReadU16(ms); //Spawndelay
                    ReadU16(ms); //Glukkontype
                    ReadU16(ms); //StartgasId
                    ReadU16(ms); //PlaymovieId
                    ReadU16(ms); //MovieId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("GLUKKON.BND_800_AePc_1");
                    break;
                }
                case ObjectTypesAe::eKillUnsavedMuds:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eSoftLanding:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU32(ms); //Id
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eWater:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Maxdrops
                    ReadU16(ms); //Id
                    ReadU16(ms); //Splashtime
                    ReadU16(ms); //SplashXvelocity
                    ReadU16(ms); //SplashYvelocity
                    ReadU16(ms); //Timeout
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eWheel:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Id
                    ReadU16(ms); //Duration
                    ReadU16(ms); //Offtime
                    ReadU32(ms); //Offwhenstopped
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("WORKWHEL.BAN_320_AePc_1");
                    break;
                }
                case ObjectTypesAe::eLaughingGas:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Islaughinggas
                    ReadU16(ms); //GasId
                    ReadU16(ms); //Redpercent
                    ReadU16(ms); //Greenpercent
                    ReadU32(ms); //Bluepercent
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eFlyingSlig:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //State
                    ReadU16(ms); //Hipausetime
                    ReadU16(ms); //Patrolpausemin
                    ReadU16(ms); //Patrolpausemax
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Panicdelay
                    ReadU16(ms); //Giveupchasedelay
                    ReadU16(ms); //Prechasedelay
                    ReadU16(ms); //SligId
                    ReadU16(ms); //Listentime
                    ReadU16(ms); //Triggerid
                    ReadU16(ms); //Grenadedelay
                    ReadU16(ms); //Maxvelocity
                    ReadU16(ms); //LaunchId
                    ReadU16(ms); //Persistant
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("FLYSLIG.BND_450_AePc_1");
                    break;
                }
                case ObjectTypesAe::eFleech:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Asleep
                    ReadU16(ms); //Wakeup
                    ReadU16(ms); //Notused
                    ReadU16(ms); //Attackanger
                    ReadU16(ms); //Attackdelay
                    ReadU16(ms); //WakeupId
                    ReadU16(ms); //Hanging
                    ReadU16(ms); //Losttargettimeout
                    ReadU16(ms); //Goestosleep
                    ReadU16(ms); //Patrolrange(grids)
                    ReadU16(ms); //Unused
                    ReadU16(ms); //AllowwakeupId
                    ReadU16(ms); //Persistant
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("FLEECH.BAN_900_AePc_1");
                    break;
                }
                case ObjectTypesAe::eSlurgs:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Pausedelay
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Id
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("SLURG.BAN_306_AePc_3");
                    break;
                }
                case ObjectTypesAe::eSlamDoor:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX) + 13, static_cast<float>(object.mRectTopLeft.mY) + 18);
                    entity->GetComponent<AnimationComponent>()->Change("SLAM.BAN_2020_AePc_1");
                    ReadU16(ms); // closed
                    ReadU16(ms); // scale
                    ReadU16(ms); // id
                    ReadU16(ms); // inverted
                    ReadU32(ms); // delete
                    break;
                }
                case ObjectTypesAe::eLevelLoader:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Id
                    ReadU16(ms); //Destlevel
                    ReadU16(ms); //Destpath
                    ReadU16(ms); //Destcamera
                    ReadU32(ms); //Movie
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eDemoSpawnPoint:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eTeleporter:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Id
                    ReadU16(ms); //TargetId
                    ReadU16(ms); //Camera
                    ReadU16(ms); //Path
                    ReadU16(ms); //Level
                    ReadU16(ms); //TriggerId
                    ReadU16(ms); //Halfscale
                    ReadU16(ms); //Wipe
                    ReadU16(ms); //Movienumber
                    ReadU16(ms); //ElectricX
                    ReadU32(ms); //ElectricY
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eSlurgSpawner:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Pausedelay
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Id
                    ReadU16(ms); //Delaybetweenslurgs
                    ReadU16(ms); //Maxslurgs
                    ReadU32(ms); //SpawnerId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eGrinder:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Minofftime
                    ReadU16(ms); //Maxofftime
                    ReadU16(ms); //Id
                    ReadU16(ms); //Behavior
                    ReadU16(ms); //Speed
                    ReadU16(ms); //Startstate
                    ReadU16(ms); //Offspeed
                    ReadU16(ms); //Minofftime2
                    ReadU16(ms); //Maxofftime2
                    ReadU16(ms); //Startposition
                    ReadU16(ms); //Direction
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("DRILL.BAN_6004_AePc_0");
                    break;
                }
                case ObjectTypesAe::eColorfulMeter:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Id
                    ReadU16(ms); //Numberofmeterbars
                    ReadU16(ms); //Timer
                    ReadU16(ms); //Startsfull
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eFlyingSligSpawner:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //State
                    ReadU16(ms); //Hipausetime
                    ReadU16(ms); //Patrolpausemin
                    ReadU16(ms); //Patrolpausemax
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Panicdelay
                    ReadU16(ms); //Giveupchasedelay
                    ReadU16(ms); //Prechasedelay
                    ReadU16(ms); //SligId
                    ReadU16(ms); //Listentime
                    ReadU16(ms); //Triggerid
                    ReadU16(ms); //Grenadedelay
                    ReadU16(ms); //Maxvelocity
                    ReadU16(ms); //LaunchId
                    ReadU16(ms); //Persistant
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eMineCar:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Maxdamage
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("BAYROLL.BAN_6013_AePc_2");
                    break;
                }
                case ObjectTypesAe::eSlogFoodSack:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Side
                    ReadU16(ms); //Xvel
                    ReadU16(ms); //Yvel
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Numberofbones
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("BONEBAG.BAN_590_AePc_2");
                    break;
                }
                case ObjectTypesAe::eExplosionSet:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Startinstantly
                    ReadU16(ms); //Id
                    ReadU16(ms); //Bigrocks
                    ReadU16(ms); //Startdelay
                    ReadU16(ms); //Sequncedirection
                    ReadU16(ms); //Sequncedelay
                    ReadU16(ms); //Spacing(grids)
                    ReadU16(ms); //Scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eMultiswitchController:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eRedGreenStatusLight:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Id
                    ReadU16(ms); //Scale
                    ReadU16(ms); //OtherId1
                    ReadU16(ms); //OtherId2
                    ReadU16(ms); //OtherId3
                    ReadU16(ms); //OtherId4
                    ReadU16(ms); //OtherId5
                    ReadU16(ms); //Snaptogrid
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("STATUSLT.BAN_373_AePc_1");
                    break;
                }
                case ObjectTypesAe::eGhostTrap:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //TargettombId1
                    ReadU16(ms); //TargettombId2
                    ReadU16(ms); //Ispersistant
                    ReadU16(ms); //Hasghost
                    ReadU16(ms); //Haspowerup
                    ReadU16(ms); //PowerupId
                    ReadU16(ms); //OptionId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("GHOSTTRP.BAN_1053_AePc_0");
                    break;
                }
                case ObjectTypesAe::eParamiteNet:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("WEB.BAN_2034_AePc_0");
                    break;
                }
                case ObjectTypesAe::eAlarm:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Id
                    ReadU16(ms); //Duration
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eFartMachine:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Numberofbrews
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("BREWBTN.BAN_6016_AePc_0");
                    break;
                }
                case ObjectTypesAe::eScrabSpawner:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Attackdelay
                    ReadU16(ms); //Patroltype
                    ReadU16(ms); //Leftmindelay
                    ReadU16(ms); //Leftmaxdelay
                    ReadU16(ms); //Rightmindelay
                    ReadU16(ms); //Rightmaxdelay
                    ReadU16(ms); //Attackduration
                    ReadU16(ms); //Disableresources
                    ReadU16(ms); //Roarrandomly
                    ReadU16(ms); //Persistant
                    ReadU16(ms); //Whirlattackduration
                    ReadU16(ms); //Whirlattackrecharge
                    ReadU16(ms); //Killclosefleech
                    ReadU16(ms); //Id
                    ReadU16(ms); //Appearfrom
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eCrawlingSlig:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Direction
                    ReadU16(ms); //State
                    ReadU16(ms); //Lockerdirection
                    ReadU16(ms); //PanicId
                    ReadU16(ms); //Resetondeath
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("CRAWLSLG.BND_449_AePc_3");
                    break;
                }
                case ObjectTypesAe::eSligGetPants:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("LOCKER.BAN_448_AePc_1");
                    break;
                }
                case ObjectTypesAe::eSligGetWings:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //State
                    ReadU16(ms); //Hipausetime
                    ReadU16(ms); //Patrolpausemin
                    ReadU16(ms); //Patrolpausemax
                    ReadU16(ms); //Direction
                    ReadU16(ms); //Panicdelay
                    ReadU16(ms); //Giveupchasedelay
                    ReadU16(ms); //Prechasedelay
                    ReadU16(ms); //SligId
                    ReadU16(ms); //Listentime
                    ReadU16(ms); //Triggerid
                    ReadU16(ms); //Grenadedelay
                    ReadU16(ms); //Maxvelocity
                    ReadU16(ms); //LaunchId
                    ReadU16(ms); //Persistant
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("LOCKER.BAN_448_AePc_1");
                    break;
                }
                case ObjectTypesAe::eGreeter:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Motiondetectorspeed
                    ReadU16(ms); //Direction
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("GREETER.BAN_307_AePc_0");
                    break;
                }
                case ObjectTypesAe::eCrawlingSligButton:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Id
                    ReadU16(ms); //Idaction
                    ReadU16(ms); //Onsound
                    ReadU16(ms); //Offsound
                    ReadU16(ms); //Sounddirection
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("CSLGBUTN.BAN_1057_AePc_0");
                    break;
                }
                case ObjectTypesAe::eGlukkonSecurityDoor:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //OkId
                    ReadU16(ms); //FailId
                    ReadU16(ms); //XPos
                    ReadU32(ms); //YPos
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("SECDOOR.BAN_6027_AePc_1");
                    break;
                }
                case ObjectTypesAe::eDoorBlocker:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); //Scale
                    ReadU16(ms); //Id
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    break;
                }
                case ObjectTypesAe::eTorturedMudokon:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); //SpeedId
                    ReadU16(ms); //ReleaseId
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("MUDTORT.BAN_518_AePc_2");
                    break;
                }
                case ObjectTypesAe::eTrainDoor:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU32(ms); //Xflip
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));
                    entity->GetComponent<AnimationComponent>()->Change("TRAINDOR.BAN_2013_AePc_1");
                    break;
                }
                }
            });
        });
    }))
    {
        auto abe = mGm.mRoot.CreateEntityWith<TransformComponent, PhysicsComponent, AnimationComponent, AbeMovementComponent, AbePlayerControllerComponent, CameraComponent>();
        auto pos = abe->GetComponent<TransformComponent>();
        pos->Set(125.0f, 380.0f + (80.0f));
        pos->SnapXToGrid();

        auto slig = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent, PhysicsComponent, SligMovementComponent, SligPlayerControllerComponent>();
        auto pos2 = slig->GetComponent<TransformComponent>();
        pos2->Set(125.0f + (25.0f), 380.0f + (80.0f));
        pos2->SnapXToGrid();

        SetState(LoaderStates::eInit);
    }
}

void GridMap::Loader::SetState(GridMap::Loader::LoaderStates state)
{
    if (state != mState)
    {
        mState = state;
    }
}

bool GridMap::Loader::Load(const Oddlib::Path& path, ResourceLocator& locator)
{
    switch (mState)
    {
    case LoaderStates::eInit:
        mState = LoaderStates::eSetupAndConvertCollisionItems;
        break;
    case LoaderStates::eSetupAndConvertCollisionItems:
        SetupAndConvertCollisionItems(path);
        break;
    case LoaderStates::eAllocateCameraMemory:
        HandleAllocateCameraMemory(path);
        break;
    case LoaderStates::eLoadCameras:
        HandleLoadCameras(path, locator);
        break;
    case LoaderStates::eLoadEntities:
        RunForAtLeast(kMaxExecutionTimeMs, [&]() { if (mState == LoaderStates::eLoadEntities) { HandleLoadEntities(path); } });
        break;
    }
    return mState == LoaderStates::eInit;
}