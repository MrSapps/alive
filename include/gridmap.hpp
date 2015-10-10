#pragma once

#include <memory>
#include <vector>
#include <deque>
#include "core/audiobuffer.hpp"
#include "gamedata.hpp"
#include "filesystem.hpp"
#include "nanovg.h"


class Level
{
public:
    Level(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    void Update();
    void Render(NVGcontext* ctx, int screenW, int screenH);
private:
    void RenderDebugPathSelection();

    std::unique_ptr<class GridMap> mMap;
    GameData& mGameData;
    FileSystem& mFs;
};

class GridScreen
{
public:

private:
    //std::vector<std::unique_ptr<class MapObject>> mObjects;
};

class GridMap
{
public:
    GridMap(Oddlib::IStream& pathChunkStream, const GameData::PathEntry& pathSettings);
    void Update();
    void Render(NVGcontext* ctx, int screenW, int screenH);
private:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;
    // std::vector<std::unique_ptr<class MapObject>> mObjects;
};
