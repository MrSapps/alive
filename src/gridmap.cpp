#include "core/systems/camerasystem.hpp"
#include "core/systems/collisionsystem.hpp"

#include "core/components/cameracomponent.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

#include "oddlib/bits_factory.hpp"
#include "aeentityfactory.hpp"
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

GridMap::GridMap(CoordinateSpace& coords, WorldState& state, EntityManager &entityManager) : mEntityManager(entityManager), mLoader(*this), mWorldState(state)
{
    auto cameraSystem = mEntityManager.GetSystem<CameraSystem>();

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
    auto collisionSystem = mEntityManager.GetSystem<CollisionSystem>();
    if (collisionSystem)
    {
        mEntityManager.Clear();
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
    auto cameraSystem = mGm.mEntityManager.GetSystem<CameraSystem>();
    auto collisionSystem = mGm.mEntityManager.GetSystem<CollisionSystem>();

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
                
                AeEntityFactory factory;
                auto entity = factory.Create(object, mGm.mEntityManager, ms);
            });
        });
    }))
    {
        auto abe = mGm.mEntityManager.CreateEntityWith<TransformComponent, PhysicsComponent, AnimationComponent, AbeMovementComponent, AbePlayerControllerComponent, CameraComponent>();
        auto pos = abe.GetComponent<TransformComponent>();
        pos->Set(125.0f, 380.0f + (80.0f));
        pos->SnapXToGrid();

        auto slig = mGm.mEntityManager.CreateEntityWith<TransformComponent, AnimationComponent, PhysicsComponent, SligMovementComponent, SligPlayerControllerComponent>();
        auto pos2 = slig.GetComponent<TransformComponent>();
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