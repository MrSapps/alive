#pragma once

#include <memory>
#include <vector>
#include <deque>
#include "core/audiobuffer.hpp"
#include "gamedata.hpp"
#include "filesystem.hpp"
#include "nanovg.h"
#include "imgui/imgui.h"

class GridScreen
{
public:

private:
    //std::vector<std::unique_ptr<class MapObject>> mObjects;
};

// Asynchronously loads from either old format path binary chunk or new engine path format
class MapLoader
{
public:

};

class Level
{
public:
    Level(GameData& gameData, IAudioController& audioController, FileSystem& fs)
        : mGameData(gameData)
    {

    }



    void Update()
    {

    }

    void Render(NVGcontext* ctx, int screenW, int screenH)
    {
        nvgText(ctx, 100, 100, "Level rendering test", nullptr);

        ImGui::Begin("Paths", nullptr, ImVec2(400, 200), 1.0f, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

        static std::vector<std::string> items;
        if (items.empty())
        {
            for (const auto& pathGroup : mGameData.Paths())
            {
                for (const auto& path : pathGroup.second)
                {
                    items.push_back(pathGroup.first + " " + std::to_string(path.mPathChunkId));
                }
            }
        }
     
        for (const auto& item : items)
        {
            ImGui::Selectable(item.c_str());
        }

        ImGui::End();

    }

    std::unique_ptr<MapLoader> mMapLoader;
    std::unique_ptr<class GridMap> mMap;

private:
    GameData& mGameData;
};

class GridMap
{
public:
    GridMap() = default;

    GridMap(int xSize, int ySize, int gridX, int gridY);

    void Add();
    void Remove();

private:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;
   // std::vector<std::unique_ptr<class MapObject>> mObjects;
};
