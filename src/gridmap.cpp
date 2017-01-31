#include "gridmap.hpp"
#include "gui.h"
#include "oddlib/lvlarchive.hpp"
#include "renderer.hpp"
#include "oddlib/path.hpp"
#include "oddlib/bits_factory.hpp"
#include "logger.hpp"
#include <cassert>
#include "sdl_raii.hpp"
#include <algorithm> // min/max
#include <cmath>
#include "resourcemapper.hpp"
#include "engine.hpp"

MapObject::MapObject(IMap& map, sol::state& luaState, ResourceLocator& locator, const ObjRect& rect)
    : mMap(map), mLuaState(luaState), mLocator(locator), mRect(rect)
{

}

MapObject::MapObject(IMap& map, sol::state& luaState, ResourceLocator& locator, const std::string& scriptName)
    : mMap(map), mLuaState(luaState), mLocator(locator), mScriptName(scriptName)
{

}

/*static*/ void MapObject::RegisterLuaBindings(sol::state& state)
{
    state.new_usertype<MapObject>("MapObject",
        "SetAnimation", &MapObject::SetAnimation,
        "SetAnimationFrame", &MapObject::SetAnimationFrame,
        "FrameNumber", &MapObject::FrameNumber,
        "IsLastFrame", &MapObject::IsLastFrame,
        "AnimUpdate", &MapObject::AnimUpdate,
        "SetAnimationAtFrame", &MapObject::SetAnimationAtFrame,
        "AnimationComplete", &MapObject::AnimationComplete,
        "NumberOfFrames", &MapObject::NumberOfFrames,
        "FrameCounter", &MapObject::FrameCounter,

        "WallCollision", &MapObject::WallCollision,
        "CellingCollision", &MapObject::CellingCollision,
        "FloorCollision", &MapObject::FloorCollision,

        "SnapXToGrid", &MapObject::SnapXToGrid,
        "FacingLeft", &MapObject::FacingLeft,

        "FacingRight", &MapObject::FacingRight,
        "FlipXDirection", &MapObject::FlipXDirection,
        "states", &MapObject::mStates,
        "mXPos", &MapObject::mXPos,
        "mYPos", &MapObject::mYPos);
}

void MapObject::Init()
{
    LoadScript();
}

void MapObject::GetName()
{
    mName = mStates["mName"];
}

void MapObject::Activate(bool direction)
{
    sol::protected_function f = mStates["Activate"];
    auto ret = f(direction);
    if (!ret.valid())
    {
        sol::error err = ret;
        std::string what = err.what();
        LOG_ERROR(what);
    }
}

bool MapObject::WallCollision(f32 dx, f32 dy) const
{
    // The game checks for both kinds of walls no matter the direction
    // ddcheat into a tunnel and the "inside out" wall will still force
    // a crouch.
    return
        CollisionLine::RayCast<2>(mMap.Lines(),
            glm::vec2(mXPos, mYPos + dy),
            glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos + dy),
            { 1u, 2u }, nullptr);
}

bool MapObject::CellingCollision(f32 dx, f32 dy) const
{
    return CollisionLine::RayCast<1>(mMap.Lines(),
        glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos - 2), // avoid collision if we are standing on a celling
        glm::vec2(mXPos + (mFlipX ? -dx : dx), mYPos + dy),
        { 3u }, nullptr);
}

std::tuple<bool, f32, f32, f32> MapObject::FloorCollision() const
{
    Physics::raycast_collision c;
    if (CollisionLine::RayCast<1>(mMap.Lines(),
        glm::vec2(mXPos, mYPos),
        glm::vec2(mXPos, mYPos + 260*3), // Check up to 3 screen down
        { 0u }, &c))
    {
        const f32 distance = glm::distance(mYPos, c.intersection.y);
        return std::make_tuple(true, c.intersection.x, c.intersection.y, distance);
    }
    return std::make_tuple(false, 0.0f, 0.0f, 0.0f);
}

void MapObject::LoadScript()
{
    // Load FSM script
    const std::string script = mLocator.LocateScript(mScriptName.c_str());
    try
    {
        mLuaState.script(script);
    }
    catch (const sol::error& /*ex*/)
    {
        //LOG_ERROR(ex.what());
        return;
    }

    // Set initial state
    try
    {
        sol::protected_function f = mLuaState["init"];
        auto ret = f(this);
        if (!ret.valid())
        {
            sol::error err = ret;
            std::string what = err.what();
            LOG_ERROR(what);
        }

    }
    catch (const sol::error& e)
    {
        LOG_ERROR(e.what());
    }
}

void MapObject::Update(const InputState& input)
{
    //TRACE_ENTRYEXIT;

    Debugging().mDebugObj = this;
    if (Debugging().mSingleStepObject && !Debugging().mDoSingleStepObject)
    {
        return;
    }

    //if (mAnim)
    {
        sol::protected_function f = mLuaState["update"];
        auto ret = f(this, input.Mapping().GetActions());
        if (!ret.valid())
        {
            sol::error err = ret;
            std::string what = err.what();
            LOG_ERROR(what);
        }
    }

    //::Sleep(300);

    static float prevX = 0.0f;
    static float prevY = 0.0f;
    if (prevX != mXPos || prevY != mYPos)
    {
        //LOG_INFO("Player X Delta " << mXPos - prevX << " Y Delta " << mYPos - prevY << " frame " << mAnim->FrameNumber());
    }
    prevX = mXPos;
    prevY = mYPos;

    Debugging().mInfo.mXPos = mXPos;
    Debugging().mInfo.mYPos = mYPos;
    Debugging().mInfo.mFrameToRender = FrameNumber();

    if (Debugging().mSingleStepObject && Debugging().mDoSingleStepObject)
    {
        // Step is done - no more updates till the user requests it
        Debugging().mDoSingleStepObject = false;
    }
}

bool MapObject::AnimationComplete() const
{
    if (!mAnim) { return false; }
    return mAnim->IsComplete();
}

void MapObject::SetAnimation(const std::string& animation)
{
    if (animation.empty())
    {
        mAnim = nullptr;
    }
    else
    {
        if (mAnims.find(animation) == std::end(mAnims))
        {
            auto anim = mLocator.LocateAnimation(animation.c_str());
            if (!anim)
            {
                LOG_ERROR("Animation " << animation << " not found");
                abort();
            }
            mAnims[animation] = std::move(anim);
        }
        mAnim = mAnims[animation].get();
        mAnim->Restart();
    }
}

void MapObject::SetAnimationFrame(s32 frame)
{
    if (mAnim)
    {
        mAnim->SetFrame(frame);
    }
}

void MapObject::SetAnimationAtFrame(const std::string& animation, u32 frame)
{
    SetAnimation(animation);
    mAnim->SetFrame(frame);
}

bool MapObject::AnimUpdate()
{
    return mAnim->Update();
}

s32 MapObject::FrameCounter() const
{
    return mAnim->FrameCounter();
}

s32 MapObject::NumberOfFrames() const
{
    return mAnim->NumberOfFrames();
}

bool MapObject::IsLastFrame() const
{
    return mAnim->IsLastFrame();
}

s32 MapObject::FrameNumber() const
{
    if (!mAnim) { return 0; }
    return mAnim->FrameNumber();
}

void MapObject::ReloadScript()
{
    LoadScript();
    SnapXToGrid();
}

void MapObject::Render(Renderer& rend, GuiContext& /*gui*/, int x, int y, float scale) const
{
    if (mAnim)
    {
        mAnim->SetXPos(static_cast<s32>(mXPos)+x);
        mAnim->SetYPos(static_cast<s32>(mYPos)+y);
        mAnim->SetScale(scale);
        mAnim->Render(rend, mFlipX);
    }
}

bool MapObject::ContainsPoint(s32 x, s32 y) const
{
    if (!mAnim)
    {
        // For animationless objects use the object rect
        return PointInRect(x, y, mRect.x, mRect.y, mRect.w, mRect.h);
    }

    return mAnim->Collision(x, y);
}

void MapObject::SnapXToGrid()
{
    //25x20 grid hack
    const float oldX = mXPos;
    const s32 xpos = static_cast<s32>(mXPos);
    const s32 gridPos = (xpos - 12) % 25;
    if (gridPos >= 13)
    {
        mXPos = static_cast<float>(xpos - gridPos + 25);
    }
    else
    {
        mXPos = static_cast<float>(xpos - gridPos);
    }

    LOG_INFO("SnapX: " << oldX << " to " << mXPos);
}

// ============================================

Level::Level(IAudioController& /*audioController*/, ResourceLocator& locator, sol::state& luaState, Renderer& rend)
    : mLocator(locator), mLuaState(luaState)
{

    // Debugging - reload path and load next path
    static std::string currentPathName;
    static s32 nextPathIndex;
    Debugging().mFnNextPath = [&]() 
    {
        s32 idx = 0;
        for (const auto& pathMap : mLocator.mResMapper.mPathMaps)
        {
            if (idx == nextPathIndex)
            {
                std::unique_ptr<Oddlib::Path> path = mLocator.LocatePath(pathMap.first.c_str());
                if (path)
                {
                    mMap = std::make_unique<GridMap>(*path, mLocator, mLuaState, rend);
                    currentPathName = pathMap.first;
                    nextPathIndex = idx +1;
                    if (nextPathIndex > static_cast<s32>(mLocator.mResMapper.mPathMaps.size()))
                    {
                        nextPathIndex = 0;
                    }
                }
                else
                {
                    LOG_ERROR("LVL or file in LVL not found");
                }
                return;
            }
            idx++;
        }
    };

    Debugging().mFnReloadPath = [&]()
    {
        if (!currentPathName.empty())
        {
            std::unique_ptr<Oddlib::Path> path = mLocator.LocatePath(currentPathName.c_str());
            if (path)
            {
                mMap = std::make_unique<GridMap>(*path, mLocator, mLuaState, rend);
            }
            else
            {
                LOG_ERROR("LVL or file in LVL not found");
            }
        }
    };
}

void Level::EnterState()
{

}

void Level::Update(const InputState& input, CoordinateSpace& coords)
{
    if (mMap)
    {
        mMap->Update(input, coords);
    }
}

void Level::Render(Renderer& rend, GuiContext& gui, int , int )
{
    if (Debugging().mShowBrowserUi)
    {
        RenderDebugPathSelection(rend, gui);
    }

    if (mMap)
    {
        mMap->Render(rend, gui);
    }
}

void Level::RenderDebugPathSelection(Renderer& rend, GuiContext& gui)
{
    gui_begin_window(&gui, "Paths");

    for (const auto& pathMap : mLocator.mResMapper.mPathMaps)
    {
        if (gui_button(&gui, pathMap.first.c_str()))
        {
            std::unique_ptr<Oddlib::Path> path = mLocator.LocatePath(pathMap.first.c_str());
            if (path)
            {
                mMap = std::make_unique<GridMap>(*path, mLocator, mLuaState, rend);
            }
            else
            {
                LOG_ERROR("LVL or file in LVL not found");
            }
            
        }
    }

    gui_end_window(&gui);
}

GridScreen::GridScreen(const std::string& lvlName, const Oddlib::Path::Camera& camera, Renderer& rend, ResourceLocator& locator)
    : mLvlName(lvlName)
    , mFileName(camera.mName)
    , mTexHandle(0)
    , mCamera(camera)
    , mLocator(locator)
    , mRend(rend)
{
   
}

GridScreen::~GridScreen()
{

}

int GridScreen::getTexHandle()
{
    if (!mTexHandle)
    {
        mCam = mLocator.LocateCamera(mFileName.c_str());
        if (mCam) // One path trys to load BRP08C10.CAM which exists in no data sets anywhere!
        {
            SDL_Surface* surf = mCam->GetSurface();
            mTexHandle = mRend.createTexture(GL_RGB, surf->w, surf->h, GL_RGB, GL_UNSIGNED_BYTE, surf->pixels, true);
        }
    }
    return mTexHandle;
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

GridMap::GridMap(Oddlib::Path& path, ResourceLocator& locator, sol::state& luaState, Renderer& rend)
    : mPlayer(*this, luaState, locator, "abe.lua")
{
    mIsAo = path.IsAo();

    // Size of the screen you see during normal game play, this is always less the the "block" the camera image fits into
    kVirtualScreenSize = glm::vec2(368.0f, 240.0f);

    // The "block" or grid square that a camera fits into, it never usually fills the grid
    kCameraBlockSize = (mIsAo) ? glm::vec2(1024, 480) : glm::vec2(375, 260);

    // Since the camera won't fill a block it can be offset so the camera image is in the middle
    // of the block or else where.
    kCameraBlockImageOffset = (mIsAo) ? glm::vec2(257, 114) : glm::vec2(0, 0);

    ConvertCollisionItems(path.CollisionItems());

    luaState.set_function("GetMapObject", &GridMap::GetMapObject, this);
    luaState.set_function("ActivateObjectsWithId", &GridMap::ActivateObjectsWithId, this);

    mScreens.resize(path.XSize());
    for (auto& col : mScreens)
    {
        col.resize(path.YSize());
    }

    for (u32 x = 0; x < path.XSize(); x++)
    {
        for (u32 y = 0; y < path.YSize(); y++)
        {
            mScreens[x][y] = std::make_unique<GridScreen>(mLvlName, path.CameraByPosition(x, y), rend, locator);
        }
    }

    mPlayer.Init();

    // TODO: Need to figure out what the right way to figure out where abe goes is
    // HACK: Place the player in the first screen that isn't blank
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen *screen = mScreens[x][y].get();
            if (screen->hasTexture())
            {
                const glm::vec2 camGapSize = (mIsAo) ? glm::vec2(1024, 480) : glm::vec2(375, 260);

                mPlayer.mXPos = (x * camGapSize.x) + 100.0f;
                mPlayer.mYPos = (y * camGapSize.y) + 100.0f;
            }
        }
    }
    mPlayer.SnapXToGrid();

    // Hack: Don't reload object_factory as the require statements only execute once
    // which results in the global factory table being cleared
    static bool factoryLoaded = false;
    if (!factoryLoaded)
    {
        const std::string script = locator.LocateScript("object_factory.lua");
        try
        {
            luaState.script(script);
        }
        catch (const sol::error& ex)
        {
            LOG_ERROR(ex.what()); // TODO: This is fatal
            return;
        }
        factoryLoaded = true;
    }

    // Load objects
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen* screen = mScreens[x][y].get();
            const Oddlib::Path::Camera& cam = screen->getCamera();
            for (size_t i = 0; i < cam.mObjects.size(); ++i)
            {
                const Oddlib::Path::MapObject& obj = cam.mObjects[i];
                Oddlib::MemoryStream ms(std::vector<u8>(obj.mData.data(), obj.mData.data() + obj.mData.size()));
                const ObjRect rect =
                {
                    obj.mRectTopLeft.mX,
                    obj.mRectTopLeft.mY,
                    obj.mRectBottomRight.mX - obj.mRectTopLeft.mX,
                    obj.mRectBottomRight.mY - obj.mRectTopLeft.mY
                };
                auto tmp = std::make_unique<MapObject>(*this, luaState, locator, rect);
                // Default "best guess" positioning
                tmp->mXPos = obj.mRectTopLeft.mX;
                tmp->mYPos = obj.mRectTopLeft.mY;

                sol::function f = luaState["object_factory"];
                Oddlib::IStream* s = &ms; // required else binding fails
                bool ret = f(*tmp, path.IsAo(), obj.mType, rect, *s);
                if (ret)
                {
                    tmp->GetName();
                    mObjs.push_back(std::move(tmp));
                }
            }
        }
    }
}

void GridMap::Update(const InputState& input, CoordinateSpace& coords)
{
    if (mState == eStates::eEditor)
    {
        UpdateEditor(input, coords);
    }
    else if (mState == eStates::eInGame)
    {
        UpdateGame(input, coords);
    }
    else
    {
        UpdateToEditorOrToGame(input, coords);
    }
}

void GridMap::UpdateToEditorOrToGame(const InputState& input, CoordinateSpace& coords)
{
    std::ignore = input;

    if (mState == eStates::eToEditor)
    {
        coords.SetScreenSize(glm::vec2(coords.Width(), coords.Height()) * mEditorCamZoom);
        if (SDL_TICKS_PASSED(SDL_GetTicks(), mModeSwitchTimeout))
        {
            mState = eStates::eEditor;
        }
    }
    else if (mState == eStates::eToGame)
    {
        coords.SetScreenSize(glm::vec2(368, 240));
        if (SDL_TICKS_PASSED(SDL_GetTicks(), mModeSwitchTimeout))
        {
            mState = eStates::eInGame;
        }
    }
    coords.SetCameraPosition(mCameraPosition);
}

constexpr u32 kSwitchTimeMs = 300;

void GridMap::UpdateEditor(const InputState& input, CoordinateSpace& coords)
{
    const glm::vec2 mousePosWorld = coords.ScreenToWorld({ input.mMousePosition.mX, input.mMousePosition.mY });

    if (input.mKeys[SDL_SCANCODE_E].IsPressed())
    {
        mState = eStates::eToGame;
        coords.mSmoothCameraPosition = true;
        mModeSwitchTimeout = SDL_GetTicks() + kSwitchTimeMs;

        const s32 mouseCamX = static_cast<s32>(mousePosWorld.x / kCameraBlockSize.x);
        const s32 mouseCamY = static_cast<s32>(mousePosWorld.y / kCameraBlockSize.y);

        mPlayer.mXPos = (mouseCamX * kCameraBlockSize.x) + (kVirtualScreenSize.x / 2);
        mPlayer.mYPos = (mouseCamY * kCameraBlockSize.y) + (kVirtualScreenSize.y / 2);

        mCameraPosition.x = mPlayer.mXPos;
        mCameraPosition.y = mPlayer.mYPos;
    }

    bool bGoFaster = false;
    if (input.mKeys[SDL_SCANCODE_LCTRL].IsDown())
    {
        if (input.mKeys[SDL_SCANCODE_W].IsPressed()) { mEditorCamZoom -= 0.1f; }
        else if (input.mKeys[SDL_SCANCODE_S].IsPressed()) { mEditorCamZoom += 0.1f; }

        mEditorCamZoom = glm::clamp(mEditorCamZoom, 0.1f, 3.0f);
    }
    else
    {
        if (input.mKeys[SDL_SCANCODE_LSHIFT].IsDown()) { bGoFaster = true; }

        f32 editorCamSpeed = 10.0f * mEditorCamZoom;
        if (bGoFaster)
        {
            editorCamSpeed *= 4.0f;
        }

        if (input.mKeys[SDL_SCANCODE_W].IsDown()) { mCameraPosition.y -= editorCamSpeed; }
        else if (input.mKeys[SDL_SCANCODE_S].IsDown()) { mCameraPosition.y += editorCamSpeed; }

        if (input.mKeys[SDL_SCANCODE_A].IsDown()) { mCameraPosition.x -= editorCamSpeed; }
        else if (input.mKeys[SDL_SCANCODE_D].IsDown()) { mCameraPosition.x += editorCamSpeed; }
    }

    coords.SetScreenSize(glm::vec2(coords.Width(), coords.Height()) * mEditorCamZoom);
    coords.SetCameraPosition(mCameraPosition);

    // Find out what line is under the mouse pos, if any
    const s32 lineIdx = CollisionLine::Pick(mCollisionItems, mousePosWorld, mState == eStates::eInGame ? 1.0f : mEditorCamZoom);

    if (lineIdx >= 0)
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND)); // SDL_SYSTEM_CURSOR_CROSSHAIR
    }
    else
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
    }

    if (input.mMouseButtons[0].IsReleased())
    {
        mSelectionState = eSelectionState::eNone;
    }

    if (input.mKeys[SDL_SCANCODE_LCTRL].IsDown())
    {
        if (input.mKeys[SDL_SCANCODE_Z].IsPressed())
        {
            mUndoStack.Undo();
        }
        else if (input.mKeys[SDL_SCANCODE_Y].IsPressed())
        {
            mUndoStack.Redo();
        }
    }

    if (input.mMouseButtons[0].IsPressed())
    {
        mMergeCommand = false; // New mouse press/selection, start a new undo command
        if (lineIdx >= 0)
        {
            const bool bGroupSelection = input.mKeys[SDL_SCANCODE_LCTRL].IsDown();
            bool updateState = true;
            if (bGroupSelection)
            {
                LOG_INFO("Toggle line selected status");
                // Only change state if we select a new line, when we de-select there is no selection area to update
                updateState = !mCollisionItems[lineIdx]->IsSelected();
                mUndoStack.Push<CommandSelectOrDeselectLine>(mCollisionItems, mSelection, lineIdx, !mCollisionItems[lineIdx]->IsSelected());
                if (!updateState)
                {
                    mSelectionState = eSelectionState::eNone;
                }
            }
            else
            {
                // If clicking on a line that is already selected then don't clear out any other lines that are already selected
                if (mSelection.SelectedLines().find(lineIdx) == std::end(mSelection.SelectedLines()))
                {
                    LOG_INFO("Select single line");
                    if (mSelection.HasSelection())
                    {
                        mUndoStack.Push<CommandClearSelection>(mCollisionItems, mSelection);
                    }
                    mUndoStack.Push<CommandSelectOrDeselectLine>(mCollisionItems, mSelection, lineIdx, true);
                }
                else
                {
                    LOG_INFO("Line already selected, do nothing");
                }
            }

            if (updateState)
            {
                if (mSelection.SelectedLines().size() > 1)
                {
                    mSelectionState = eSelectionState::eMoveSelected;
                }
                else if (mSelection.SelectedLines().size() == 1)
                {
                    const f32 lineLength = mCollisionItems[lineIdx]->mLine.Length();
                    const f32 distToP1 = glm::distance(mCollisionItems[lineIdx]->mLine.mP1, mousePosWorld) / lineLength;
                    const f32 distToP2 = glm::distance(mCollisionItems[lineIdx]->mLine.mP2, mousePosWorld) / lineLength;
                    if (distToP1 < 0.2f)
                    {
                        mSelectionState = eSelectionState::eLineP1Selected;
                    }
                    else if (distToP2 < 0.2f)
                    {
                        mSelectionState = eSelectionState::eLineP2Selected;
                    }
                    else
                    {
                        mSelectionState = eSelectionState::eLineMiddleSelected;
                    }
                }
                else
                {
                    mSelectionState = eSelectionState::eNone;
                }
            }
            mLastMousePos = mousePosWorld;
        }
        else
        {
            if (mSelection.HasSelection())
            {
                LOG_INFO("Nothing clicked, clear selected");
                mUndoStack.Push<CommandClearSelection>(mCollisionItems, mSelection);
            }
        }
    }
    else if (input.mMouseButtons[0].IsDown() && mSelectionState != eSelectionState::eNone && mLastMousePos != mousePosWorld)
    {
        // TODO: Update mouse cursor to reflect action
        switch (mSelectionState)
        {
        case eSelectionState::eLineP1Selected:
        case eSelectionState::eLineP2Selected:
            // TODO: Handle dis/connect to other lines when moving end points
            mUndoStack.PushMerge<CommandMoveLinePoint>(mMergeCommand, mCollisionItems, mSelection, mousePosWorld, mSelectionState == eSelectionState::eLineP1Selected);
            break;

        case eSelectionState::eMoveSelected:
        case eSelectionState::eLineMiddleSelected:
            // TODO: Disconnect from other lines if moved away
            mUndoStack.PushMerge<MoveSelection>(mMergeCommand, mCollisionItems, mSelection, mousePosWorld - mLastMousePos);
            break;

        case eSelectionState::eNone:
            break;
        }

        mLastMousePos = mousePosWorld;
        mMergeCommand = true;
    }
}

void GridMap::UpdateGame(const InputState& input, CoordinateSpace& coords)
{
    if (input.mKeys[SDL_SCANCODE_E].IsPressed())
    {
        mState = eStates::eToEditor;
        coords.mSmoothCameraPosition = true;

        mModeSwitchTimeout = SDL_GetTicks() + kSwitchTimeMs;

        mCameraPosition.x = mPlayer.mXPos;
        mCameraPosition.y = mPlayer.mYPos;
    }

    coords.SetScreenSize(glm::vec2(368, 240));
 
    mPlayer.Update(input);

    for (std::unique_ptr<MapObject>& obj : mObjs)
    {
        obj->Update(input);
    }

    const s32 camX = static_cast<s32>(mPlayer.mXPos / kCameraBlockSize.x);
    const s32 camY = static_cast<s32>(mPlayer.mYPos / kCameraBlockSize.y);

    const glm::vec2 camPos = glm::vec2(
        (camX * kCameraBlockSize.x) + kCameraBlockImageOffset.x,
        (camY * kCameraBlockSize.y) + kCameraBlockImageOffset.y) + glm::vec2(kVirtualScreenSize.x / 2, kVirtualScreenSize.y / 2);

    if (mCameraPosition != camPos)
    {
        LOG_INFO("TODO: Screen change");
        coords.mSmoothCameraPosition = false;
        mCameraPosition = camPos;
    }
    coords.SetCameraPosition(mCameraPosition);
}

MapObject* GridMap::GetMapObject(s32 x, s32 y, const char* type)
{
    for (std::unique_ptr<MapObject>& obj : mObjs)
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

void GridMap::ActivateObjectsWithId(MapObject* from, s32 id, bool direction)
{
    for (std::unique_ptr<MapObject>& obj : mObjs)
    {
        if (obj.get() != from && obj->Id() == id)
        {
            obj->Activate(direction);
        }
    }
}

void GridMap::RenderDebug(Renderer& rend) const
{
    // Draw collisions
    if (Debugging().mCollisionLines)
    {
        CollisionLine::Render(rend, mCollisionItems);
    }

    // Draw grid
    if (Debugging().mGrid)
    {
        rend.strokeColor(ColourF32{ 1, 1, 1, 0.1f });
        rend.strokeWidth(2.f);

        int gridLineCountX = static_cast<int>((rend.ScreenSize().x / mEditorGridSizeX));
        for (int x = -gridLineCountX; x < gridLineCountX; x++)
        {
            rend.beginPath();
            glm::vec2 screenPos = rend.WorldToScreen(glm::vec2(rend.CameraPosition().x + (x * mEditorGridSizeX) - (static_cast<int>(rend.CameraPosition().x) % mEditorGridSizeX), 0));
            rend.moveTo(screenPos.x, 0);
            rend.lineTo(screenPos.x, static_cast<f32>(rend.Height()));
            rend.stroke();
        }

        int gridLineCountY = static_cast<int>((rend.ScreenSize().y / mEditorGridSizeY));
        for (int y = -gridLineCountY; y < gridLineCountY; y++)
        {
            rend.beginPath();
            glm::vec2 screenPos = rend.WorldToScreen(glm::vec2(0, rend.CameraPosition().y + (y * mEditorGridSizeY) - (static_cast<int>(rend.CameraPosition().y) % mEditorGridSizeY)));
            rend.moveTo(0, screenPos.y);
            rend.lineTo(static_cast<f32>(rend.Width()), screenPos.y);
            rend.stroke();
        }
    }

    // Draw objects
    if (Debugging().mObjectBoundingBoxes)
    {
        rend.strokeColor(ColourF32{ 1, 1, 1, 1 });
        rend.strokeWidth(1.f);
        for (auto x = 0u; x < mScreens.size(); x++)
        {
            for (auto y = 0u; y < mScreens[x].size(); y++)
            {
                GridScreen *screen = mScreens[x][y].get();
                if (!screen->hasTexture())
                    continue;
                const Oddlib::Path::Camera& cam = screen->getCamera();
                for (size_t i = 0; i < cam.mObjects.size(); ++i)
                {
                    const Oddlib::Path::MapObject& obj = cam.mObjects[i];

                    glm::vec2 topLeft = glm::vec2(obj.mRectTopLeft.mX, obj.mRectTopLeft.mY);
                    glm::vec2 bottomRight = glm::vec2(obj.mRectBottomRight.mX, obj.mRectBottomRight.mY);

                    glm::vec2 objPos = rend.WorldToScreen(glm::vec2(topLeft.x, topLeft.y));
                    glm::vec2 objSize = rend.WorldToScreen(glm::vec2(bottomRight.x, bottomRight.y)) - objPos;
                    rend.beginPath();
                    rend.rect(objPos.x, objPos.y, objSize.x, objSize.y);
                    rend.stroke();
                }
            }
        }
    }
}

void GridMap::RenderToEditorOrToGame(Renderer& rend, GuiContext& gui) const
{
    // TODO: Better transition
    // Keep everything rendered for now
    RenderEditor(rend, gui);
}

void GridMap::RenderEditor(Renderer& rend, GuiContext& gui) const
{
    rend.beginLayer(gui_layer(&gui) + 1);

    // Draw every cam
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen *screen = mScreens[x][y].get();
            if (!screen->hasTexture())
                continue;

            rend.drawQuad(screen->getTexHandle(), 
                (x * kCameraBlockSize.x) + kCameraBlockImageOffset.x, 
                (y * kCameraBlockSize.y) + kCameraBlockImageOffset.y, 
                kVirtualScreenSize.x, kVirtualScreenSize.y);
        }
    }

    RenderDebug(rend);

    rend.endLayer();
}

void GridMap::RenderGame(Renderer& rend, GuiContext& gui) const
{
    if (Debugging().mShowDebugUi)
    {
        // Debug ui
        gui_begin_window(&gui, "Script debug");
        if (gui_button(&gui, "Reload abe script"))
        {
            // TODO: Debug hack
            const_cast<MapObject&>(mPlayer).ReloadScript();
        }
        gui_end_window(&gui);
    }
    
    mUndoStack.DebugRenderCommandList(gui);

    const s32 camX = static_cast<s32>(mPlayer.mXPos / kCameraBlockSize.x);
    const s32 camY = static_cast<s32>(mPlayer.mYPos / kCameraBlockSize.y);


    // Culling is disabled until proper camera position updating order is fixed
    // ^ not sure what this means, but rendering things at negative cam index seems to go wrong
    if (camX >= 0 && camY >= 0 && 
        camX < static_cast<s32>(mScreens.size()) && 
        camY < static_cast<s32>(mScreens[camX].size()))
    {
        GridScreen* screen = mScreens[camX][camY].get();
        if (screen->hasTexture())
        {
            rend.drawQuad(screen->getTexHandle(),
                (camX * kCameraBlockSize.x) + kCameraBlockImageOffset.x,
                (camY * kCameraBlockSize.y) + kCameraBlockImageOffset.y,
                kVirtualScreenSize.x, kVirtualScreenSize.y);
        }
    }
    RenderDebug(rend);

    for (const std::unique_ptr<MapObject>& obj : mObjs)
    {
        obj->Render(rend, gui, 0, 0, 1.0f);
    }

    mPlayer.Render(rend, gui,
        0,
        0,
        1.0f);

    // Test raycasting for shadows
    DebugRayCast(rend,
        glm::vec2(mPlayer.mXPos, mPlayer.mYPos),
        glm::vec2(mPlayer.mXPos, mPlayer.mYPos + 500),
        0,
        glm::vec2(0, -10)); // -10 so when we are *ON* a line you can see something

    DebugRayCast(rend,
        glm::vec2(mPlayer.mXPos, mPlayer.mYPos - 2),
        glm::vec2(mPlayer.mXPos, mPlayer.mYPos - 60),
        3,
        glm::vec2(0, 0));

    if (mPlayer.mFlipX)
    {
        DebugRayCast(rend,
            glm::vec2(mPlayer.mXPos, mPlayer.mYPos - 20),
            glm::vec2(mPlayer.mXPos - 25, mPlayer.mYPos - 20), 1);

        DebugRayCast(rend,
            glm::vec2(mPlayer.mXPos, mPlayer.mYPos - 50),
            glm::vec2(mPlayer.mXPos - 25, mPlayer.mYPos - 50), 1);
    }
    else
    {
        DebugRayCast(rend,
            glm::vec2(mPlayer.mXPos, mPlayer.mYPos - 20),
            glm::vec2(mPlayer.mXPos + 25, mPlayer.mYPos - 20), 2);

        DebugRayCast(rend,
            glm::vec2(mPlayer.mXPos, mPlayer.mYPos - 50),
            glm::vec2(mPlayer.mXPos + 25, mPlayer.mYPos - 50), 2);
    }
}

void GridMap::DebugRayCast(Renderer& rend, const glm::vec2& from, const glm::vec2& to, u32 collisionType, const glm::vec2& fromDrawOffset) const
{
    if (Debugging().mRayCasts)
    {
        Physics::raycast_collision collision;
        if (CollisionLine::RayCast<1>(Lines(), from, to, { collisionType }, &collision))
        {
            const glm::vec2 fromDrawPos = rend.WorldToScreen(from + fromDrawOffset);
            const glm::vec2 hitPos = rend.WorldToScreen(collision.intersection);

            rend.strokeColor(ColourF32{ 1, 0, 1, 1 });
            rend.strokeWidth(2.f);
            rend.beginPath();
            rend.moveTo(fromDrawPos.x, fromDrawPos.y);
            rend.lineTo(hitPos.x, hitPos.y);
            rend.stroke();
        }
    }
}

void UndoStack::DebugRenderCommandList(GuiContext& gui) const
{
    gui_begin_window(&gui, "Undo stack");
    for (const auto& cmd : mUndoStack)
    {
        gui_label(&gui, cmd->Message().c_str());
    }
    gui_end_window(&gui);
}

void UndoStack::Clear()
{
    mUndoStack.clear();
}

void UndoStack::Undo()
{
    if (Count() > 0 && mCommandIndex >= 1)
    {
        LOG_ERROR("Undoing command: " << mUndoStack[mCommandIndex - 1]->Message());

        // Undo the current command
        mUndoStack[mCommandIndex-1]->Undo();

        // Go back one command
        mCommandIndex--;
    }
}

void UndoStack::Redo()
{
    if (mCommandIndex + 1 <= Count())
    {
        LOG_ERROR("Redoing command: " << mUndoStack[mCommandIndex]->Message());

        // Redo the current command
        mUndoStack[mCommandIndex]->Redo();

        // Go forward one command
        mCommandIndex++;
    }
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
    mCollisionItems.resize(count);

    // First pass to create/convert from original/"raw" path format
    for (auto i = 0; i < count; i++)
    {
        mCollisionItems[i] = std::make_unique<CollisionLine>();
        mCollisionItems[i]->mLine.mP1.x = items[i].mP1.mX;
        mCollisionItems[i]->mLine.mP1.y = items[i].mP1.mY;

        mCollisionItems[i]->mLine.mP2.x = items[i].mP2.mX;
        mCollisionItems[i]->mLine.mP2.y = items[i].mP2.mY;

        mCollisionItems[i]->mType = CollisionLine::ToType(items[i].mType);
    }

    // Second pass to set up raw pointers to existing lines for connected segments of 
    // collision lines
    for (auto i = 0; i < count; i++)
    {
        // TODO: Check if optional link is ever used in conjunction with link
        ConvertLink(mCollisionItems, items[i].mLinks[0], mCollisionItems[i]->mLink);
        ConvertLink(mCollisionItems, items[i].mLinks[1], mCollisionItems[i]->mOptionalLink);
    }

    // Now we can re-order collision items without breaking prev/next links, thus we want to ensure
    // that anything that either has no links, or only a single prev/next links is placed first
    // so that we can render connected segments from the start or end.
    std::sort(std::begin(mCollisionItems), std::end(mCollisionItems), [](std::unique_ptr<CollisionLine>& a, std::unique_ptr<CollisionLine>& b)
    {
        return std::tie(a->mLink.mNext, a->mLink.mPrevious) < std::tie(b->mLink.mNext, b->mLink.mPrevious);
    });

    // Ensure that lines link together physically
    for (auto i = 0; i < count; i++)
    {
        // Some walls have next links, overlapping the walls will break them
        if (mCollisionItems[i]->mLink.mNext && mCollisionItems[i]->mType == CollisionLine::eTrackLine)
        {
            mCollisionItems[i]->mLine.mP2 = mCollisionItems[i]->mLink.mNext->mLine.mP1;
        }
    }

    // TODO: Render connected segments as one with control points
}

void GridMap::Render(Renderer& rend, GuiContext& gui) const
{
    if (mState == eStates::eEditor)
    {
        RenderEditor(rend, gui);
    }
    else if (mState == eStates::eInGame)
    {
        RenderGame(rend, gui);
    }
    else
    {
        RenderToEditorOrToGame(rend, gui);
    }
}

static u32 gTypeIds = 0;
u32 NextId()
{
    gTypeIds++;
    return gTypeIds;
}


std::set<s32> Selection::Clear(CollisionLines& items)
{
    for (s32 idx : mSelectedLines)
    {
        items[idx]->SetSelected(false);
    }
    auto ret = std::move(mSelectedLines);
    return ret;
}

void Selection::Select(CollisionLines& items, s32 idx, bool select)
{
    if (items[idx]->SetSelected(select))
    {
        mSelectedLines.insert(idx);
    }
    else
    {
        mSelectedLines.erase(idx);
    }
}
