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

using CollisionLines = std::vector<std::unique_ptr<class CollisionLine>>;

class CollisionLine
{
public:

    glm::vec2 mP1;
    glm::vec2 mP2;

    enum eLineTypes : u32
    {
        eFloor = 0,
        eWallLeft = 1,
        eWallRight = 2,
        eCeiling = 3,
        eBackGroundFloor = 4,
        eBackGroundWallLeft = 5,
        eBackGroundWallRight = 6,
        eBackGroundCeiling = 7,
        eFlyingSligLine = 8,
        eArt = 9,
        eBulletWall = 10,
        eMineCarFloor = 11,
        eMineCarWall = 12,
        eMineCarCeiling = 13,
        eFlyingSligCeiling = 17,
        eUnknown = 99
    };
    eLineTypes mType = eFloor;

    struct Link
    {
        CollisionLine* mPrevious = nullptr;
        CollisionLine* mNext = nullptr;
    };

    Link mLink;
    Link mOptionalLink;

    CollisionLine() = default;
    CollisionLine(glm::vec2 p1, glm::vec2 p2, eLineTypes type)
        : mP1(p1), mP2(p2), mType(type)
    {

    }

    static eLineTypes ToType(u16 type, bool isAo);
    static void Render(Renderer& rend, const CollisionLines& lines);

    template<u32 N>
    static bool RayCast(const CollisionLines& lines, const glm::vec2& line1p1, const glm::vec2& line1p2, u32 const (&collisionTypes)[N], Physics::raycast_collision* const collision)
    {
        const CollisionLine* nearestLine = nullptr;
        float nearestCollisionX = 0;
        float nearestCollisionY = 0;
        float nearestDistance = 0.0f;

        for (const std::unique_ptr<CollisionLine>& line : lines)
        {
            bool found = false;
            for (u32 type : collisionTypes)
            {
                if (type == line->mType)
                {
                    found = true;
                    break;
                }
            }

            if (!found) { continue; }

            const float line1p1x = line1p1.x;
            const float line1p1y = line1p1.y;
            const float line1p2x = line1p2.x;
            const float line1p2y = line1p2.y;

            const float line2p1x = line->mP1.x;
            const float line2p1y = line->mP1.y;
            const float line2p2x = line->mP2.x;
            const float line2p2y = line->mP2.y;

            // Get the segments' parameters.
            const float dx12 = line1p2x - line1p1x;
            const float dy12 = line1p2y - line1p1y;
            const float dx34 = line2p2x - line2p1x;
            const float dy34 = line2p2y - line2p1y;

            // Solve for t1 and t2
            const float denominator = (dy12 * dx34 - dx12 * dy34);
            if (denominator == 0.0f) { continue; }

            const float t1 = ((line1p1x - line2p1x) * dy34 + (line2p1y - line1p1y) * dx34) / denominator;
            const float t2 = ((line2p1x - line1p1x) * dy12 + (line1p1y - line2p1y) * dx12) / -denominator;

            // Find the point of intersection.
            const float intersectionX = line1p1x + dx12 * t1;
            const float intersectionY = line1p1y + dy12 * t1;

            // The segments intersect if t1 and t2 are between 0 and 1.
            const bool hasCollided = ((t1 >= 0) && (t1 <= 1) && (t2 >= 0) && (t2 <= 1));

            if (hasCollided)
            {
                const float distance = glm::sqrt(powf((line1p1x - intersectionX), 2) + powf((line1p1y - intersectionY), 2));
                if (!nearestLine || distance < nearestDistance)
                {
                    nearestCollisionX = intersectionX;
                    nearestCollisionY = intersectionY;
                    nearestDistance = distance;
                    nearestLine = line.get();
                }
            }
        }

        if (nearestLine)
        {
            if (collision)
            {
                collision->intersection.x = nearestCollisionX;
                collision->intersection.y = nearestCollisionY;
            }
            return true;
        }

        return false;
    }

    struct LineData
    {
        std::string mName;
        ColourU8 mColour;
    };
    static const std::map<eLineTypes, LineData> mData;
};



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

class IMap
{
public:
    virtual ~IMap() = default;
    virtual const CollisionLines& Lines() const = 0;
};

class MapObject
{
public:
    MapObject(MapObject&&) = delete;
    MapObject& operator = (MapObject&&) = delete;
    MapObject(IMap& map, sol::state& luaState, ResourceLocator& locator, const ObjRect& rect);
    MapObject(IMap& map, sol::state& luaState, ResourceLocator& locator, const std::string& scriptName);
    void Init();
    void GetName();
    void Init(const ObjRect& rect, Oddlib::IStream& objData);
    void Update(const InputState& input);
    void Render(Renderer& rend, GuiContext& gui, int x, int y, float scale);
    void ReloadScript();
    static void RegisterLuaBindings(sol::state& state);

    bool ContainsPoint(s32 x, s32 y) const;
    const std::string& Name() const { return mName; }

    // TODO: Shouldn't be part of this object
    void SnapXToGrid();

    float mXPos = 50.0f;
    float mYPos = 100.0f;
    s32 Id() const { return mId; }
    void Activate(bool direction);
    bool WallCollision(f32 dx, f32 dy) const;
    bool CellingCollision(f32 dx, f32 dy) const;
    std::tuple<bool, f32, f32, f32> FloorCollision() const;
private:
    void ScriptLoadAnimations();
    IMap& mMap;

    std::map<std::string, std::unique_ptr<Animation>> mAnims;
    Animation* mAnim = nullptr;
    sol::state& mLuaState;
    sol::table mStates;

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
};

class Level
{
public:
    Level(Level&&) = delete;
    Level& operator = (Level&&) = delete;
    Level(IAudioController& audioController, ResourceLocator& locator, sol::state& luaState, Renderer& render);
    void Update(const InputState& input);
    void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
    void EnterState();
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

class GridMap : public IMap
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

    virtual const CollisionLines& Lines() const override final { return mCollisionItems; }

    void DebugRayCast(Renderer& rend, const glm::vec2& from, const glm::vec2& to, u32 collisionType, const glm::vec2& fromDrawOffset = glm::vec2());

    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;
    
    std::string mLvlName;

    // Editor stuff
    glm::vec2 mEditorCamOffset;
    int mEditorCamZoom = 5;
    const int mEditorGridSizeX = 25;
    const int mEditorGridSizeY = 20;

    // CollisionLine contains raw pointers to other CollisionLine objects. Hence the vector
    // has unique_ptrs so that adding or removing to this vector won't cause the raw pointers to dangle.
    CollisionLines mCollisionItems;

    bool mIsAo;

    MapObject mPlayer;
    std::vector<std::unique_ptr<MapObject>> mObjs;

    enum class eStates
    {
        eInGame,
        eEditor
    };
    eStates mState = eStates::eInGame;

    void ConvertCollisionItems(const std::vector<Oddlib::Path::CollisionItem>& items);
};