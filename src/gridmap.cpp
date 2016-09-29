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

namespace Physics
{
    bool raycast_lines(const glm::vec2& line1p1, const glm::vec2& line1p2, const glm::vec2& line2p1, const glm::vec2& line2p2, raycast_collision * collision)
    {
        //bool lines_intersect = false;
        bool segments_intersect = false;
        glm::vec2 intersection;
        glm::vec2 close_p1;
        glm::vec2 close_p2;

        // Get the segments' parameters.
        float dx12 = line1p2.x - line1p1.x;
        float dy12 = line1p2.y - line1p1.y;
        float dx34 = line2p2.x - line2p1.x;
        float dy34 = line2p2.y - line2p1.y;

        // Solve for t1 and t2
        float denominator = (dy12 * dx34 - dx12 * dy34);

        float t1 =
            ((line1p1.x - line2p1.x) * dy34 + (line2p1.y - line1p1.y) * dx34)
            / denominator;
        if (glm::isinf(t1))
        {
            // The lines are parallel (or close enough to it).
            //lines_intersect = false;
            segments_intersect = false;
            const float fnan = nanf("");
            intersection = glm::vec2(fnan, fnan);
            close_p1 = glm::vec2(fnan, fnan);
            close_p2 = glm::vec2(fnan, fnan);
            if (collision)
                collision->intersection = intersection;

            return segments_intersect;
        }
        //lines_intersect = true;

        float t2 =
            ((line2p1.x - line1p1.x) * dy12 + (line1p1.y - line2p1.y) * dx12)
            / -denominator;

        // Find the point of intersection.
        intersection = glm::vec2(line1p1.x + dx12 * t1, line1p1.y + dy12 * t1);
        if (collision)
            collision->intersection = intersection;

        // The segments intersect if t1 and t2 are between 0 and 1.
        segments_intersect =
            ((t1 >= 0) && (t1 <= 1) &&
            (t2 >= 0) && (t2 <= 1));

        // Find the closest points on the segments.
        if (t1 < 0)
        {
            t1 = 0;
        }
        else if (t1 > 1)
        {
            t1 = 1;
        }

        if (t2 < 0)
        {
            t2 = 0;
        }
        else if (t2 > 1)
        {
            t2 = 1;
        }

        close_p1 = glm::vec2(line1p1.x + dx12 * t1, line1p1.y + dy12 * t1);
        close_p2 = glm::vec2(line2p1.x + dx34 * t2, line2p1.y + dy34 * t2);

        return segments_intersect;
    }
}

MapObject::MapObject(sol::state& luaState, ResourceLocator& locator, const std::string& scriptName)
    : mLuaState(luaState), mLocator(locator), mScriptName(scriptName)
{

}

/*static*/ void MapObject::RegisterLuaBindings(sol::state& state)
{
    state.new_usertype<MapObject>("MapObject",
        "SetAnimation", &MapObject::SetAnimation,
        "SnapToGrid", &MapObject::SnapToGrid,
        "FrameNumber", &MapObject::FrameNumber,
        "IsLastFrame", &MapObject::IsLastFrame,
        "AnimUpdate", &MapObject::AnimUpdate,
        "SetAnimationAtFrame", &MapObject::SetAnimationAtFrame,
        "AnimationComplete", &MapObject::AnimationComplete,
        "NumberOfFrames", &MapObject::NumberOfFrames,
        "FrameCounter", &MapObject::FrameCounter,
        "FacingLeft", &MapObject::FacingLeft,
        "FacingRight", &MapObject::FacingRight,
        "FlipXDirection", &MapObject::FlipXDirection,
        "ScriptLoadAnimations", &MapObject::ScriptLoadAnimations,
        "states", &MapObject::mStates,
        "mXPos", &MapObject::mXPos,
        "mYPos", &MapObject::mYPos);
}

void MapObject::Init()
{
    LoadScript(nullptr, nullptr);
}

void MapObject::Init(const ObjRect& rect, Oddlib::IStream& objData)
{
    LoadScript(&rect, &objData);
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

void MapObject::ScriptLoadAnimations()
{
    mAnim = nullptr;
    mAnims.clear();
    mStates.for_each([&](const sol::object& key, const sol::object& value)
    {
        std::string stateName = key.as<std::string>();
        if (stateName == "name")
        {
            mName = value.as<std::string>();
        }
        else if (stateName == "id")
        {
            mId = value.as<s32>();
        }
        else if (value.is<sol::table>())
        {
            const sol::table& state = value.as<sol::table>();
            state.for_each([&](const sol::object& key, const sol::object& value)
            {
                if (key.is<std::string>())
                {
                    std::string name = key.as<std::string>();
                    if (name == "animation")
                    {
                        std::string strAnim = value.as<std::string>();
                        if (!strAnim.empty())
                        {
                            auto pAnim = mLocator.LocateAnimation(strAnim.c_str());
                            if (!pAnim)
                            {
                                LOG_ERROR("Animation: " << strAnim << " not found");
                                abort();
                            }
                            mAnims.insert(std::make_pair(strAnim, std::move(pAnim)));
                        }
                    }
                }
            });
        }
    });
}

void MapObject::LoadScript(const ObjRect* rect, Oddlib::IStream* objData)
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
    if (objData)
    {
        sol::protected_function f = mLuaState["init_with_data"];
        auto ret = f(this, *rect, *objData);
        if (!ret.valid())
        {
            sol::error err = ret;
            std::string what = err.what();
            LOG_ERROR(what);
        }
    }
    else
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

void MapObject::Render(Renderer& rend, GuiContext& gui, int x, int y, float scale)
{
    // Debug ui
    gui_begin_window(&gui, "Script debug");
    if (gui_button(&gui, "Reload script"))
    {
        LoadScript(nullptr, nullptr);
        SnapToGrid();

        
    }
    gui_end_window(&gui);

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
        return false;
    }

    return mAnim->Collision(x, y);
}

void MapObject::SnapToGrid()
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

void Level::Update(const InputState& input)
{
    if (mMap)
    {
        mMap->Update(input);
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
    for (size_t i = 0; i < mFileName.size(); ++i) {
        if (mFileName[i] != ' ' && mFileName[i] != '\0')
        {
            onlySpaces = false;
            break;
        }
    }
    return !onlySpaces;
}

GridMap::GridMap(Oddlib::Path& path, ResourceLocator& locator, sol::state& luaState, Renderer& rend)
    : mCollisionItems(path.CollisionItems()), mPlayer(luaState, locator, "abe.lua")
{
    mCollisionItemsSorted = mCollisionItems;
    mIsAo = path.IsAo();


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
    mPlayer.SnapToGrid();

    // Load objects
    /*
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen* screen = mScreens[x][y].get();
            const Oddlib::Path::Camera& cam = screen->getCamera();
            for (size_t i = 0; i < cam.mObjects.size(); ++i)
            {
                const Oddlib::Path::MapObject& obj = cam.mObjects[i];
                if (path.IsAo() == false)
                {
                    // TODO: Delegate to lua object factory
                    std::string scriptName;
                    if (obj.mType == 24)
                    {
                        scriptName = "mine.lua";
                    }
                    else if (obj.mType == 13)
                    {
                        scriptName = "background_animation.lua";
                    }
                    else if (obj.mType == 17)
                    {
                        scriptName = "switch.lua";
                    }
                    else if (obj.mType == 85)
                    {
                        scriptName = "slam_door.lua";
                    }
                    else if (obj.mType == 5)
                    {
                        scriptName = "door.lua";
                    }
                    else if (obj.mType == 38)
                    {
                        scriptName = "electric_wall.lua";
                    }
                    else
                    {
                        continue;
                    }

                    auto tmp = std::make_unique<MapObject>(luaState, locator, scriptName.c_str());

                    // Default "best guess" positioning
                    tmp->mXPos = obj.mRectTopLeft.mX;
                    tmp->mYPos = obj.mRectTopLeft.mY;

                    const ObjRect rect =  {
                            obj.mRectTopLeft.mX,
                            obj.mRectTopLeft.mY,
                            obj.mRectBottomRight.mX - obj.mRectTopLeft.mX,
                            obj.mRectBottomRight.mY - obj.mRectTopLeft.mY
                    };

                    Oddlib::MemoryStream ms(std::vector<u8>(obj.mData.data(), obj.mData.data() + obj.mData.size()));
                    tmp->Init(rect, ms);

                    mObjs.push_back(std::move(tmp));
                }
            }
        }
    }*/
}

void GridMap::Update(const InputState& input)
{
    if (input.mKeys[SDL_SCANCODE_E].mIsPressed)
    {
        if (mState == eStates::eEditor)
        {
            mState = eStates::eInGame;
            mPlayer.mXPos = mEditorCamOffset.x;
            mPlayer.mYPos = mEditorCamOffset.y;
        }
        else if (mState == eStates::eInGame)
        {
            mState = eStates::eEditor;
            mEditorCamOffset.x = mPlayer.mXPos;
            mEditorCamOffset.y = mPlayer.mYPos;
        }
    }

    f32 editorCamSpeed = 10.0f;

    if (input.mKeys[SDL_SCANCODE_LCTRL].mIsDown)
    {
        if (input.mKeys[SDL_SCANCODE_W].mIsPressed)
            mEditorCamZoom--;
        else if (input.mKeys[SDL_SCANCODE_S].mIsPressed)
            mEditorCamZoom++;

        mEditorCamZoom = glm::clamp(mEditorCamZoom, 1, 15);
    }
    else
    {
        if (input.mKeys[SDL_SCANCODE_LSHIFT].mIsDown)
            editorCamSpeed *= 4;

        if (input.mKeys[SDL_SCANCODE_W].mIsDown)
            mEditorCamOffset.y -= editorCamSpeed;
        else if (input.mKeys[SDL_SCANCODE_S].mIsDown)
            mEditorCamOffset.y += editorCamSpeed;

        if (input.mKeys[SDL_SCANCODE_A].mIsDown)
            mEditorCamOffset.x -= editorCamSpeed;
        else if (input.mKeys[SDL_SCANCODE_D].mIsDown)
            mEditorCamOffset.x += editorCamSpeed;
    }

    mPlayer.Update(input);

    for (std::unique_ptr<MapObject>& obj : mObjs)
    {
        obj->Update(input);
    }
}

bool GridMap::raycast_map(const glm::vec2& line1p1, const glm::vec2& line1p2, int collisionType, Physics::raycast_collision * collision)
{
    std::sort(std::begin(mCollisionItemsSorted), std::end(mCollisionItemsSorted), [line1p1](const Oddlib::Path::CollisionItem& lhs, const Oddlib::Path::CollisionItem& rhs)
    {
        return glm::distance((glm::vec2(lhs.mP1.mX, lhs.mP1.mY) + glm::vec2(lhs.mP2.mX, lhs.mP2.mY)) / 2.0f, line1p1) < glm::distance((glm::vec2(rhs.mP1.mX, rhs.mP1.mY) + glm::vec2(rhs.mP2.mX, rhs.mP2.mY)) / 2.0f, line1p1);
    });

    for (const Oddlib::Path::CollisionItem& item : mCollisionItemsSorted)
    {
        if (item.mType != collisionType)
            continue;

        if (Physics::raycast_lines(glm::vec2(item.mP1.mX, item.mP1.mY), glm::vec2(item.mP2.mX, item.mP2.mY), line1p1, line1p2, collision))
            return true;
    }

    return false;
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

void GridMap::RenderDebug(Renderer& rend)
{
    // Draw collisions
    if (Debugging().mCollisionLines)
    {
        rend.strokeColor(Color{ 0, 0, 1, 1 });
        rend.strokeWidth(2.f);
        for (const Oddlib::Path::CollisionItem& item : mCollisionItemsSorted)
        {
            glm::vec2 p1 = rend.WorldToScreen(glm::vec2(item.mP1.mX, item.mP1.mY));
            glm::vec2 p2 = rend.WorldToScreen(glm::vec2(item.mP2.mX, item.mP2.mY));

            rend.beginPath();
            rend.moveTo(p1.x, p1.y);
            rend.lineTo(p2.x, p2.y);
            rend.stroke();

            rend.text(p1.x, p1.y, std::string("L: " + std::to_string(item.mType)).c_str());
        }
    }

    // Draw grid
    if (Debugging().mGrid)
    {
        rend.strokeColor(Color{ 1, 1, 1, 0.1f });
        rend.strokeWidth(2.f);
        int gridLineCountX = static_cast<int>((rend.mScreenSize.x / mEditorGridSizeX) / 2) + 2;
        for (int x = -gridLineCountX; x < gridLineCountX; x++)
        {
            rend.beginPath();
            glm::vec2 screenPos = rend.WorldToScreen(glm::vec2(rend.mCameraPosition.x + (x * mEditorGridSizeX) - (static_cast<int>(rend.mCameraPosition.x) % mEditorGridSizeX), 0));
            rend.moveTo(screenPos.x, 0);
            rend.lineTo(screenPos.x, static_cast<f32>(rend.mH));
            rend.stroke();
        }
        int gridLineCountY = static_cast<int>((rend.mScreenSize.y / mEditorGridSizeY) / 2) + 2;
        for (int y = -gridLineCountY; y < gridLineCountY; y++)
        {
            rend.beginPath();
            glm::vec2 screenPos = rend.WorldToScreen(glm::vec2(0, rend.mCameraPosition.y + (y * mEditorGridSizeY) - (static_cast<int>(rend.mCameraPosition.y) % mEditorGridSizeY)));
            rend.moveTo(0, screenPos.y);
            rend.lineTo(static_cast<f32>(rend.mW), screenPos.y);
            rend.stroke();
        }
    }

    // Draw objects
    if (Debugging().mObjectBoundingBoxes)
    {
        rend.strokeColor(Color{ 1, 1, 1, 1 });
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

void GridMap::RenderEditor(Renderer& rend, GuiContext& gui)
{
    //gui_begin_panel(&gui, "camArea");

    rend.mSmoothCameraPosition = true;

    rend.beginLayer(gui_layer(&gui) + 1);

    glm::vec2 camGapSize = (mIsAo) ? glm::vec2(1024, 480) : glm::vec2(375, 260);

    rend.mScreenSize = glm::vec2(rend.mW / 8, rend.mH / 8) * static_cast<f32>(mEditorCamZoom);

    rend.mCameraPosition = mEditorCamOffset;

    // Draw every cam
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen *screen = mScreens[x][y].get();
            if (!screen->hasTexture())
                continue;

            rend.drawQuad(screen->getTexHandle(), x * camGapSize.x, y * camGapSize.y, 368.0f, 240.0f);
        }
    }

    RenderDebug(rend);

    //const f32 zoomBase = 1.2f;
    //const f32 oldZoomMul = std::pow(zoomBase, 1.f*mZoomLevel);
    //bool zoomChanged = false;
    //if (gui.key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT)
    //{
    //    mZoomLevel += gui.mouse_scroll;
    //    zoomChanged = (gui.mouse_scroll != 0);
    //}
    //// Cap zooming so that things don't clump in the upper left corner
    //mZoomLevel = std::max(mZoomLevel, -12);

    //const f32 zoomMul = std::pow(zoomBase, 1.f*mZoomLevel);
    //// Use oldZoom because gui_set_frame_scroll below doesn't change scrolling in current frame. Could be changed though.
    //const int camSize[2] = { (int)(1440 * oldZoomMul), (int)(1080 * oldZoomMul) }; // TODO: Native reso should be constant somewhere
    //const int margin[2] = { (int)(3000 * oldZoomMul), (int)(3000 * oldZoomMul) };

    //int worldFrameSize[2] = { 375, 260 };
    //if (mIsAo)
    //{
    //    worldFrameSize[0] = 1024;
    //    worldFrameSize[1] = 480;
    //}
    //int worldCamSize[2] = { 368, 240 }; // Size of cam background in object coordinate system
    //f32 frameSize[2] = { 1.f * worldFrameSize[0] / worldCamSize[0] * camSize[0],
    //    1.f * worldFrameSize[1] / worldCamSize[1] * camSize[1] };

    //// Zoom around cursor
    //*if (zoomChanged)
    //{
    //    int scroll[2];
    //    gui_scroll(&gui, &scroll[0], &scroll[1]);
    //    f32 scaledCursorPos[2] = { 1.f*gui.cursor_pos[0], 1.f*gui.cursor_pos[1] };
    //    f32 oldClientPos[2] = { scroll[0] + scaledCursorPos[0], scroll[1] + scaledCursorPos[1] };
    //    f32 worldPos[2] = { oldClientPos[0] * (1.f / oldZoomMul), oldClientPos[1] * (1.f / oldZoomMul) };
    //    f32 newClientPos[2] = { worldPos[0] * zoomMul, worldPos[1] * zoomMul };
    //    f32 newScreenPos[2] = { newClientPos[0] - scaledCursorPos[0], newClientPos[1] - scaledCursorPos[1] };

    //    gui_set_scroll(&gui, (int)(newScreenPos[0] + 0.5f), (int)(newScreenPos[1] + 0.5f));
    //}*/

    //// Draw cam backgrounds
    //f32 offset[2] = { 0, 0 };
    //if (mIsAo)
    //{
    //    offset[0] = 257.f * camSize[0] / worldCamSize[0];
    //    offset[1] = 114.f * camSize[1] / worldCamSize[1];
    //}
    //for (auto x = 0u; x < mScreens.size(); x++)
    //{
    //    for (auto y = 0u; y < mScreens[x].size(); y++)
    //    {
    //        GridScreen *screen = mScreens[x][y].get();
    //        if (!screen->hasTexture())
    //            continue;

    //        int pos[2];
    //        gui_turtle_pos(&gui, &pos[0], &pos[1]);
    //        pos[0] += (int)(frameSize[0] * x + offset[0]) + margin[0];
    //        pos[1] += (int)(frameSize[1] * y + offset[1]) + margin[1];
    //        rend.drawQuad(screen->getTexHandle(), 1.0f*pos[0], 1.0f*pos[1], 1.0f*camSize[0], 1.0f*camSize[1]);
    //        gui_enlarge_bounding(&gui, pos[0] + camSize[0] + margin[0] * 2,
    //            pos[1] + camSize[1] + margin[1] * 2);
    //    }
    //}

    //// Draw collision lines
    //{
    //    rend.strokeColor(Color{ 0, 0, 1, 1 });
    //    rend.strokeWidth(2.f);
    //    int pos[2];
    //    gui_turtle_pos(&gui, &pos[0], &pos[1]);
    //    pos[0] += margin[0];
    //    pos[1] += margin[1];
    //    for (size_t i = 0; i < mCollisionItems.size(); ++i)
    //    {
    //        const Oddlib::Path::CollisionItem& item = mCollisionItems[i];
    //        int p1[2] = { (int)(1.f * item.mP1.mX * frameSize[0] / worldFrameSize[0]),
    //            (int)(1.f * item.mP1.mY * frameSize[1] / worldFrameSize[1]) };
    //        int p2[2] = { (int)(1.f * item.mP2.mX * frameSize[0] / worldFrameSize[0]),
    //            (int)(1.f * item.mP2.mY * frameSize[1] / worldFrameSize[1]) };
    //        rend.beginPath();
    //        rend.moveTo(pos[0] + p1[0] + 0.5f, pos[1] + p1[1] + 0.5f);
    //        rend.lineTo(pos[0] + p2[0] + 0.5f, pos[1] + p2[1] + 0.5f);
    //        rend.stroke();
    //    }
    //}

    //{ // Draw objects
    //    rend.strokeColor(Color{ 1, 1, 1, 1 });
    //    rend.strokeWidth(1.f);
    //    for (auto x = 0u; x < mScreens.size(); x++)
    //    {
    //        for (auto y = 0u; y < mScreens[x].size(); y++)
    //        {
    //            GridScreen *screen = mScreens[x][y].get();
    //            if (!screen->hasTexture())
    //                continue;

    //            int pos[2];
    //            gui_turtle_pos(&gui, &pos[0], &pos[1]);
    //            pos[0] += margin[0];
    //            pos[1] += margin[1];
    //            const Oddlib::Path::Camera& cam = screen->getCamera();
    //            for (size_t i = 0; i < cam.mObjects.size(); ++i)
    //            {
    //                const Oddlib::Path::MapObject& obj = cam.mObjects[i];
    //                int objPos[2] = { (int)(1.f * obj.mRectTopLeft.mX * frameSize[0] / worldFrameSize[0]),
    //                    (int)(1.f * obj.mRectTopLeft.mY * frameSize[1] / worldFrameSize[1]) };
    //                int objSize[2] = { (int)(1.f * (obj.mRectBottomRight.mX - obj.mRectTopLeft.mX) * frameSize[0] / worldFrameSize[0]),
    //                    (int)(1.f * (obj.mRectBottomRight.mY - obj.mRectTopLeft.mY) * frameSize[1] / worldFrameSize[1]) };

    //                rend.beginPath();
    //                rend.rect(pos[0] + 1.f*objPos[0] + 0.5f, pos[1] + 1.f*objPos[1] + 0.5f, 1.f*objSize[0], 1.f*objSize[1]);
    //                rend.stroke();
    //            }
    //        }
    //    }
    //}

    rend.endLayer();
    //gui_end_panel(&gui);
}

void GridMap::RenderGame(Renderer& rend, GuiContext& gui)
{
    rend.mSmoothCameraPosition = false;

    glm::vec2 camGapSize = (mIsAo) ? glm::vec2(1024, 480) : glm::vec2(375, 260);

    rend.mScreenSize = glm::vec2(368, 240);
    int camX = static_cast<int>(mPlayer.mXPos / camGapSize.x);
    int camY = static_cast<int>(mPlayer.mYPos / camGapSize.y);

    rend.mCameraPosition = glm::vec2(camX * camGapSize.x, camY * camGapSize.y) + glm::vec2(368 / 2, 240 / 2);

    // Culling is disabled until proper camera position updating order is fixed
    /*if (camX >= 0 && camY >= 0 && camX < static_cast<int>(mScreens.size()) && camY < static_cast<int>(mScreens[camX].size()))
    {
        GridScreen *screen = mScreens[camX][camY].get();
        if (screen->hasTexture())
            rend.drawQuad(screen->getTexHandle(), camX * camGapSize.x, camY * camGapSize.y, 368.0f, 240.0f);
    }*/

    // For now draw every cam
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen *screen = mScreens[x][y].get();
            if (!screen->hasTexture())
            {
                screen = nullptr;
                continue;
            }

            rend.drawQuad(screen->getTexHandle(), x * camGapSize.x, y * camGapSize.y, 368.0f, 240.0f);
        }
    }

    RenderDebug(rend);

    for (std::unique_ptr<MapObject>& obj : mObjs)
    {
        obj->Render(rend, gui, 0, 0, 1.0f);
    }

    mPlayer.Render(rend, gui,
        0,
        0,
        1.0f);

    Physics::raycast_collision collision;

    // Test raycasting for shadows
    if (raycast_map(glm::vec2(mPlayer.mXPos, mPlayer.mYPos), glm::vec2(mPlayer.mXPos, mPlayer.mYPos + 500), 0, &collision))
    {
        if (Debugging().mRayCasts)
        {
            glm::vec2 screenSpaceHit = rend.WorldToScreen(collision.intersection);

            rend.strokeColor(Color{ 1, 0, 0, 1 });
            rend.strokeWidth(2.f);
            rend.beginPath();
            rend.moveTo(screenSpaceHit.x, screenSpaceHit.y);
            rend.lineTo(screenSpaceHit.x, screenSpaceHit.y - 20);
            rend.stroke();
        }
    }
}

void GridMap::Render(Renderer& rend, GuiContext& gui)
{
    if (mState == eStates::eEditor)
    {
        RenderEditor(rend, gui);
    }
    else if (mState == eStates::eInGame)
    {
        RenderGame(rend, gui);
    }
}
