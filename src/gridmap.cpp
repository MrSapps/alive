#include "gridmap.hpp"
//#include "imgui/imgui.h"
#include "oddlib/lvlarchive.hpp"
#include "renderer.hpp"
#include "oddlib/path.hpp"
#include "oddlib/ao_bits_pc.hpp"

#include <cassert>

Level::Level(GameData& gameData, IAudioController& audioController, FileSystem& fs)
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

void Level::Render(Renderer* rend, int screenW, int screenH)
{
    RenderDebugPathSelection();
    if (mMap)
    {
        mMap->Render(rend, screenW, screenH);
    }
}

void Level::RenderDebugPathSelection()
{
#if 0
    ImGui::Begin("Paths", nullptr, ImVec2(400, 200), 1.0f, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

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
        if (ImGui::Selectable(item.first.c_str()))
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
                        mMap = std::make_unique<GridMap>(path, std::move(archive));
                    }
                }
            }
        }
    }

    ImGui::End();
#endif
}

GridScreen::GridScreen(const std::string& fileName, Oddlib::LvlArchive& archive, Renderer *rend)
    : mFileName(fileName)
    , mTexHandle(0)
    , mRend(rend)
{
    auto file = archive.FileByName(fileName);
    if (file)
    {
        auto chunk = file->ChunkByType(Oddlib::MakeType('B','i','t','s'));
        auto stream = chunk->Stream();
        Oddlib::AoBitsPc bits(*stream);

        assert(bits.getImageFormat()->format == SDL_PIXELFORMAT_RGB24);
        assert(bits.getImageFormat()->BytesPerPixel == 1);
        assert(bits.getImageFormat()->BitsPerPixel == 8);

        mTexHandle = mRend->createTexture(bits.getPixelData(), bits.getImageWidth(), bits.getImageHeight(), PixelFormat_RGB24);
    }
}

GridScreen::~GridScreen()
{
    mRend->destroyTexture(mTexHandle);
}

GridMap::GridMap(Oddlib::Path& path, std::unique_ptr<Oddlib::LvlArchive> archive, Renderer *rend)
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

void GridMap::Render(Renderer* rend, int screenW, int screenH)
{
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[0].size(); y++)
        {
            rend->resetTransform();
            rend->text(40+(x*100), 40+(y*20), mScreens[x][y]->FileName().c_str());
        }
    }
}

