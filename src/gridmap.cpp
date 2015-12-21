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

#ifdef _CRT_SECURE_NO_WARNINGS
#   undef _CRT_SECURE_NO_WARNINGS // stb_image.h might redefine _CRT_SECURE_NO_WARNINGS
#   include "stb_image.h"
#   ifndef _CRT_SECURE_NO_WARNINGS
#       define _CRT_SECURE_NO_WARNINGS
#   endif
#else
#   include "stb_image.h"
#endif

Level::Level(GameData& gameData, IAudioController& /*audioController*/, FileSystem& fs)
    : mGameData(gameData), mFs(fs)
{
    mScript = std::make_unique<Script>();
    if (!mScript->Init(fs))
    {
        LOG_ERROR("Script init failed");
    }
}

void Level::Update()
{
    if (mMap)
    {
        mMap->Update();
    }

    if (mScript)
    {
        mScript->Update();
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
    gui_set_next_window_pos(&gui, 10, 300);
    gui_begin_window(&gui, "Paths", 150, 400);

    static std::vector<std::pair<std::string, const GameData::PathEntry*>> items;
    if (items.empty())
    {

        for (const auto& pathGroup : mGameData.Paths())
        {
            for (const auto& path : pathGroup.second)
            {
                items.push_back(std::make_pair(pathGroup.first + " " + std::to_string(path.mPathChunkId), &path));
            }
        }
    }

    for (const auto& item : items)
    {
        if (gui_button(&gui, item.first.c_str()))
        {
            const GameData::PathEntry* entry = item.second;
            const std::string baseLvlNameUpper = item.first.substr(0, 2); // R1, MI etc
            std::string baseLvlNameLower = baseLvlNameUpper;

            // Open the LVL
            std::transform(baseLvlNameLower.begin(), baseLvlNameLower.end(), baseLvlNameLower.begin(), string_util::c_tolower);
            const std::string lvlName = baseLvlNameLower + ".lvl";

            LOG_INFO("Looking for LVL " << lvlName);

            auto chunkStream = mFs.ResourcePaths().OpenLvlFileChunkById(lvlName, baseLvlNameUpper + "PATH.BND", entry->mPathChunkId);
            if (chunkStream)
            {
                Oddlib::Path path(*chunkStream,
                    entry->mCollisionDataOffset,
                    entry->mObjectIndexTableOffset,
                    entry->mObjectDataOffset,
                    entry->mMapXSize,
                    entry->mMapYSize,
                    entry->mIsAo);
                mMap = std::make_unique<GridMap>(lvlName, path, mFs, rend);
            }
            else
            {
                LOG_ERROR("LVL or file in LVL not found");
            }
        }
    }

    gui_end_window(&gui);
}

GridScreen::GridScreen(const std::string& lvlName, const Oddlib::Path::Camera& camera, Renderer& rend)
    : mLvlName(lvlName)
    , mFileName(camera.mName)
    , mTexHandle(0)
    , mRend(rend)
    , mCamera(camera)
{
   
}

GridScreen::~GridScreen()
{
    mRend.destroyTexture(mTexHandle);
}

int GridScreen::getTexHandle(FileSystem& fs)
{
    if (!mTexHandle)
    {
        auto stream = fs.ResourcePaths().OpenLvlFileChunkByType(mLvlName, mFileName, Oddlib::MakeType('B', 'i', 't', 's'));
        if (stream)
        {
            auto bits = Oddlib::MakeBits(*stream);

            SDL_Surface* surf = bits->GetSurface();
            SDL_SurfacePtr converted;
            if (surf->format->format != SDL_PIXELFORMAT_RGB24)
            {
                converted.reset(SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGB24, 0));
                surf = converted.get();
            }

            { // Create upscaled "hd" image
                assert(surf->w == 640);
                assert(surf->h == 240);

#if 0
                { // Test image
                    uint8_t *src = static_cast<uint8_t*>(surf->pixels);
                    for (int y = 0; y < surf->h; ++y)
                    {
                        for (int x = 0; x < surf->w; ++x)
                        {
                            const int ix = x*3 + surf->pitch*y;

                            if (x == 0 || y == 0 || x == surf->w - 1 || y == surf->h - 1)
                            {
                                src[ix + 0] = 0;
                                src[ix + 1] = 0;
                                src[ix + 2] = 0;
                            }
                            else
                            {
                                src[ix + 0] = 255;
                                src[ix + 1] = 255;
                                src[ix + 2] = 255;
                            }
                        }
                    }
                }
#endif

                // TODO: Use resource system
                // This will look files named like "R1P15C01.CAM"
                std::string path = std::string("../data/deltas/") + mFileName;
                int w, h;
                int bpp;
                uint8_t *dst = static_cast<uint8_t*>(stbi_load(path.c_str(), &w, &h, &bpp, STBI_rgb));
                if (dst)
                {
                    uint8_t *src = static_cast<uint8_t*>(surf->pixels);
                    // Apply delta image over linearly interpolated game cam image
                    for (int y = 0; y < h; ++y)
                    {
                        const float src_rel_y = 1.f*y/(h - 1); // 0..1
                        for (int x = 0; x < w; ++x)
                        {
                            const float src_rel_x = 1.f*x/(w - 1); // 0..1

                            int src_x = (int)floor(src_rel_x*surf->w - 0.5f);
                            int src_y = (int)floor(src_rel_y*surf->h - 0.5f);

                            int src_x_plus = src_x + 1;
                            int src_y_plus = src_y + 1;

                            float lerp_x = src_rel_x*surf->w - (src_x + 0.5f);
                            float lerp_y = src_rel_y*surf->h - (src_y + 0.5f);
                            assert(lerp_x >= 0.0f);
                            assert(lerp_x <= 1.0f);
                            assert(lerp_y >= 0.0f);
                            assert(lerp_y <= 1.0f);

                            // Limit source pixels inside image
                            src_x = std::max(src_x, 0);
                            src_y = std::max(src_y, 0);
                            src_x_plus = std::min(src_x_plus, surf->w - 1);
                            src_y_plus = std::min(src_y_plus, surf->h - 1);

                            // Indices to four neighbouring pixels
                            const int src_indices[4] =
                            {
                                src_x*3 +      surf->pitch*src_y,
                                src_x_plus*3 + surf->pitch*src_y,
                                src_x*3 +      surf->pitch*src_y_plus,
                                src_x_plus*3 + surf->pitch*src_y_plus
                            };

                            const int dst_ix = (x + w*y)*3;

                            for (int comp = 0; comp < 3; ++comp)
                            {
                                // 4 neighbouring texels
                                float a = src[src_indices[0] + comp]/255.f;
                                float b = src[src_indices[1] + comp]/255.f;
                                float c = src[src_indices[2] + comp]/255.f;
                                float d = src[src_indices[3] + comp]/255.f;

                                // 2d linear interpolation
                                float orig = (a*(1 - lerp_x) + b*lerp_x)*(1 - lerp_y) + (c*(1 - lerp_x) + d*lerp_x)*lerp_y;
                                float delta = dst[dst_ix + comp]/255.f;

                                // "Grain extract" has been used in creating the delta image
                                float merged = orig + delta - 0.5f;
                                dst[dst_ix + comp] = (uint8_t)(std::max(std::min(merged*255 + 0.5f, 255.f), 0.0f));
                            }
                        }
                    }
                    SDL_SurfacePtr scaled;
                    scaled.reset(SDL_CreateRGBSurfaceFrom(dst, w, h, 24, 0, 0xFF, 0x00FF, 0x0000FF, 0));
                    mTexHandle = mRend.createTexture(GL_RGB, scaled->w, scaled->h, GL_RGB, GL_UNSIGNED_BYTE, scaled->pixels, true);
                }
                else
                {
                    //printf("Delta load failed: %s\n", stbi_failure_reason());
                    mTexHandle = mRend.createTexture(GL_RGB, surf->w, surf->h, GL_RGB, GL_UNSIGNED_BYTE, surf->pixels, true);
                }
                stbi_image_free(dst);
            }
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

GridMap::GridMap(const std::string& lvlName, Oddlib::Path& path, FileSystem& fs, Renderer& rend)
    : mFs(fs), mLvlName(lvlName), mCollisionItems(path.CollisionItems())
{
    mIsAo = path.IsAo();

    mScreens.resize(path.XSize());
    for (auto& col : mScreens)
    {
        col.resize(path.YSize());
    }

    for (Uint32 x = 0; x < path.XSize(); x++)
    {
        for (Uint32 y = 0; y < path.YSize(); y++)
        {
            mScreens[x][y] = std::make_unique<GridScreen>(mLvlName, path.CameraByPosition(x,y), rend);
        }
    }
}

void GridMap::Update()
{

}

void GridMap::Render(Renderer& rend, GuiContext& gui, int screenW, int screenH)
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

    gui_begin_frame(&gui, "camArea", 0, 0, screenW, screenH);
    rend.beginLayer(gui_layer(&gui));

    const float zoomBase = 1.2f;
    const float oldZoomMul = std::pow(zoomBase, 1.f*mZoomLevel);
    bool zoomChanged = false;
    if (gui.key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT)
    {
        mZoomLevel += gui.mouse_scroll;
        zoomChanged = (gui.mouse_scroll != 0);
    }
    // Cap zooming so that things don't clump in the upper left corner
    mZoomLevel = std::max(mZoomLevel, -12);

    const float zoomMul = std::pow(zoomBase, 1.f*mZoomLevel);
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
    float frameSize[2] = { 1.f * worldFrameSize[0]/worldCamSize[0] * camSize[0],
                           1.f * worldFrameSize[1]/worldCamSize[1] * camSize[1] };

    // Zoom around cursor
    if (zoomChanged)
    {
        int scroll[2];
        gui_frame_scroll(&gui, &scroll[0], &scroll[1]);
        float scaledCursorPos[2] = { 1.f*gui.cursor_pos[0], 1.f*gui.cursor_pos[1] };
        float oldClientPos[2] = { scroll[0] + scaledCursorPos[0], scroll[1] + scaledCursorPos[1] };
        float worldPos[2] = { oldClientPos[0]*(1.f/oldZoomMul), oldClientPos[1]*(1.f/oldZoomMul) };
        float newClientPos[2] = { worldPos[0]*zoomMul, worldPos[1]*zoomMul };
        float newScreenPos[2] = { newClientPos[0] - scaledCursorPos[0], newClientPos[1] - scaledCursorPos[1] };

        gui_set_frame_scroll(&gui, (int)(newScreenPos[0] + 0.5f), (int)(newScreenPos[1] + 0.5f));
    }

    // Draw cam backgrounds
    float offset[2] = {0, 0};
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
            rend.drawQuad(screen->getTexHandle(mFs), 1.0f*pos[0], 1.0f*pos[1], 1.0f*camSize[0], 1.0f*camSize[1]);
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
    gui_end_frame(&gui);
#endif
}

