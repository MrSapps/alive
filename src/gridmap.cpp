#include "gridmap.hpp"
#include "gui.hpp"
#include "oddlib/lvlarchive.hpp"
#include "renderer.hpp"
#include "oddlib/path.hpp"
#include "oddlib/bits_factory.hpp"

#include <cassert>

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
    gui.next_window_pos = V2i(600, 100);
    gui_begin_window(&gui, "Paths", V2i(300, 400));

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
            auto resource = mFs.ResourceExists(lvlName);
            if (resource)
            {
                auto archive = std::make_unique<Oddlib::LvlArchive>(resource->Open(lvlName));
                
                // Get the BND file from the LVL
                auto file = archive->FileByName(baseLvlName + "PATH.BND");
                if (file)
                {
                    // Get the chunk from the LVL
                    auto chunk = file->ChunkById(entry->mPathChunkId);
                    if (chunk)
                    {
                        // Then we can get a stream for the chunk
                        auto chunkStream = chunk->Stream();
                        Oddlib::Path path(*chunkStream, 
                                          entry->mNumberOfCollisionItems, 
                                          entry->mObjectIndexTableOffset,
                                          entry->mObjectDataOffset, 
                                          entry->mMapXSize, 
                                          entry->mMapYSize);
                        mMap = std::make_unique<GridMap>(path, std::move(archive), rend);
                    }
                }
            }
        }
    }

    gui_end_window(&gui);
}

GridScreen::GridScreen(const std::string& fileName, Oddlib::LvlArchive& archive, Renderer& rend)
    : mFileName(fileName)
    , mTexHandle(0)
    , mRend(rend)
{
    auto file = archive.FileByName(fileName);
    if (file)
    {
        auto chunk = file->ChunkByType(Oddlib::MakeType('B', 'i', 't', 's'));
        auto stream = chunk->Stream();

        auto bits = Oddlib::MakeBits(*stream);

        SDL_Surface *surf = bits->GetSurface();
        SDL_Surface *converted = nullptr;
        if (surf->format->format != SDL_PIXELFORMAT_RGB24)
        {
            converted = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGB24, 0);
            surf = converted;
        }
        mTexHandle = mRend.createTexture(surf->pixels, surf->w, surf->h, PixelFormat_RGB24);
        SDL_FreeSurface(converted);
    }
}

GridScreen::~GridScreen()
{
    mRend.destroyTexture(mTexHandle);
}

GridMap::GridMap(Oddlib::Path& path, std::unique_ptr<Oddlib::LvlArchive> archive, Renderer& rend)
    : mArchive(std::move(archive))
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
            mScreens[x][y] = std::make_unique<GridScreen>(path.CameraFileName(x,y), *mArchive, rend);
        }
    }
}

void GridMap::Update()
{

}

void GridMap::Render(Renderer& rend, GuiContext& gui, int /*screenW*/, int /*screenH*/)
{
    gui.next_window_pos = V2i(950, 50);
    gui_begin_window(&gui, "GridMap", V2i(200, 500));
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[0].size(); y++)
        {
            if (gui_button(&gui, mScreens[x][y]->FileName().c_str()))
            {
                mEditorScreenX = (int)x;
                mEditorScreenY = (int)y;
            }
        }
    }
    gui_end_window(&gui);

    if (mEditorScreenX >= (int)mScreens.size() || mEditorScreenY >= (int)mScreens.size())
    {
        mEditorScreenX = mEditorScreenY = -1;
    }

    if (mEditorScreenX >= 0 && mEditorScreenY >= 0)
    {
        GridScreen *screen = mScreens[mEditorScreenX][mEditorScreenY].get();

        gui.next_window_pos = V2i(600, 600);

        V2i size(640, 480);
        gui_begin_window(&gui, "CAM", size);

        rend.beginLayer(gui_layer(&gui));
        V2i pos = gui_turtle_pos(&gui);
        rend.drawQuad(screen->getTexHandle(), 1.0f*pos.x, 1.0f*pos.y, 1.0f*size.x, 1.0f*size.y);
        rend.endLayer();

        gui_end_window(&gui);
    }
}

