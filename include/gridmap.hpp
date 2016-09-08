#pragma once

#include <memory>
#include <vector>
#include <map>
#include <deque>
#include "core/audiobuffer.hpp"
#include "proxy_nanovg.h"
#include "script.hpp"
#include "oddlib/path.hpp"
#include "fsm.hpp"

struct GuiContext;
class Renderer;
class ResourceLocator;
class InputState;

namespace Oddlib { class LvlArchive; class IBits; }

class Animation;
class Player
{
public:
    void Init(ResourceLocator& locator);
    void Update();
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
    void Input(const InputState& input);
public:
    std::map<std::string, std::unique_ptr<Animation>> mAnims;
    float mXPos = 200.0f;
    float mYPos = 500.0f;
    Animation* mAnim = nullptr;

private: // Actions
    void SetAnimation(const std::string& animation);

    void PlaySoundEffect(const std::string& str)
    {
        LOG_WARNING("TODO: Play: " << str.c_str());
    }

private: // Conditions
    bool IsAnimationComplete() const;
    bool IsAnimationFrameGreaterThan(s32 frameNo) const;

    bool mLeft = false;
    bool mRight = false;

    FiniteStateMachine mStateMachine;
};

class Level
{
public:
    Level(IAudioController& audioController, ResourceLocator& locator);
    void Update();
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
    void EnterState();
    void Input(const InputState& input);
private:
    void RenderDebugPathSelection(Renderer& rend, GuiContext& gui);
    std::unique_ptr<class GridMap> mMap;
    std::unique_ptr<Script> mScript;
    ResourceLocator& mLocator;
    Player mPlayer;
};

class GridScreen
{
public:
    GridScreen(const GridScreen&) = delete;
    GridScreen& operator = (const GridScreen&) = delete;
    GridScreen(const std::string& lvlName, const Oddlib::Path::Camera& camera, Renderer& rend, ResourceLocator& locator);
    ~GridScreen();
    const std::string& FileName() const { return mFileName; }
    int getTexHandle();
    bool hasTexture() const;
    const Oddlib::Path::Camera &getCamera() const { return mCamera; }
private:
    std::string mLvlName;
    std::string mFileName;
    int mTexHandle;

    // TODO: This is not the in-game format
    Oddlib::Path::Camera mCamera;

    // Temp hack to prevent constant reloading of LVLs
    std::unique_ptr<Oddlib::IBits> mCam;

    ResourceLocator& mLocator;
    Renderer& mRend;
};

class GridMap
{
public:
    GridMap(const GridMap&) = delete;
    GridMap& operator = (const GridMap&) = delete;
    GridMap(Oddlib::Path& path, ResourceLocator& locator, Renderer& rend);
    void Update();
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
private:
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;

    std::string mLvlName;

    // Editor stuff
    int mZoomLevel = -10; // 0 is native reso

    // TODO: This is not the in-game format
    std::vector<Oddlib::Path::CollisionItem> mCollisionItems;
    bool mIsAo;
};
