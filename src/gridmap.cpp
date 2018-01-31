#include "core/systems/inputsystem.hpp"
#include "core/systems/collisionsystem.hpp"
#include "core/systems/resourcelocatorsystem.hpp"
#include "core/components/physicscomponent.hpp"
#include "core/components/transformcomponent.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/abemovementcomponent.hpp"
#include "core/components/sligmovementcomponent.hpp"

#include "gridmap.hpp"
#include "oddlib/bits_factory.hpp"
#include "engine.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include "world.hpp"

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

/*static*/ void GridMap::RegisterScriptBindings()
{
    Sqrat::Class<IMap, Sqrat::NoConstructor<IMap>> im(Sqrat::DefaultVM::Get(), "IMap");
    Sqrat::RootTable().Bind("IMap", im);

    Sqrat::DerivedClass<GridMap, IMap, Sqrat::NoConstructor<GridMap>> gm(Sqrat::DefaultVM::Get(), "GridMap");

    gm.Func("GetMapObject", &GridMap::GetMapObject);

    Sqrat::RootTable().Bind("GridMap", gm);
}

GridMap::GridMap(CoordinateSpace& coords, WorldState& state)
    : mLoader(*this), mScriptInstance("gMap", this), mWorldState(state)
{


    // Set up the screen size and camera pos so that the grid is drawn correctly during init
    mWorldState.kVirtualScreenSize = glm::vec2(368.0f, 240.0f);
    mWorldState.kCameraBlockSize = glm::vec2(375, 260);
    mWorldState.kCamGapSize = glm::vec2(375, 260);
    mWorldState.kCameraBlockImageOffset = glm::vec2(0, 0);

    coords.SetScreenSize(mWorldState.kVirtualScreenSize);

    const int camX = 0;
    const int camY = 0;
    const glm::vec2 camPos = glm::vec2(
        (camX * mWorldState.kCameraBlockSize.x) + mWorldState.kCameraBlockImageOffset.x,
        (camY * mWorldState.kCameraBlockSize.y) + mWorldState.kCameraBlockImageOffset.y) +
        glm::vec2(mWorldState.kVirtualScreenSize.x / 2, mWorldState.kVirtualScreenSize.y / 2);

    mWorldState.mCameraPosition = camPos;
    coords.SetCameraPosition(mWorldState.mCameraPosition);
}

GridMap::Loader::Loader(GridMap& gm)
    : mGm(gm)
{

}

void GridMap::Loader::SetupSystems(ResourceLocator& locator, const InputState& state)
{
    mGm.mRoot.RegisterComponent<AbeMovementComponent>();
    mGm.mRoot.RegisterComponent<AbePlayerControllerComponent>();
    mGm.mRoot.RegisterComponent<AnimationComponent>();
    mGm.mRoot.RegisterComponent<PhysicsComponent>();
    mGm.mRoot.RegisterComponent<SligMovementComponent>();
    mGm.mRoot.RegisterComponent<SligPlayerControllerComponent>();
    mGm.mRoot.RegisterComponent<TransformComponent>();

    mGm.mRoot.AddSystem<InputSystem>(state);
    mGm.mRoot.AddSystem<CollisionSystem>();
    mGm.mRoot.AddSystem<ResourceLocatorSystem>(locator);

    SetState(LoaderStates::eSetupAndConvertCollisionItems);
}

void GridMap::Loader::SetupAndConvertCollisionItems(const Oddlib::Path& path)
{
    // Clear out existing objects from previous map
    mGm.mWorldState.mObjs.clear();
    mGm.mWorldState.mCollisionItems.clear();

    // The "block" or grid square that a camera fits into, it never usually fills the grid
    mGm.mWorldState.kCameraBlockSize = (path.IsAo()) ? glm::vec2(1024, 480) : glm::vec2(375, 260);
    mGm.mWorldState.kCamGapSize = (path.IsAo()) ? glm::vec2(1024, 480) : glm::vec2(375, 260);

    // Since the camera won't fill a block it can be offset so the camera image is in the middle
    // of the block or else where.
    mGm.mWorldState.kCameraBlockImageOffset = (path.IsAo()) ? glm::vec2(257, 114) : glm::vec2(0, 0);

    mGm.ConvertCollisionItems(path.CollisionItems());

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
        SetState(LoaderStates::eObjectLoaderScripts);
    }
}

void GridMap::Loader::HandleObjectLoaderScripts(ResourceLocator& locator)
{
    SquirrelVm::CompileAndRun(locator, "object_factory.nut");
    Sqrat::Function objFactoryInit(Sqrat::RootTable(), "init_object_factory");
    objFactoryInit.Execute();
    SquirrelVm::CheckError();

    SquirrelVm::CompileAndRun(locator, "map.nut");

    SetState(LoaderStates::eLoadObjects);
}

void GridMap::Loader::HandleLoadObjects(const Oddlib::Path& path, ResourceLocator& locator)
{
    if (mMapObjectBeingLoaded)
    {
        if (mMapObjectBeingLoaded->Init())
        {
            mGm.mWorldState.mObjs.push_back(std::move(mMapObjectBeingLoaded));
        }
        return;
    }

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
                const ObjRect rect =
                    {
                        object.mRectTopLeft.mX,
                        object.mRectTopLeft.mY,
                        object.mRectBottomRight.mX - object.mRectTopLeft.mX,
                        object.mRectBottomRight.mY - object.mRectTopLeft.mY
                    };
                switch (object.mType)
                {
                case ObjectTypesAe::eHoist:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent>();
                    ReadU16(ms); // hoistType
                    ReadU16(ms); // edgeType
                    ReadU16(ms); // id
                    ReadU16(ms); // scale
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(rect.x), static_cast<float>(rect.y));
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
                    auto xoffset = ReadU16(ms);
					auto yoffset = ReadU16(ms);
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(rect.x) + xoffset + 5, static_cast<float>(rect.y) + rect.h + yoffset);
                    entity->GetComponent<AnimationComponent>()->Change("DoorClosed_Barracks");
                    break;
                }
                case ObjectTypesAe::eBackgroundAnimation:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(rect.x), static_cast<float>(rect.y));
                    auto animId = ReadU32(ms);
                    switch (animId)
                    {
                    case 1201:
                        entity->GetComponent<AnimationComponent>()->Change("BAP01C06.CAM_1201_AePc_0");
                        entity->GetComponent<TransformComponent>()->Set(static_cast<float>(rect.x) + 5, static_cast<float>(rect.y) + 3);
                        break;
                    case 1202:
                        entity->GetComponent<AnimationComponent>()->Change("FARTFAN.BAN_1202_AePc_0");
                        break;
                    default:
                        entity->GetComponent<AnimationComponent>()->Change("AbeStandSpeak1");
                    }
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
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(rect.x) + 37, static_cast<float>(rect.y) + rect.h - 5);
                    entity->GetComponent<AnimationComponent>()->Change("SwitchIdle");
                    break;
                }
                case ObjectTypesAe::eMine:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU32(ms); // Skip unused "num patterns"
                    ReadU32(ms); // Skip unused "patterns"
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(rect.x) + 10, static_cast<float>(rect.y) + 22);
                    entity->GetComponent<AnimationComponent>()->Change("LANDMINE.BAN_1036_AePc_0");
                    break;
                }
                case ObjectTypesAe::eElectricWall:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    ReadU16(ms); // scale
                    ReadU16(ms); // id
                    ReadU16(ms); // enabled
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(rect.x), static_cast<float>(rect.y));
                    entity->GetComponent<AnimationComponent>()->Change("ELECWALL.BAN_6000_AePc_0");
                    break;
                }
                case ObjectTypesAe::eSlamDoor:
                {
                    auto* entity = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent>();
                    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(rect.x) + 13, static_cast<float>(rect.y) + 18);
                    entity->GetComponent<AnimationComponent>()->Change("SLAM.BAN_2020_AePc_1");
                    ReadU16(ms); // closed
                    ReadU16(ms); // scale
                    ReadU16(ms); // id
                    ReadU16(ms); // inverted
                    ReadU32(ms); // delete
                    break;
                }
                default:
                {
                    auto mapObj = std::make_unique<MapObject>(locator, rect);
                    Oddlib::IStream* s = &ms; // Script only knows about IStream, not the derived types
                    Sqrat::Function objFactory(Sqrat::RootTable(), "object_factory");
                    Sqrat::SharedPtr<bool> ret = objFactory.Evaluate<bool>(mapObj.get(), &mGm, path.IsAo(), object.mType, rect, s); // TODO: Don't need to pass rect?
                    SquirrelVm::CheckError();
                    // TODO: Handle error case
                    if (ret.get() && *ret)
                    {
                        mMapObjectBeingLoaded = std::move(mapObj);
                    }
                }
                }
            });
        });
    }))
    {
        SetState(LoaderStates::eLoadEntities);
    }
}

void GridMap::Loader::HandleLoadEntities()
{
    auto abe = mGm.mRoot.CreateEntityWith<TransformComponent, PhysicsComponent, AnimationComponent, AbeMovementComponent, AbePlayerControllerComponent>();
    auto pos = abe->GetComponent<TransformComponent>();
    pos->Set(125.0f, 380.0f + (80.0f));
    pos->SnapXToGrid();

    auto slig = mGm.mRoot.CreateEntityWith<TransformComponent, AnimationComponent, PhysicsComponent, SligMovementComponent, SligPlayerControllerComponent>();
    auto pos2 = slig->GetComponent<TransformComponent>();
    pos2->Set(125.0f + (25.0f), 380.0f + (80.0f));
    pos2->SnapXToGrid();
    SetState(LoaderStates::eHackToPlaceAbeInValidCamera);
}

void GridMap::Loader::HandleHackAbeIntoValidCamera(ResourceLocator& locator)
{
    if (mMapObjectBeingLoaded)
    {
        if (mMapObjectBeingLoaded->Init())
        {
            mMapObjectBeingLoaded->SnapXToGrid(); // Ensure player is locked to grid
            mGm.mWorldState.mCameraSubject = mMapObjectBeingLoaded.get();
            mGm.mWorldState.mObjs.push_back(std::move(mMapObjectBeingLoaded));
            SetState(LoaderStates::eInit);
        }
    }
    else
    {
        // TODO: Need to figure out what the right way to figure out where abe goes is
        // HACK: Place the player in the first screen that isn't blank
        for (auto x = 0u; x < mGm.mWorldState.mScreens.size(); x++)
        {
            for (auto y = 0u; y < mGm.mWorldState.mScreens[x].size(); y++)
            {
                GridScreen* screen = mGm.mWorldState.mScreens[x][y].get();
                if (screen->hasTexture())
                {

                    auto xPos = (x * mGm.mWorldState.kCamGapSize.x) + 100.0f;
                    auto yPos = (y * mGm.mWorldState.kCamGapSize.y) + 100.0f;

                    auto tmp = std::make_unique<MapObject>(locator, ObjRect{});

                    Sqrat::Function onInitMap(Sqrat::RootTable(), "on_init_map");
                    Sqrat::SharedPtr<bool> ret = onInitMap.Evaluate<bool>(tmp.get(), &mGm, xPos, yPos);
                    SquirrelVm::CheckError();
                    // TODO: Handle error case
                    if (ret.get() && *ret)
                    {
                        mMapObjectBeingLoaded = std::move(tmp);
                        return;
                    }
                }
            }
        }
    }
}

void GridMap::Loader::SetState(GridMap::Loader::LoaderStates state)
{
    if (state != mState)
    {
        mState = state;
    }
}

bool GridMap::Loader::Load(const Oddlib::Path& path, ResourceLocator& locator, const InputState& input)
{
    switch (mState)
    {
    case LoaderStates::eInit:
        mState = LoaderStates::eSetupSystems;
        break;

    case LoaderStates::eSetupSystems:
        SetupSystems(locator, input);
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

    case LoaderStates::eObjectLoaderScripts:
        HandleObjectLoaderScripts(locator);
        break;

    case LoaderStates::eLoadObjects:
        RunForAtLeast(kMaxExecutionTimeMs, [&]() { if (mState == LoaderStates::eLoadObjects) { HandleLoadObjects(path, locator); } });
        break;

    case LoaderStates::eLoadEntities:
        HandleLoadEntities();
        break;

    case LoaderStates::eHackToPlaceAbeInValidCamera:
        HandleHackAbeIntoValidCamera(locator);
    }

    if (mState == LoaderStates::eInit)
    {
        return true;
    }

    return false;
}

bool GridMap::LoadMap(const Oddlib::Path& path, ResourceLocator& locator, const InputState& input) // TODO: Input wired here
{
#ifdef _DEBUG
    while (!mLoader.Load(path, locator, input))
    {

    }
    return true;
#else
    return mLoader.Load(path, locator, input);
#endif
}

GridMap::~GridMap()
{
    TRACE_ENTRYEXIT;
}

MapObject* GridMap::GetMapObject(s32 x, s32 y, const char* type)
{
    for (auto& obj : mWorldState.mObjs)
    {
        if (obj->Name() == type)
        {
            if (obj->ContainsPoint(x, y))
            {
                return obj.get();
            }
        }
    }
    return nullptr;
}

const CollisionLines& GridMap::Lines() const
{
    return mWorldState.mCollisionItems;
}

static CollisionLine* GetCollisionIndexByIndex(CollisionLines& lines, s16 index)
{
    const s32 count = static_cast<s32>(lines.size());
    if (index > 0)
    {
        if (index < count)
        {
            return lines[index].get();
        }
        else
        {
            LOG_ERROR("Link index is out of bounds: " << index);
        }
    }
    return nullptr;
}

static void ConvertLink(CollisionLines& lines, const Oddlib::Path::Links& oldLink, CollisionLine::Link& newLink)
{
    newLink.mPrevious = GetCollisionIndexByIndex(lines, oldLink.mPrevious);
    newLink.mNext = GetCollisionIndexByIndex(lines, oldLink.mNext);
}

void GridMap::ConvertCollisionItems(const std::vector<Oddlib::Path::CollisionItem>& items)
{
    const s32 count = static_cast<s32>(items.size());
    mWorldState.mCollisionItems.resize(count);

    // First pass to create/convert from original/"raw" path format
    for (auto i = 0; i < count; i++)
    {
        mWorldState.mCollisionItems[i] = std::make_unique<CollisionLine>();
        mWorldState.mCollisionItems[i]->mLine.mP1.x = items[i].mP1.mX;
        mWorldState.mCollisionItems[i]->mLine.mP1.y = items[i].mP1.mY;

        mWorldState.mCollisionItems[i]->mLine.mP2.x = items[i].mP2.mX;
        mWorldState.mCollisionItems[i]->mLine.mP2.y = items[i].mP2.mY;

        mWorldState.mCollisionItems[i]->mType = CollisionLine::ToType(items[i].mType);
    }

    // Second pass to set up raw pointers to existing lines for connected segments of 
    // collision lines
    for (auto i = 0; i < count; i++)
    {
        // TODO: Check if optional link is ever used in conjunction with link
        ConvertLink(mWorldState.mCollisionItems, items[i].mLinks[0], mWorldState.mCollisionItems[i]->mLink);
        ConvertLink(mWorldState.mCollisionItems, items[i].mLinks[1], mWorldState.mCollisionItems[i]->mOptionalLink);
    }

    // Now we can re-order collision items without breaking prev/next links, thus we want to ensure
    // that anything that either has no links, or only a single prev/next links is placed first
    // so that we can render connected segments from the start or end.
    std::sort(std::begin(mWorldState.mCollisionItems), std::end(mWorldState.mCollisionItems), [](std::unique_ptr<CollisionLine>& a, std::unique_ptr<CollisionLine>& b)
    {
        return std::tie(a->mLink.mNext, a->mLink.mPrevious) < std::tie(b->mLink.mNext, b->mLink.mPrevious);
    });

    // Ensure that lines link together physically
    for (auto i = 0; i < count; i++)
    {
        // Some walls have next links, overlapping the walls will break them
        if (mWorldState.mCollisionItems[i]->mLink.mNext && mWorldState.mCollisionItems[i]->mType == CollisionLine::eTrackLine)
        {
            mWorldState.mCollisionItems[i]->mLine.mP2 = mWorldState.mCollisionItems[i]->mLink.mNext->mLine.mP1;
        }
        // TODO: Move collisions in CollisionSystem
        // mRoot.GetSystem<CollisionSystem>()->AddCollisionLine(mWorldState.mCollisionItems[i].get());
    }

    // TODO: Render connected segments as one with control points
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

    mWorldState.mObjs.clear();
    mWorldState.mCollisionItems.clear();
    mWorldState.mScreens.clear();
}
