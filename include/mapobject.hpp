#pragma once

#include <string>
#include "types.hpp"
#include "proxy_sqrat.hpp"
#include "logger.hpp"

struct ObjRect
{
    s32 x;
    s32 y;
    s32 w;
    s32 h;

    static void RegisterScriptBindings()
    {
        Sqrat::Class<ObjRect> c(Sqrat::DefaultVM::Get(), "ObjRect");
        c.Var("x", &ObjRect::x)
            .Var("y", &ObjRect::y)
            .Var("w", &ObjRect::h)
            .Var("h", &ObjRect::h)
            .Ctor();
        Sqrat::RootTable().Bind("ObjRect", c);
    }
};

namespace Oddlib
{
    class IStream;
}

class InputState;
class Renderer;
class Animation;
struct GuiContext;
class ResourceLocator;
class IMap;

struct CollisionResult
{
    bool collision;
    float x;
    float y;
    float distance;
};

class MapObject
{
public:
    MapObject() = delete;
    MapObject(ResourceLocator& locator, const ObjRect& rect)
        : mLocator(locator), mRect(rect)
    {
    }
    MapObject(const MapObject&) = delete;
    MapObject(MapObject&& other) = delete;
   
    MapObject& operator = (const MapObject&) = delete;
    MapObject& operator = (MapObject&& other) = delete;
    ~MapObject();

    void LoadAnimation(const std::string& name);

    void SetScriptInstance(Sqrat::Object obj)
    {
        TRACE_ENTRYEXIT;
        mScriptObject = obj;
    }

    void Init();
    void Update(const InputState& input);
    void Render(Renderer& rend, GuiContext& gui, int x, int y, float scale, int layer) const;
    void ReloadScript();
    static void RegisterScriptBindings();

    bool ContainsPoint(s32 x, s32 y) const;
    const std::string& Name() const { return mName; }

    // TODO: Shouldn't be part of this object
    void SnapXToGrid();

    float mXPos = 50.0f;
    float mYPos = 100.0f;

    s32 Id() const { return mId; }
    bool WallCollision(IMap& map, f32 dx, f32 dy) const;
    bool CellingCollision(IMap& map, f32 dx, f32 dy) const;

    CollisionResult FloorCollision(IMap& map) const;
private:
    void ScriptLoadAnimations();
    std::map<std::string, std::shared_ptr<Animation>> mAnims;
    Animation* mAnim = nullptr;

    void LoadScript();
private: // Actions
    bool AnimationComplete() const;
    void SetAnimation(const std::string& animation);
    void SetAnimationFrame(s32 frame);
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
public:
    bool mFlipX = false;
private:
    ResourceLocator& mLocator;
    std::string mScriptName;
    std::string mName;
    s32 mId = 0;
    ObjRect mRect;
    Sqrat::Object mScriptObject; // Derived script object instance
};
