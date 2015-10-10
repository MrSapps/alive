#pragma once

#include <memory>
#include <vector>
#include <deque>
#include "core/audiobuffer.hpp"
#include "gamedata.hpp"
#include "filesystem.hpp"
#include "nanovg.h"

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
    Level(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    void Update();
    void Render(NVGcontext* ctx, int screenW, int screenH);
private:
    void RenderDebugPathSelection();

    std::unique_ptr<MapLoader> mMapLoader;
    std::unique_ptr<class GridMap> mMap;

    GameData& mGameData;
    FileSystem& mFs;
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
