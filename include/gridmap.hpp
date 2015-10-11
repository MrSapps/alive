#pragma once

#include <memory>
#include <vector>
#include <deque>
#include "core/audiobuffer.hpp"
#include "gamedata.hpp"
#include "filesystem.hpp"
#include "nanovg.h"

namespace Oddlib { class Path; class LvlArchive; }

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
    GridScreen(const std::string& fileName, Oddlib::LvlArchive& archive);
    const std::string& FileName() const { return mFileName; }
private:
    std::string mFileName;
};

class GridMap
{
public:
    GridMap(Oddlib::Path& path, std::unique_ptr<Oddlib::LvlArchive> archive);
    void Update();
    void Render(NVGcontext* ctx, int screenW, int screenH);
private:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;

    // TODO: Should be owned and ref counted by something else
    std::unique_ptr<Oddlib::LvlArchive> mArchive;
};
