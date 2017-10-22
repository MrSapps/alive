#pragma once

#include "gridmap.hpp"

class WorldState;

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

u32 NextId();

template<class T>
u32 GenerateTypeId()
{
    static u32 id = NextId();
    return id;
}

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


class UndoStack
{
public:
    UndoStack(s32 stackLimit = -1);

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
    void DebugRenderCommandList() const;
private:
    std::vector<std::unique_ptr<ICommand>> mUndoStack;
    u32 mCommandIndex = 0;
    s32 mStackLimit = -1;
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

class EditorMode
{
public:
    NO_MOVE_OR_MOVE_ASSIGN(EditorMode);

    EditorMode(WorldState& mapState);
   

    /* TODO: Set correct cursors
    enum class eMouseCursor
    {
    eArrow,
    eOpenHand,
    eClosedHand,
    };
    eMouseCursor mMouseCursor = eMouseCursor::eArrow;
    */

    void Update(const InputState& input, CoordinateSpace& coords);
    void Render(AbstractRenderer& rend) const;
    void OnMapChanged();

    f32 mEditorCamZoom = 1.0f;

private:

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
    WorldState& mWorldState;
};
