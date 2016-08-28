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

void Player::Init(ResourceLocator& locator)
{
    mAnims.push_back(locator.LocateAnimation("ABEBSIC.BAN_10_AePc_34"));
}

void Player::Update()
{
    mAnims[0]->Update();
}

void Player::Input(InputState& input)
{
    if (input.mLeftMouseState == 0)
    {
        mAnims[0]->SetXPos(mAnims[0]->XPos() + 1);
    }
}

void Player::Render(Renderer& rend, GuiContext& /*gui*/, int /*screenW*/, int /*screenH*/)
{
    mAnims[0]->Render(rend);
}

Level::Level(IAudioController& /*audioController*/, ResourceLocator& locator)
    : mLocator(locator)
{
    mScript = std::make_unique<Script>();
    /*
    if (!mScript->Init(fs))
    {
        LOG_ERROR("Script init failed");
    }
    */
}

void Level::EnterState()
{
    mPlayer.Init(mLocator);
}

void Level::Input(InputState& input)
{
    mPlayer.Input(input);
}

void Level::Update()
{
    if (mMap)
    {
        mMap->Update();
    }

    mPlayer.Update();

    if (mScript)
    {
        //mScript->Update();
    }
}

void Level::Render(Renderer& rend, GuiContext& gui, int screenW, int screenH)
{
    RenderDebugPathSelection(rend, gui);
    if (mMap)
    {
        mMap->Render(rend, gui, screenW, screenH);
    }

    mPlayer.Render(rend, gui, screenW, screenH);
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

                mMap = std::make_unique<GridMap>(*path, mLocator, rend);
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

GridMap::GridMap(Oddlib::Path& path, ResourceLocator& locator, Renderer& rend)
    : mCollisionItems(path.CollisionItems())
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
}

void GridMap::Update()
{

}

void GridMap::Render(Renderer& rend, GuiContext& gui, int, int)
{
#if 0 // List of cams
    gui.next_window_pos = v2i(950, 50);
    gui_begin_window(&gui, "GridMap", v2i(200, 500));
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[0].size(); y++)
        {
            if (gui_button(&gui, gui_str(&gui, "cam_%i_%i|%s", (int)x, (int)y, mScreens[x][y]->FileName().c_str())))
            {
                mEditorScreenX = (int)x;
                mEditorScreenY = (int)y;
            }
        }
    }
    gui_end_window(&gui);

    if (mEditorScreenX >= static_cast<int>(mScreens.size()) || // Invalid x check
        (mEditorScreenX >= 0 && mEditorScreenY >= static_cast<int>(mScreens[mEditorScreenX].size()))) // Invalid y check
    {
        mEditorScreenX = mEditorScreenY = -1;
    }

    if (mEditorScreenX >= 0 && mEditorScreenY >= 0)
    {
        GridScreen *screen = mScreens[mEditorScreenX][mEditorScreenY].get();

        gui.next_window_pos = v2i(100, 100);

        V2i size = v2i(640, 480);
        gui_begin_window(&gui, "CAM", size);
        size = gui_window_client_size(&gui);

        rend.beginLayer(gui_layer(&gui));
        V2i pos = gui_turtle_pos(&gui);
        rend.drawQuad(screen->getTexHandle(mFs), 1.0f*pos.x, 1.0f*pos.y, 1.0f*size.x, 1.0f*size.y);
        rend.endLayer();

        gui_end_window(&gui);
    }
#else // Proper editor gui

    gui_begin_panel(&gui, "camArea");
    rend.beginLayer(gui_layer(&gui) + 1);

    const f32 zoomBase = 1.2f;
    const f32 oldZoomMul = std::pow(zoomBase, 1.f*mZoomLevel);
    bool zoomChanged = false;
    if (gui.key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT)
    {
        mZoomLevel += gui.mouse_scroll;
        zoomChanged = (gui.mouse_scroll != 0);
    }
    // Cap zooming so that things don't clump in the upper left corner
    mZoomLevel = std::max(mZoomLevel, -12);

    const f32 zoomMul = std::pow(zoomBase, 1.f*mZoomLevel);
    // Use oldZoom because gui_set_frame_scroll below doesn't change scrolling in current frame. Could be changed though.
    const int camSize[2] = { (int)(1440*oldZoomMul), (int)(1080*oldZoomMul) }; // TODO: Native reso should be constant somewhere
    const int margin[2] = { (int)(3000*oldZoomMul), (int)(3000*oldZoomMul) };

    int worldFrameSize[2] = {375, 260};
    if (mIsAo)
    {
        worldFrameSize[0] = 1024;
        worldFrameSize[1] = 480;
    }
    int worldCamSize[2] = {368, 240}; // Size of cam background in object coordinate system
    f32 frameSize[2] = { 1.f * worldFrameSize[0]/worldCamSize[0] * camSize[0],
                           1.f * worldFrameSize[1]/worldCamSize[1] * camSize[1] };

    // Zoom around cursor
    if (zoomChanged)
    {
        int scroll[2];
        gui_scroll(&gui, &scroll[0], &scroll[1]);
        f32 scaledCursorPos[2] = { 1.f*gui.cursor_pos[0], 1.f*gui.cursor_pos[1] };
        f32 oldClientPos[2] = { scroll[0] + scaledCursorPos[0], scroll[1] + scaledCursorPos[1] };
        f32 worldPos[2] = { oldClientPos[0]*(1.f/oldZoomMul), oldClientPos[1]*(1.f/oldZoomMul) };
        f32 newClientPos[2] = { worldPos[0]*zoomMul, worldPos[1]*zoomMul };
        f32 newScreenPos[2] = { newClientPos[0] - scaledCursorPos[0], newClientPos[1] - scaledCursorPos[1] };

        gui_set_scroll(&gui, (int)(newScreenPos[0] + 0.5f), (int)(newScreenPos[1] + 0.5f));
    }

    // Draw cam backgrounds
    f32 offset[2] = {0, 0};
    if (mIsAo)
    {
        offset[0] = 257.f * camSize[0] / worldCamSize[0];
        offset[1] = 114.f * camSize[1] / worldCamSize[1];
    }
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen *screen = mScreens[x][y].get();
            if (!screen->hasTexture())
                continue;

            int pos[2];
            gui_turtle_pos(&gui, &pos[0], &pos[1]);
            pos[0] += (int)(frameSize[0] * x + offset[0]) + margin[0];
            pos[1] += (int)(frameSize[1] * y + offset[1]) + margin[1];
            rend.drawQuad(screen->getTexHandle(), 1.0f*pos[0], 1.0f*pos[1], 1.0f*camSize[0], 1.0f*camSize[1]);
            gui_enlarge_bounding(&gui, pos[0] + camSize[0] + margin[0]*2,
                                       pos[1] + camSize[1] + margin[1]*2);
        }
    }

    // Draw collision lines
    {
        rend.strokeColor(Color{0, 0, 1, 1});
        rend.strokeWidth(2.f);
        int pos[2];
        gui_turtle_pos(&gui, &pos[0], &pos[1]);
        pos[0] += margin[0];
        pos[1] += margin[1];
        for (size_t i = 0; i < mCollisionItems.size(); ++i)
        {
            const Oddlib::Path::CollisionItem& item = mCollisionItems[i];
            int p1[2] = { (int)(1.f * item.mP1.mX * frameSize[0] / worldFrameSize[0]),
                          (int)(1.f * item.mP1.mY * frameSize[1] / worldFrameSize[1]) };
            int p2[2] = { (int)(1.f * item.mP2.mX * frameSize[0] / worldFrameSize[0]),
                          (int)(1.f * item.mP2.mY * frameSize[1] / worldFrameSize[1]) };
            rend.beginPath();
            rend.moveTo(pos[0] + p1[0] + 0.5f, pos[1] + p1[1] + 0.5f);
            rend.lineTo(pos[0] + p2[0] + 0.5f, pos[1] + p2[1] + 0.5f);
            rend.stroke();
        }
    }

    { // Draw objects
        rend.strokeColor(Color{1, 1, 1, 1});
        rend.strokeWidth(1.f);
        for (auto x = 0u; x < mScreens.size(); x++)
        {
            for (auto y = 0u; y < mScreens[x].size(); y++)
            {
                GridScreen *screen = mScreens[x][y].get();
                if (!screen->hasTexture())
                    continue;

                int pos[2];
                gui_turtle_pos(&gui, &pos[0], &pos[1]);
                pos[0] += margin[0];
                pos[1] += margin[1];
                const Oddlib::Path::Camera& cam = screen->getCamera();
                for (size_t i = 0; i < cam.mObjects.size(); ++i)
                {
                    const Oddlib::Path::MapObject& obj = cam.mObjects[i];
                    int objPos[2] = { (int)(1.f * obj.mRectTopLeft.mX * frameSize[0] / worldFrameSize[0]),
                                      (int)(1.f * obj.mRectTopLeft.mY * frameSize[1] / worldFrameSize[1])};
                    int objSize[2] = { (int)(1.f * (obj.mRectBottomRight.mX - obj.mRectTopLeft.mX) * frameSize[0] / worldFrameSize[0]),
                                       (int)(1.f * (obj.mRectBottomRight.mY - obj.mRectTopLeft.mY) * frameSize[1] / worldFrameSize[1])};

                    rend.beginPath();
                    rend.rect(pos[0] + 1.f*objPos[0] + 0.5f, pos[1] + 1.f*objPos[1] + 0.5f, 1.f*objSize[0], 1.f*objSize[1]);
                    rend.stroke();
                }
            }
        }
    }

    rend.endLayer();
    gui_end_panel(&gui);
#endif
}

