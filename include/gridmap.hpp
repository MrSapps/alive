#pragma once

#include <memory>
#include <vector>
#include <deque>
#include "core/audiobuffer.hpp"
#include "gamedata.hpp"
#include "filesystem.hpp"
#include "nanovg.h"

namespace Oddlib { class Path; }

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
    explicit GridScreen(const std::string& fileName);
    const std::string& FileName() const { return mFileName; }
private:
    //std::vector<std::unique_ptr<class MapObject>> mObjects;
    std::string mFileName;
};

class GridMap
{
public:
    explicit GridMap(Oddlib::Path& path);
    void Update();
    void Render(NVGcontext* ctx, int screenW, int screenH);
private:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;
    // std::vector<std::unique_ptr<class MapObject>> mObjects;
};
