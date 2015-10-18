#pragma once

#include <memory>
#include <vector>
#include <deque>
#include "core/audiobuffer.hpp"
#include "gamedata.hpp"
#include "filesystem.hpp"
#include "nanovg.h"

struct GuiContext;
class Renderer;
namespace Oddlib { class Path; class LvlArchive; }
class Level
{
public:
    Level(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    void Update();
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
private:
    void RenderDebugPathSelection(Renderer& rend, GuiContext& gui);

    std::unique_ptr<class GridMap> mMap;
    GameData& mGameData;
    FileSystem& mFs;
};

class GridScreen
{
public:
    GridScreen(const GridScreen&) = delete;
    GridScreen& operator = (const GridScreen&) = delete;
    GridScreen(const std::string& lvlName, const std::string& fileName, Renderer& rend);
    ~GridScreen();
    const std::string& FileName() const { return mFileName; }
    int getTexHandle(FileSystem& fs);
private:
    std::string mLvlName;
    std::string mFileName;
    int mTexHandle;
    Renderer& mRend;
};

class GridMap
{
public:
    GridMap(const GridMap&) = delete;
    GridMap& operator = (const GridMap&) = delete;
    GridMap(const std::string& lvlName, Oddlib::Path& path, FileSystem& fs, Renderer& rend);
    void Update();
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
private:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;
    int mEditorScreenX = -1;
    int mEditorScreenY = -1;


    FileSystem& mFs;
    std::string mLvlName;
};
