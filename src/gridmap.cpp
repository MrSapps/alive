#include "gridmap.hpp"
#include "imgui/imgui.h"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/path.hpp"

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

void Level::Render(NVGcontext* ctx, int screenW, int screenH)
{
    RenderDebugPathSelection();
    if (mMap)
    {
        mMap->Render(ctx, screenW, screenH);
    }
}

void Level::RenderDebugPathSelection()
{
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
                        mMap = std::make_unique<GridMap>(path);
                    }
                }
            }
        }
    }

    ImGui::End();
}

GridScreen::GridScreen(const std::string& fileName)
    : mFileName(fileName)
{

}

GridMap::GridMap(Oddlib::Path& path)
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
            mScreens[x][y] = std::make_unique<GridScreen>(path.CameraFileName(x,y));
        }
    }
}

void GridMap::Update()
{

}

void GridMap::Render(NVGcontext* ctx, int screenW, int screenH)
{
    for (auto x = 0u; x < mScreens.size(); x++)
    {
        for (auto y = 0u; y < mScreens[0].size(); y++)
        {
            nvgResetTransform(ctx);
            nvgText(ctx, 100+(x*30), 100+(y*30), mScreens[x][y]->FileName().c_str(), nullptr);
        }
    }
}

