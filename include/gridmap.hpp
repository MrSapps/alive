#pragma once

#include <memory>
#include <vector>
#include <deque>
#include "core/audiobuffer.hpp"
#include "gamedata.hpp"
#include "filesystem.hpp"
#include "proxy_nanovg.h"
#include "script.hpp"
#include "oddlib/path.hpp"

struct GuiContext;
class Renderer;
namespace Oddlib { class LvlArchive; }
class Level
{
public:
    Level(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    void Update();
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
private:
    void RenderDebugPathSelection(Renderer& rend, GuiContext& gui);
    std::unique_ptr<class GridMap> mMap;
    std::unique_ptr<Script> mScript;
    GameData& mGameData;
    FileSystem& mFs;
};

class GridScreen
{
public:
    GridScreen(const GridScreen&) = delete;
    GridScreen& operator = (const GridScreen&) = delete;
    GridScreen(const std::string& lvlName, const Oddlib::Path::Camera& camera, Renderer& rend);
    ~GridScreen();
    const std::string& FileName() const { return mFileName; }
    int getTexHandle(FileSystem& fs);
    bool hasTexture() const;
    const Oddlib::Path::Camera &getCamera() const { return mCamera; }
private:
    std::string mLvlName;
    std::string mFileName;
    int mTexHandle;
    Renderer& mRend;

    // TODO: This is not the in-game format
    Oddlib::Path::Camera mCamera;
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

    FileSystem& mFs;
    std::string mLvlName;

    // Editor stuff
    int mZoomLevel = -10; // 0 is native reso

    // TODO: This is not the in-game format
    std::vector<Oddlib::Path::CollisionItem> mCollisionItems;
    bool mIsAo = false;
};
