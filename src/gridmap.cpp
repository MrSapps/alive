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

Level::Level(GameData& gameData, IAudioController& /*audioController*/, FileSystem& fs)
    : mGameData(gameData), mFs(fs)
{

}

void Level::Update()
{
    if (mMap)
    {
        mMap->Update();
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
                    entry->mNumberOfCollisionItems,
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

GridScreen::GridScreen(const std::string& lvlName, const std::string& fileName, Renderer& rend)
    : mLvlName(lvlName),
      mFileName(fileName)
      , mTexHandle(0)
      , mRend(rend)
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

#if 0
            SDL_SurfacePtr scaled;
            { // Scale the surface
                assert(surf->w == 640);
                assert(surf->h == 240);

                scaled.reset(SDL_CreateRGBSurface(0, 640, 480, 24, 0xFF, 0x00FF, 0x0000FF, 0));
                uint8_t *src = static_cast<uint8_t*>(surf->pixels);
                uint8_t *dst = static_cast<uint8_t*>(scaled->pixels);
                for (int y = 0; y < scaled->h; ++y) {
                    const int src_y = min(y, surf->h);
                    for (int x = 0; x < scaled->w; ++x) {
                        const int src_x = min(x, surf->w);

                        const int dst_ix = x*3 + scaled->pitch*y;
                        const int src_ix = src_x*3 + surf->pitch*src_y;

                        dst[dst_ix + 0] = src[src_ix + 0];
                        dst[dst_ix + 1] = src[src_ix + 1];
                        dst[dst_ix + 2] = src[src_ix + 2];
                    }
                }
            }
            mTexHandle = mRend.createTexture(GL_RGB, scaled->w, scaled->h, GL_RGB, GL_UNSIGNED_BYTE, scaled->pixels, false);
#else
            mTexHandle = mRend.createTexture(GL_RGB, surf->w, surf->h, GL_RGB, GL_UNSIGNED_BYTE, surf->pixels, false);
#endif
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
            mScreens[x][y] = std::make_unique<GridScreen>(mLvlName, path.CameraFileName(x,y), rend);
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
    const float zoomMul = std::pow(zoomBase, 1.f*mZoomLevel);
    // Use oldZoom because gui_set_frame_scroll below doesn't change scrolling in current frame. Could be changed though.
    const V2i camSize = v2i((int)(1440*oldZoomMul), (int)(1080*oldZoomMul)); // TODO: Native reso should be constant somewhere
    const int gap = (int)(20*oldZoomMul);
    const V2i margin = v2i((int)(2000*oldZoomMul), (int)(2000*oldZoomMul));

    // Zoom around cursor
    if (zoomChanged)
    {
        V2f scaledCursorPos = v2i_to_v2f(gui.cursor_pos);// - v2f(screenW/2.f, screenH/2.f);
        V2f oldClientPos = v2i_to_v2f(gui_frame_scroll(&gui)) + scaledCursorPos;
        V2f worldPos = oldClientPos*(1.f/oldZoomMul);
        V2f newClientPos = worldPos*zoomMul;
        V2f newScreenPos = newClientPos - scaledCursorPos;

        gui_set_frame_scroll(&gui, v2f_to_v2i(newScreenPos));
    }

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
    rend.endLayer();
    gui_end_frame(&gui);
#endif
}

