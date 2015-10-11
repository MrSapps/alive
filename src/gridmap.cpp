#include "gridmap.hpp"
//#include "imgui/imgui.h"
#include "oddlib/lvlarchive.hpp"
#include "renderer.hpp"

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
                        mMap = std::make_unique<GridMap>(*chunkStream, *entry);
                    }
                }
            }
        }
    }

    ImGui::End();
#endif
}

GridMap::GridMap(Oddlib::IStream& pathChunkStream, const GameData::PathEntry& pathSettings)
{
    mScreens.resize(pathSettings.mMapXSize);
    for (auto& col : mScreens)
    {
        col.resize(pathSettings.mMapYSize);
    }
}

void GridMap::Update()
{

}

void GridMap::Render(Renderer* rend, int screenW, int screenH)
{
    std::string str = "Level rendering test (" + std::to_string(mScreens.size()) + "," + std::to_string(mScreens[0].size()) + ")";
    rend->drawText(100, 100, str.c_str());
}
