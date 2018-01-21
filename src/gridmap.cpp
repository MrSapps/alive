#include "gridmap.hpp"
#include "oddlib/bits_factory.hpp"
#include "engine.hpp"
#include "fmv.hpp"
#include "sound.hpp"
#include "world.hpp"

GridScreen::GridScreen(const Oddlib::Path::Camera& camera, ResourceLocator& locator)
    : mFileName(camera.mName)
    , mCamera(camera)
    , mLocator(locator)
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
    mWorldState.kCameraBlockSize =  glm::vec2(375, 260);
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
                const Oddlib::Path::MapObject& obj = cam.mObjects[mIForLoop.Value()];
                Oddlib::MemoryStream ms(std::vector<u8>(obj.mData.data(), obj.mData.data() + obj.mData.size()));
                const ObjRect rect =
                {
                    obj.mRectTopLeft.mX,
                    obj.mRectTopLeft.mY,
                    obj.mRectBottomRight.mX - obj.mRectTopLeft.mX,
                    obj.mRectBottomRight.mY - obj.mRectTopLeft.mY
                };

                auto mapObj = std::make_unique<MapObject>(locator, rect);

                Sqrat::Function objFactory(Sqrat::RootTable(), "object_factory");
                Oddlib::IStream* s = &ms; // Script only knows about IStream, not the derived types
                Sqrat::SharedPtr<bool> ret = objFactory.Evaluate<bool>(mapObj.get(), &mGm, path.IsAo(), obj.mType, rect, s); // TODO: Don't need to pass rect?
                SquirrelVm::CheckError();
                // TODO: Handle error case
                if (ret.get() && *ret)
                {
                    mMapObjectBeingLoaded = std::move(mapObj);
                }
            });
        });
    }))
    {
        SetState(LoaderStates::eHackToPlaceAbeInValidCamera);
    }
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
                GridScreen *screen = mGm.mWorldState.mScreens[x][y].get();
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

    case LoaderStates::eObjectLoaderScripts:
        HandleObjectLoaderScripts(locator);
        break;

    case LoaderStates::eLoadObjects:
        RunForAtLeast(kMaxExecutionTimeMs, [&]() { if (mState == LoaderStates::eLoadObjects) { HandleLoadObjects(path, locator); } });
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
    mRoot = std::make_unique<Entity>();
    mRoot->AddChild(std::make_unique<AbeEntity>(locator, input));
    mRoot->AddChild(std::make_unique<SligEntity>(locator, input));
#ifdef _DEBUG
    while (!mLoader.Load(path, locator))
    {
        
    }
    return true;
#else
    return mLoader.Load(path, locator);
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
