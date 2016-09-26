#pragma once

#include <memory>
#include <vector>
#include <map>
#include <deque>
#include "core/audiobuffer.hpp"
#include "proxy_nanovg.h"
#include "oddlib/path.hpp"
#include "fsm.hpp"
#include "proxy_sol.hpp"
#include "renderer.hpp"

struct GuiContext;
class Renderer;
class ResourceLocator;
class InputState;

namespace Oddlib { class LvlArchive; class IBits; }

namespace Physics
{
    struct raycast_collision
    {
        glm::vec2 intersection;
    };

    bool raycast_lines(const glm::vec2& line1p1, const glm::vec2& line1p2, const glm::vec2& line2p1, const glm::vec2& line2p2, raycast_collision * collision);
}

class Animation;


struct ObjRect
{
    s32 x;
    s32 y;
    s32 w;
    s32 h;

    static void RegisterLuaBindings(sol::state& state)
    {
        state.new_usertype<ObjRect>("ObjRect",
            "x", &ObjRect::x,
            "y", &ObjRect::y,
            "w", &ObjRect::h,
            "h", &ObjRect::h);
    }
};

class MapObject
{
public:
    MapObject(sol::state& luaState, ResourceLocator& locator, const std::string& scriptName);
    void Init();
    void Init(const ObjRect& rect, Oddlib::IStream& objData);
    void Update(const InputState& input);
    void Render(Renderer& rend, GuiContext& gui, int x, int y, float scale);
    void Input(const InputState& input);
    static void RegisterLuaBindings(sol::state& state);

    bool ContainsPoint(s32 x, s32 y) const;
    const std::string& Name() const { return mName; }

    // TODO: Shouldn't be part of this object
    void SnapToGrid();

    float mXPos = 50.0f;
    float mYPos = 100.0f;
    s32 Id() const { return mId; }
    void Activate(bool direction);
private:
    void ScriptLoadAnimations();

    std::map<std::string, std::unique_ptr<Animation>> mAnims;
    Animation* mAnim = nullptr;
    sol::state& mLuaState;
    sol::table mStates;

    void LoadScript(const ObjRect* rect, Oddlib::IStream* objData);
private: // Actions
    void SetAnimation(const std::string& animation);
    void SetAnimationAtFrame(const std::string& animation, u32 frame);
    bool FacingLeft() const { return mFlipX; }
    bool FacingRight() const { return !FacingLeft(); }
    void FlipXDirection() { mFlipX = !mFlipX; }
private: 
    bool AnimUpdate();
    s32 FrameCounter() const;
    s32 NumberOfFrames() const;
    bool IsLastFrame() const;
    s32 FrameNumber() const;
    bool mFlipX = false;

    ResourceLocator& mLocator;
    std::string mScriptName;
    std::string mName;
    s32 mId = 0;
};

class Level
{
public:
    Level(IAudioController& audioController, ResourceLocator& locator, sol::state& luaState, Renderer& render);
    void Update(const InputState& input);
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
    void EnterState();
    void Input(const InputState& input);
private:
    void RenderDebugPathSelection(Renderer& rend, GuiContext& gui);
    std::unique_ptr<class GridMap> mMap;
    ResourceLocator& mLocator;
    sol::state& mLuaState;
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
    GridMap(Oddlib::Path& path, ResourceLocator& locator, sol::state& luaState, Renderer& rend);
    void Update(const InputState& input);
    void Render(Renderer& rend, GuiContext& gui);
private:
    MapObject* GetMapObject(s32 x, s32 y, const char* type);
    void ActivateObjectsWithId(MapObject* from, s32 id, bool direction);
    void RenderDebug(Renderer& rend);
    void RenderEditor(Renderer& rend, GuiContext& gui);
    void RenderGame(Renderer& rend, GuiContext& gui);

    bool raycast_map(const glm::vec2& line1p1, const glm::vec2& line1p2, int collisionType, Physics::raycast_collision * collision);

    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;
    
    std::string mLvlName;

    // Editor stuff
    glm::vec2 mEditorCamOffset;
    int mEditorCamZoom = 5;
    const int mEditorGridSizeX = 25;
    const int mEditorGridSizeY = 20;

    // TODO: This is not the in-game format
    std::vector<Oddlib::Path::CollisionItem> mCollisionItems;
    std::vector<Oddlib::Path::CollisionItem> mCollisionItemsSorted;
    bool mIsAo;

    MapObject mPlayer;
    std::vector<std::unique_ptr<MapObject>> mObjs;

    enum class eStates
    {
        eInGame,
        eEditor
    };
    eStates mState = eStates::eInGame;
};