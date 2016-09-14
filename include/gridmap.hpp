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


// Same API as Renderer but renders in "virtual" screen coordinates
// and auto scales them to the "real" screen coordinates
class RendererProxy final
{
public:
    RendererProxy(Renderer& rend, s32 virtualW, s32 virtualH, s32 realW, s32 realH, s32 xpos, s32 ypos)
        : mRend(rend),
        mVirtualScreenW(virtualW),
        mVirtualScreenH(virtualH),
        mRealScreenW(realW),
        mRealScreenH(realH),
        mXPos(xpos),
        mYPos(ypos)
    {

    }

    void drawQuad(int texHandle, f32 x, f32 y, f32 w, f32 h, Color color = Color::white(), BlendMode blendMode = BlendMode::normal())
    {
        mRend.drawQuad(texHandle, X(x), Y(y), XNoMove(w), YNoMove(h), color, blendMode);
    }

    void rect(f32 x, f32 y, f32 w, f32 h)
    {
        mRend.rect(X(x), Y(y), XNoMove(w), YNoMove(h));
    }

    void text(f32 x, f32 y, const char *msg)
    {
        mRend.text(X(x), Y(y), msg);
    }

    void moveTo(f32 x, f32 y)
    {
        mRend.moveTo(X(x), Y(y));
    }

    void lineTo(f32 x, f32 y)
    {
        mRend.lineTo(X(x), Y(y));
    }

    int createTexture(GLenum internalFormat, int width, int height, GLenum inputFormat, GLenum colorDataType, const void *pixels, bool interpolation)
    {
        return mRend.createTexture(internalFormat, width, height, inputFormat, colorDataType, pixels, interpolation);
    }

    void destroyTexture(int handle)
    {
        mRend.destroyTexture(handle);
    }

    void beginPath()
    {
        mRend.beginPath();
    }

    void closePath()
    {
        mRend.closePath();
    }

    void resetTransform()
    {
        mRend.resetTransform();
    }

    void stroke()
    {
        mRend.stroke();
    }

    void strokeColor(Color c)
    {
        mRend.strokeColor(c);
    }

    void strokeWidth(f32 size)
    {
        mRend.strokeWidth(size);
    }

private:
    f32 X(f32 x) const
    {
        return ((x- mXPos) / mVirtualScreenW) * mRealScreenW;
    }

    f32 Y(f32 y) const
    {
        return ((y- mYPos) / mVirtualScreenH) * mRealScreenH;
    }

    f32 XNoMove(f32 x) const
    {
        return (x / mVirtualScreenW) * mRealScreenW;
    }

    f32 YNoMove(f32 y) const
    {
        return (y / mVirtualScreenH) * mRealScreenH;
    }

    Renderer& mRend;

    s32 mVirtualScreenW = 0;
    s32 mVirtualScreenH = 0;
    s32 mRealScreenW = 0;
    s32 mRealScreenH = 0;
    s32 mXPos = 0;
    s32 mYPos = 0;
};

class Animation;
class Player
{
public:
    Player(sol::state& luaState, ResourceLocator& locator);
    void Init();
    void Update(const InputState& input);
    void Render(RendererProxy& rend, GuiContext& gui, int x, int y, float scale);
    void Input(const InputState& input);
    static void RegisterLuaBindings(sol::state& state);
private:
    void ScriptLoadAnimations();

    std::map<std::string, std::unique_ptr<Animation>> mAnims;
    float mXPos = 50.0f;
    float mYPos = 100.0f;
    Animation* mAnim = nullptr;
    sol::state& mLuaState;
    sol::table mStates;

    void LoadScript();

private: // Actions
    void SetAnimation(const std::string& animation);

    void PlaySoundEffect(const std::string& str)
    {
        LOG_WARNING("TODO: Play: " << str.c_str());
    }

    bool FacingLeft() const { return mFlipX; }
    bool FacingRight() const { return !FacingLeft(); }
    void FlipXDirection() { mFlipX = !mFlipX; }
private: 
    bool IsLastFrame() const;
    s32 FrameNumber() const;
    bool mFlipX = false;

    ResourceLocator& mLocator;
};

class Level
{
public:
    Level(IAudioController& audioController, ResourceLocator& locator, sol::state& luaState);
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
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
private:
    void RenderEditor(Renderer& rend, GuiContext& gui, int screenW, int screenH);
    void RenderGame(Renderer& rend, GuiContext& gui, int screenW, int screenH);

    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;

    std::string mLvlName;

    // Editor stuff
    int mZoomLevel = -10; // 0 is native reso

    // TODO: This is not the in-game format
    std::vector<Oddlib::Path::CollisionItem> mCollisionItems;
    bool mIsAo;

    Player mPlayer;

    enum class eStates
    {
        eInGame,
        eEditor
    };
    eStates mState = eStates::eInGame;
};
