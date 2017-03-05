#pragma once

#include <memory>
#include <vector>
#include <map>
#include <deque>
#include <iomanip>
#include <sstream>
#include "core/audiobuffer.hpp"
#include "proxy_nanovg.h"
#include "oddlib/path.hpp"
#include "fsm.hpp"
#include "proxy_sol.hpp"
#include "renderer.hpp"
#include "collisionline.hpp"
#include "proxy_squall.hpp"
#include "proxy_sqrat.hpp"

struct GuiContext;
class Renderer;
class ResourceLocator;
class InputState;

namespace Oddlib { class LvlArchive; class IBits; }


class Animation;


struct ObjRect
{
    s32 x;
    s32 y;
    s32 w;
    s32 h;

    static void RegisterScriptBindings(sol::state& state)
    {
        Sqrat::Class<ObjRect> c(Sqrat::DefaultVM::Get(), "ObjRect");
        c.Var("x", &ObjRect::x)
         .Var("y", &ObjRect::y)
         .Var("w", &ObjRect::h)
         .Var("h", &ObjRect::h)
         .Ctor();

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
    void Render(Renderer& rend, GuiContext& gui, int x, int y, float scale, int layer) const;
    void ReloadScript();
    static void RegisterScriptBindings(sol::state& state);

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
    void Update(const InputState& input, CoordinateSpace& coords);
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
    void LoadTextures();
    bool hasTexture() const;
    const Oddlib::Path::Camera &getCamera() const { return mCamera; }
    void Render(float x, float y, float w, float h);
private:
    std::string mLvlName;
    std::string mFileName;
    int mTexHandle;
    int mTexHandle2; 

    // TODO: This is not the in-game format
    Oddlib::Path::Camera mCamera;

    // Temp hack to prevent constant reloading of LVLs
    std::unique_ptr<Oddlib::IBits> mCam;

    ResourceLocator& mLocator;
    Renderer& mRend;
};


#define NO_MOVE_OR_MOVE_ASSIGN(x)  x(x&&) = delete; x& operator = (x&&) = delete

u32 NextId();

template<class T>
u32 GenerateTypeId()
{
    static u32 id = NextId();
    return id;
}


class ICommand
{
public:
    NO_MOVE_OR_MOVE_ASSIGN(ICommand);
    ICommand() = default;
    virtual void Redo() = 0;
    virtual void Undo() = 0;
    virtual std::string Message() = 0;
    virtual ~ICommand() = default;
    virtual u32 Id() const = 0;
    virtual bool CanMerge() const { return false; }
};

template<class T>
class ICommandWithId : public ICommand
{
public:
    NO_MOVE_OR_MOVE_ASSIGN(ICommandWithId);
    ICommandWithId()
    {
        mId = GenerateTypeId<T>();
    }
    virtual u32 Id() const override { return mId; }
private:
    u32 mId;
};

class Selection
{
public:
    std::set<s32> Clear(CollisionLines& items);
    void Select(CollisionLines& items, s32 idx, bool select);
    bool HasSelection() const { return mSelectedLines.empty() == false; }
    const std::set<s32>& SelectedLines() const { return mSelectedLines; }
private:
    std::set<s32> mSelectedLines;
};

class CommandSelectOrDeselectLine : public ICommandWithId<CommandSelectOrDeselectLine>
{
public:
    NO_MOVE_OR_MOVE_ASSIGN(CommandSelectOrDeselectLine);

    CommandSelectOrDeselectLine(CollisionLines& lines, Selection& selection, s32 idx, bool select) 
        : mLines(lines), mSelection(selection), mIdx(idx), mSelect(select) { }

    virtual void Redo() final
    {
        mSelection.Select(mLines, mIdx, mSelect);
    }

    virtual void Undo() final
    {
        mSelection.Select(mLines, mIdx, !mSelect);
    }

    virtual std::string Message() final
    {
        if (mSelect)
        {
            return "Select line " + std::to_string(mIdx);
        }
        else
        {
            return "De select line " + std::to_string(mIdx);
        }
    }
private:
    CollisionLines& mLines;
    Selection& mSelection;
    const s32 mIdx;
    const bool mSelect;
};

class CommandClearSelection : public ICommandWithId<CommandClearSelection>
{
public:
    NO_MOVE_OR_MOVE_ASSIGN(CommandClearSelection);

    CommandClearSelection(CollisionLines& lines, Selection& selection)
        : mLines(lines), mSelection(selection) { }

    virtual void Redo() final
    {
        mOldSelection = mSelection.Clear(mLines);
    }

    virtual void Undo() final
    {
        for (s32 idx : mOldSelection)
        {
            mSelection.Select(mLines, idx, true);
        }
    }

    virtual std::string Message() final
    {
        return "Clear selection";
    }

private:
    CollisionLines& mLines;
    Selection& mSelection;
    std::set<s32> mOldSelection;
};

inline std::string FormatVec2(const glm::vec2& vec)
{
    std::ostringstream out;
    out << "("
        << static_cast<s32>(vec.x)
        << ","
        << static_cast<s32>(vec.y)
        << ")";
    return out.str();
}

class CommandMoveLinePoint : public ICommandWithId<CommandMoveLinePoint>
{
public:
    NO_MOVE_OR_MOVE_ASSIGN(CommandMoveLinePoint);

    CommandMoveLinePoint(CollisionLines& lines, Selection& selection, const glm::vec2& newPos, bool moveP1)
        : mLines(lines), mSelection(selection), mNewPos(newPos), mApplyToP1(moveP1)
    {
        mOldPos = Point();
    }

    void Merge(CollisionLines& /*lines*/, Selection& /*selection*/, const glm::vec2& newPos, bool /*moveP1*/)
    {
        mNewPos = newPos;
    }

    virtual void Redo() final
    {
        Point() = mNewPos;
    }

    virtual void Undo() final
    {
        Point() = mOldPos;
    }

    virtual bool CanMerge() const final { return true; }

    virtual std::string Message() final
    {
        return "Move line point " + std::to_string(mApplyToP1 ? 1 : 2) + " from " + FormatVec2(mOldPos) + " to " + FormatVec2(mNewPos);
    }
private:
    glm::vec2& Point()
    {
        const auto& line = mLines[*mSelection.SelectedLines().begin()];
        return mApplyToP1 ? line->mLine.mP1 : line->mLine.mP2;
    }

    CollisionLines& mLines;
    Selection& mSelection;
    glm::vec2 mOldPos;
    glm::vec2 mNewPos;
    bool mApplyToP1;
};


class MoveSelection : public ICommandWithId<MoveSelection>
{
public:
    NO_MOVE_OR_MOVE_ASSIGN(MoveSelection);

    MoveSelection(CollisionLines& lines, Selection& selection, const glm::vec2& delta)
        : mLines(lines), mSelection(selection), mDelta(delta)
    {

    }

    void Merge(CollisionLines& /*lines*/, Selection& /*selection*/, const glm::vec2& delta)
    {
        mDelta += delta;
    }

    virtual void Redo() final
    {
        for (s32 idx : mSelection.SelectedLines())
        {
            mLines[idx]->mLine.mP1 += mDelta;
            mLines[idx]->mLine.mP2 += mDelta;
        }
    }

    virtual void Undo() final
    {
        for (s32 idx : mSelection.SelectedLines())
        {
            mLines[idx]->mLine.mP1 -= mDelta;
            mLines[idx]->mLine.mP2 -= mDelta;
        }
    }

    virtual std::string Message() final
    {
        return "Move selection by " + FormatVec2(mDelta);
    }

    virtual bool CanMerge() const final
    {
        return true;
    }
private:
    CollisionLines& mLines;
    Selection& mSelection;
    glm::vec2 mDelta;
};

class UndoStack
{
public:
    UndoStack(s32 stackLimit = -1)
        : mStackLimit(stackLimit)
    {

    }

    template<class T, class... Args>
    void PushMerge(bool shouldMerge, Args&&... args)
    {
        // If the last item on the stack is of the same type we are about to create
        if (shouldMerge && !mUndoStack.empty() && GenerateTypeId<T>() == mUndoStack.back()->Id() && mUndoStack.back()->CanMerge())
        {
            // Undo the last action
            mUndoStack.back()->Undo();

            // Update its internals with the new target data
            static_cast<T*>(mUndoStack.back().get())->Merge(std::forward<Args>(args)...);

            // Re-apply with new internals
            mUndoStack.back()->Redo();
        }
        else
        {
            Push<T>(std::forward<Args>(args)...);
        }
    }

    template<class T, class... Args>
    void Push(Args&&... args)
    {
        // If the active index isn't the latest item then remove everything after it
        if (mCommandIndex != Count())
        {
            mUndoStack.erase(mUndoStack.begin() + mCommandIndex, mUndoStack.end());
        }

        // Apply action of new command and add to stack
        auto cmd = std::make_unique<T>(std::forward<Args>(args)...);
        mUndoStack.emplace_back(std::move(cmd));
        mUndoStack.back()->Redo();

        // If we are over the stack limit remove the first item to stay within the limit
        if (mStackLimit != -1 && static_cast<s32>(Count()) > mStackLimit)
        {
            mUndoStack.erase(mUndoStack.begin(), mUndoStack.begin() + 1);
        }
        else
        {
            mCommandIndex++;
        }
    }

    void Undo();
    void Redo();
    void Clear();
    u32 Count() const { return static_cast<u32>(mUndoStack.size()); }
    void DebugRenderCommandList(GuiContext& gui) const;
private:
    std::vector<std::unique_ptr<ICommand>> mUndoStack;
    u32 mCommandIndex = 0;
    s32 mStackLimit = -1;
};

class GridMap : public IMap
{
public:
    GridMap(const GridMap&) = delete;
    GridMap& operator = (const GridMap&) = delete;
    GridMap(Oddlib::Path& path, ResourceLocator& locator, sol::state& luaState, Renderer& rend);
    void Update(const InputState& input, CoordinateSpace& coords);
    void Render(Renderer& rend, GuiContext& gui) const;
private:

    void UpdateToEditorOrToGame(const InputState& input, CoordinateSpace& coords);
    void UpdateEditor(const InputState& input, CoordinateSpace& coords);
    void UpdateGame(const InputState& input, CoordinateSpace& coords);

    MapObject* GetMapObject(s32 x, s32 y, const char* type);
    void ActivateObjectsWithId(MapObject* from, s32 id, bool direction);
    void RenderDebug(Renderer& rend) const;
    
    void RenderToEditorOrToGame(Renderer& rend, GuiContext& gui) const;
    void RenderEditor(Renderer& rend, GuiContext& gui) const;
    void RenderGame(Renderer& rend, GuiContext& gui) const;

    virtual const CollisionLines& Lines() const override final { return mCollisionItems; }

    void DebugRayCast(Renderer& rend, const glm::vec2& from, const glm::vec2& to, u32 collisionType, const glm::vec2& fromDrawOffset = glm::vec2()) const;


    glm::vec2 kVirtualScreenSize;
    glm::vec2 kCameraBlockSize;
    glm::vec2 kCameraBlockImageOffset;
    std::deque<std::deque<std::unique_ptr<GridScreen>>> mScreens;
    
    std::string mLvlName;

    // Editor stuff
    f32 mEditorCamZoom = 1.0f;
    const int mEditorGridSizeX = 25;
    const int mEditorGridSizeY = 20;
    u32 mModeSwitchTimeout = 0;

    Selection mSelection;
    UndoStack mUndoStack;
    bool mMergeCommand = false;
    glm::vec2 mLastMousePos;
    enum class eSelectionState
    {
        eNone,
        eLineP1Selected,
        eLineP2Selected,
        eLineMiddleSelected,
        eMoveSelected
    };
    eSelectionState mSelectionState = eSelectionState::eNone;

    /* TODO: Set correct cursors
    enum class eMouseCursor
    {
        eArrow,
        eOpenHand,
        eClosedHand,
    };
    eMouseCursor mMouseCursor = eMouseCursor::eArrow;
    */

    // CollisionLine contains raw pointers to other CollisionLine objects. Hence the vector
    // has unique_ptrs so that adding or removing to this vector won't cause the raw pointers to dangle.
    CollisionLines mCollisionItems;

    bool mIsAo;

    MapObject mPlayer;
    std::vector<std::unique_ptr<MapObject>> mObjs;

    enum class eStates
    {
        eInGame,
        eToEditor,
        eEditor,
        eToGame
    };
    eStates mState = eStates::eInGame;

    void ConvertCollisionItems(const std::vector<Oddlib::Path::CollisionItem>& items);

    glm::vec2 mCameraPosition;

};