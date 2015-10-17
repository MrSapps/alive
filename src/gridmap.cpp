#include "gridmap.hpp"
#include "gui.hpp"
#include "oddlib/lvlarchive.hpp"
#include "renderer.hpp"
#include "oddlib/path.hpp"
#include "oddlib/bits_factory.hpp"
#include "logger.hpp"
#include <cassert>
#include "sdl_raii.hpp"

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
    gui.next_window_pos = v2i(600, 100);
    gui_begin_window(&gui, "Paths", v2i(300, 400));

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
            const std::string baseLvlName = item.first.substr(0, 2); // R1, MI etc
            
            // Open the LVL
            const std::string lvlName = baseLvlName + ".LVL";

            auto chunkStream = mFs.ResourcePaths().OpenLvlFileChunkById(lvlName, baseLvlName + "PATH.BND", entry->mPathChunkId);
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
                mTexHandle = mRend.createTexture(GL_RGB, converted->w, converted->h, GL_RGB, GL_UNSIGNED_BYTE, converted->pixels);
            }
            else
            {
                mTexHandle = mRend.createTexture(GL_RGB, surf->w, surf->h, GL_RGB, GL_UNSIGNED_BYTE, surf->pixels);
            }
        }
    }
    return mTexHandle;
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

void GridMap::Render(Renderer& rend, GuiContext& gui, int /*screenW*/, int /*screenH*/)
{
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

    if (mEditorScreenX >= (int)mScreens.size() || // Invalid x check
        (mEditorScreenX >= 0 && mEditorScreenY >= mScreens[mEditorScreenX].size())) // Invalid y check
    {
        mEditorScreenX = mEditorScreenY = -1;
    }

    if (mEditorScreenX >= 0 && mEditorScreenY >= 0)
    {
        GridScreen *screen = mScreens[mEditorScreenX][mEditorScreenY].get();

        gui.next_window_pos = v2i(600, 600);

        V2i size = v2i(640, 480);
        gui_begin_window(&gui, "CAM", size);
        size = gui_window_client_size(&gui);

        rend.beginLayer(gui_layer(&gui));
        V2i pos = gui_turtle_pos(&gui);
        rend.drawQuad(screen->getTexHandle(mFs), 1.0f*pos.x, 1.0f*pos.y, 1.0f*size.x, 1.0f*size.y);
        rend.endLayer();

        gui_end_window(&gui);
    }
}

