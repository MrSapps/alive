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

Player::Player(sol::state& luaState, ResourceLocator& locator)
    : mLuaState(luaState), mLocator(locator)
{

}

/*static*/ void Player::RegisterLuaBindings(sol::state& state)
{
    state.new_usertype<Player>("Player",
        "SetAnimation", &Player::SetAnimation,
        "PlaySoundEffect", &Player::PlaySoundEffect,
        "FrameNumber", &Player::FrameNumber,
        "IsLastFrame", &Player::IsLastFrame,
        "FacingLeft", &Player::FacingLeft,
        "FacingRight", &Player::FacingRight,
        "FlipXDirection", &Player::FlipXDirection,
        "ScriptLoadAnimations", &Player::ScriptLoadAnimations,
        "states", &Player::mStates,
        "mXPos", &Player::mXPos,
        "mYPos", &Player::mYPos);
}

void Player::Init()
{
    LoadScript();
}

void Player::ScriptLoadAnimations()
{
    mAnim = nullptr;
    mAnims.clear();
    mStates.for_each([&](const sol::object& key, const sol::object& value)
    {
        std::string stateName = key.as<std::string>();
        if (value.is<sol::table>())
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
                        auto pAnim = mLocator.LocateAnimation(strAnim.c_str());
                        if (!pAnim)
                        {
                            LOG_ERROR("Animation: " << strAnim << " not found");
                            abort();
                        }
                        mAnims.insert(std::make_pair(strAnim, std::move(pAnim)));
                    }
                }
            });
        }
    });
}

void Player::LoadScript()
{
    // Load FSM script
    const std::string script = mLocator.LocateScript("abe.lua");
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
    sol::protected_function f = mLuaState["init"];
    auto ret = f(this);
    if (!ret.valid())
    {
        sol::error err = ret;
        std::string what = err.what();
        LOG_ERROR(what);
    }
}

void Player::Update(const InputState& input)
{
    if (mAnim)
    {
        mAnim->Update();
    }

    sol::protected_function f = mLuaState["update"];
    auto ret = f(this, input.Mapping().GetActions());
    if (!ret.valid())
    {
        sol::error err = ret;
        std::string what = err.what();
        LOG_ERROR(what);
    }
}

void Player::SetAnimation(const std::string& animation)
{
    if (mAnims.find(animation) == std::end(mAnims))
    {
        LOG_ERROR("Animation " << animation << " is not loaded");
        abort();
    }
    mAnim = mAnims[animation].get();
    mAnim->Restart();
}

bool Player::IsLastFrame() const
{
    return mAnim->IsLastFrame();
}

s32 Player::FrameNumber() const
{
    return mAnim->FrameNumber();
}

void Player::Render(Renderer& rend, GuiContext& gui, int x, int y, float scale)
{
    // Debug ui
    gui_begin_window(&gui, "Script debug");
    if (gui_button(&gui, "Reload script"))
    {
        LoadScript();
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

// ============================================

Level::Level(IAudioController& /*audioController*/, ResourceLocator& locator, sol::state& luaState)
    : mLocator(locator), mLuaState(luaState)
{

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

void Level::Render(Renderer& rend, GuiContext& gui, int screenW, int screenH)
{
    RenderDebugPathSelection(rend, gui);

    if (mMap)
    {
        mMap->Render(rend, gui, screenW, screenH);
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
        SDL_Surface* surf = mCam->GetSurface();
        mTexHandle = mRend.createTexture(GL_RGB, surf->w, surf->h, GL_RGB, GL_UNSIGNED_BYTE, surf->pixels, true);
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
    : mCollisionItems(path.CollisionItems()), mPlayer(luaState, locator)
{
    mIsAo = path.IsAo();

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
}

void GridMap::Update(const InputState& input)
{
    if (input.mKeys[SDL_SCANCODE_E].mIsPressed)
    {
        if (mState == eStates::eEditor)
        {
            mState = eStates::eInGame;
        }
        else if (mState == eStates::eInGame)
        {
            mState = eStates::eEditor;
        }
    }

    mPlayer.Update(input);
}

void GridMap::RenderEditor(Renderer& rend, GuiContext& gui, int, int)
{
    //gui_begin_panel(&gui, "camArea");
    rend.beginLayer(gui_layer(&gui) + 1);

    glm::vec2 camGapSize = (mIsAo) ? glm::vec2(1024, 480) : glm::vec2(375, 260);

    rend.mScreenSize = glm::vec2(rend.mW, rend.mH);
    int camX = static_cast<int>(mPlayer.mXPos / camGapSize.x) * camGapSize.x;
    int camY = static_cast<int>(mPlayer.mYPos / camGapSize.y) * camGapSize.y;

    rend.mCameraPosition = glm::vec2(camX, camY) + glm::vec2(368 / 2, 240 / 2);

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
    ///*if (zoomChanged)
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

void GridMap::RenderGame(Renderer& rend, GuiContext& gui, int w, int h)
{
    glm::vec2 camGapSize = (mIsAo) ? glm::vec2(1024,480) : glm::vec2(375, 260); 
    
    rend.mScreenSize = glm::vec2(368, 240);
    int camX = static_cast<int>(mPlayer.mXPos / camGapSize.x) * camGapSize.x;
    int camY = static_cast<int>(mPlayer.mYPos / camGapSize.y) * camGapSize.y;

    rend.mCameraPosition = glm::vec2(camX,camY) + glm::vec2(368 / 2, 240 / 2);

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

    mPlayer.Render(rend, gui,
        0,
        0,
        1.0f);
}

void GridMap::Render(Renderer& rend, GuiContext& gui, int screenW, int screenH)
{
    if (mState == eStates::eEditor)
    {
        RenderEditor(rend, gui, screenW, screenH);
    }
    else if (mState == eStates::eInGame)
    {
        RenderGame(rend, gui, screenW, screenH);
    }
}
