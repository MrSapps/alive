#include "gridmap.hpp"
#include "gui.hpp"
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
    gui.next_window_pos = v2i(10, 300);
    gui_begin_window(&gui, "Paths", v2i(150, 400));

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
                    entry->mMapYSize);
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
    : mFs(fs), mLvlName(lvlName)
{
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

    gui_begin_frame(&gui, "camArea", v2i(0, 0), v2i(screenW, screenH));
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
    const V2i camSize = v2i((int)(1440*oldZoomMul), (int)(1080*oldZoomMul)); // TODO: Native reso should be constant somewhere
    const int gap = (int)(20*oldZoomMul);
    const V2i margin = v2i((int)(3000*oldZoomMul), (int)(3000*oldZoomMul));

    // Zoom around cursor
    if (zoomChanged)
    {
        V2f scaledCursorPos = v2i_to_v2f(gui.cursor_pos);// - v2f(screenW/2.f, screenH/2.f);
        V2f oldClientPos = v2i_to_v2f(gui_frame_scroll(&gui)) + scaledCursorPos;
        V2f worldPos = oldClientPos*(1.f/oldZoomMul);
        V2f newClientPos = worldPos*zoomMul;
        V2f newScreenPos = newClientPos - scaledCursorPos;

        gui_set_frame_scroll(&gui, v2f_to_v2i(newScreenPos + v2f(0.5f, 0.5f)));
    }

    // Draw cam backgrounds
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen *screen = mScreens[x][y].get();
            if (!screen->hasTexture())
                continue;

            V2i pos = gui_turtle_pos(&gui) + v2i((camSize.x + gap)*x, (camSize.y + gap)*y) + margin;
            rend.drawQuad(screen->getTexHandle(mFs), 1.0f*pos.x, 1.0f*pos.y, 1.0f*camSize.x, 1.0f*camSize.y);
            gui_enlarge_bounding(&gui, pos + camSize + margin*2);
        }
    }

    // Draw objects
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[x].size(); y++)
        {
            GridScreen *screen = mScreens[x][y].get();
            if (!screen->hasTexture())
                continue;

            V2i pos = gui_turtle_pos(&gui) + v2i((camSize.x + gap)*x, (camSize.y + gap)*y) + margin;
            const Oddlib::Path::Camera& cam = screen->getCamera();
            for (size_t i = 0; i < cam.mObjects.size(); ++i)
            {
                Oddlib::Path::MapObject obj = cam.mObjects[i];
                /*
                obj.mRectTopLeft.mX = 10;
                obj.mRectTopLeft.mY = 30;
                obj.mRectBottomRight.mX = 100;
                obj.mRectBottomRight.mY = 50;
                */
                V2i frameSize = v2i(375, 260); // TODO: Detect which game, and use corresponding reso. 1024x512 for AO, 375x260 for AE
                V2i bgSize = v2i(368, 240); // Size of cam background in object coordinate system
                V2i objPos = v2i((int)(1.f * (obj.mRectTopLeft.mX - (frameSize.x - bgSize.x)*0.5f) * camSize.x / frameSize.x),
                                 (int)(1.f * (obj.mRectTopLeft.mY - (frameSize.y - bgSize.y)*0.5f) * camSize.y / frameSize.y));
                V2i objSize = v2i((int)(1.f * (obj.mRectBottomRight.mX - obj.mRectTopLeft.mX) * camSize.x / frameSize.x),
                                  (int)(1.f * (obj.mRectBottomRight.mY - obj.mRectTopLeft.mY) * camSize.y / frameSize.y));

                rend.beginPath();
                rend.rect(pos.x + 1.f*objPos.x, pos.y + 1.f*objPos.y, 1.f*objSize.x, 1.f*objSize.y);
                rend.strokeColor(Color{1, 1, 1, 1});
                rend.strokeWidth(1.0f);
                rend.stroke();
            }
        }
    }

    rend.endLayer();
    gui_end_frame(&gui);
#endif
}

