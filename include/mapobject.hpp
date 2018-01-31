#pragma once

#include <string>
#include <memory>
#include "types.hpp"
#include "logger.hpp"
#include "iterativeforloop.hpp"

float SnapXToGrid(float toSnap);

struct ObjRect
{
    s32 x;
    s32 y;
    s32 w;
    s32 h;
};

namespace Oddlib
{
    class IStream;
}

class InputState;
class AbstractRenderer;
class Animation;
class ResourceLocator;
class IMap;

struct CollisionResult
{
    bool collision;
    float x;
    float y;
    float distance;
};

using UP_MapObject = std::unique_ptr<class MapObject>;
// using IterativeForLoopSQInteger = IterativeForLoop<SQInteger>;

class MapObject
{
public:
    MapObject() = delete;
    MapObject(ResourceLocator&, const ObjRect&)
    {
    }

    MapObject(const MapObject&) = delete;
    MapObject(MapObject&& other) = delete;
   
    MapObject& operator = (const MapObject&) = delete;
    MapObject& operator = (MapObject&& other) = delete;
    ~MapObject();

    void LoadAnimation(const std::string& name);

    bool Init();
    void Update(const InputState& input);
    void Render(AbstractRenderer& rend, int x, int y, float scale, int layer) const;
    void ReloadScript();

    bool ContainsPoint(s32 x, s32 y) const;
    std::string Name() const { return ""; }

    // TODO: Shouldn't be part of this object
    void SnapXToGrid();

    s32 Id() const { return 0; }
    bool WallCollision(IMap& map, f32 dx, f32 dy) const;
    bool CellingCollision(IMap& map, f32 dx, f32 dy) const;

    CollisionResult FloorCollision(IMap& map) const;

    MapObject* AddChildObject();
    u32 ChildCount() const;
    void RemoveChild(u32 index);
private:
    class Loader
    {
    public:
        Loader(MapObject& obj);
        bool Load();
        void LoadAnimations();
        void LoadSounds();
    private:
        enum class LoaderStates
        {
            eInit,
            eLoadAnimations,
            eLoadSounds
        };
        void SetState(LoaderStates state);
    };

    void LoadScript();
private: // Actions
    bool AnimationComplete() const;
    void SetAnimation(const std::string& animation);
    void SetAnimationFrame(s32 frame);
    void SetAnimationAtFrame(const std::string& animation, u32 frame);
    bool FacingLeft() const { return false; }
    bool FacingRight() const { return false; }
    void FlipXDirection() {}
private:
    bool AnimUpdate();
    s32 FrameCounter() const;
    s32 NumberOfFrames() const;
    bool IsLastFrame() const;
    s32 FrameNumber() const;
};
