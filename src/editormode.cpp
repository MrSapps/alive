#include "editormode.hpp"
#include "engine.hpp"
#include "debug.hpp"
#include "world.hpp"
#include "alive_version.h"
#include "core/systems/camerasystem.hpp"

#include "CONTRIBUTORS.md.g.h"

struct Contributor
{
public:
    Contributor(u32 flags, std::string name)
        : mFlags(flags), mName(name)
    {

    }

    enum Flags
    {
        eTitle = 0x1,
    };
    u32 mFlags = 0;
    std::string mName;
};

static std::vector<Contributor> GenerateContributors()
{
    static std::vector<Contributor> contributors;
    if (contributors.empty())
    {
        // Load the CONTRIBUTORS.MD byte array
        std::vector<unsigned char> rawData = get_CONTRIBUTORS();

        // Convert it to a std::string
        std::string rawDataStr(reinterpret_cast<const char*>(rawData.data()), rawData.size());

        // Split at line breaks
        std::deque<std::string> parts = string_util::split(rawDataStr, '\n');

        contributors.reserve(parts.size());

        // Flag mark down H5 headers as eTitle
        for (auto& part : parts)
        {
            bool hadHash = false;
            if (!part.empty() && part[0] == '#')
            {
                // Convert ### Blah to Blah
                auto spacePos = part.find_first_of(' ');
                if (spacePos != std::string::npos)
                {
                    part = part.substr(spacePos + 1);
                    hadHash = true;
                }
            }

            // We split at \n and are left with some empty lines of \r, ignore those
            if (part.size() > 1)
            {
                contributors.emplace_back(Contributor(hadHash ? Contributor::eTitle : 0, part));
            }
        }
    }
    return contributors;
}

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

static u32 gTypeIds = 0;
u32 NextId()
{
    gTypeIds++;
    return gTypeIds;
}

std::set<s32> Selection::Clear(CollisionLines& items)
{
    for (s32 idx : mSelectedLines)
    {
        items[idx]->SetSelected(false);
    }
    auto ret = std::move(mSelectedLines);
    return ret;
}

void Selection::Select(CollisionLines& items, s32 idx, bool select)
{
    if (items[idx]->SetSelected(select))
    {
        mSelectedLines.insert(idx);
    }
    else
    {
        mSelectedLines.erase(idx);
    }
}

UndoStack::UndoStack(s32 stackLimit /*= -1*/)
    : mStackLimit(stackLimit)
{
    Debugging().AddSection([&]()
    {
        DebugRenderCommandList();
    });
}

void UndoStack::DebugRenderCommandList() const
{
    if (ImGui::CollapsingHeader("Undo stack"))
    {
        for (const auto& cmd : mUndoStack)
        {
            ImGui::TextUnformatted(cmd->Message().c_str());
        }

        if (mUndoStack.empty())
        {
            ImGui::TextUnformatted("(Empty)");
        }
    }
}

void UndoStack::Clear()
{
    mUndoStack.clear();
}
void UndoStack::Undo()
{
    if (Count() > 0 && mCommandIndex >= 1)
    {
        LOG_ERROR("Undoing command: " << mUndoStack[mCommandIndex - 1]->Message());

        // Undo the current command
        mUndoStack[mCommandIndex - 1]->Undo();

        // Go back one command
        mCommandIndex--;
    }
}

void UndoStack::Redo()
{
    if (mCommandIndex + 1 <= Count())
    {
        LOG_ERROR("Redoing command: " << mUndoStack[mCommandIndex]->Message());

        // Redo the current command
        mUndoStack[mCommandIndex]->Redo();

        // Go forward one command
        mCommandIndex++;
    }
}

EditorMode::EditorMode(WorldState& mapState)
    : mWorldState(mapState)
{

}

void EditorMode::Update(const InputState& input, CoordinateSpace& coords)
{
    bool menuItemHandled = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            menuItemHandled = true;

            ImGui::MenuItem("New map", nullptr);
            ImGui::MenuItem("Open map", nullptr);
            ImGui::MenuItem("Save", nullptr);
            ImGui::MenuItem("Save as", nullptr);

            if (ImGui::MenuItem("Exit", nullptr)) 
            {
                mWorldState.mState = WorldState::States::eQuit;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            menuItemHandled = true;

            if (ImGui::MenuItem("Undo", "CTRL+Z")) 
            {
                mUndoStack.Undo();
            }

            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
            {
                mUndoStack.Redo();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            menuItemHandled = true;

            if (ImGui::MenuItem("About", nullptr))
            {
                mShowAbout = !mShowAbout;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (mShowAbout)
    {
        if (ImGui::Begin(ALIVE_VERSION))
        {
            bool isFirstSeperator = true;
            const auto& contributors = GenerateContributors();
            for (const auto& contributor : contributors)
            {
                if (contributor.mFlags & Contributor::eTitle)
                {
                    if (!isFirstSeperator)
                    {
                        ImGui::Separator();
                    }
                    isFirstSeperator = false;
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "%s", contributor.mName.c_str());
                }
                else
                {
                    ImGui::TextUnformatted(contributor.mName.c_str());
                }
            }

        }
        ImGui::End();
    }

    if (menuItemHandled)
    {
        return;
    }

    const glm::vec2 mousePosWorld = coords.ScreenToWorld({ input.mMousePosition.mX, input.mMousePosition.mY });

    if (input.mKeys[SDL_SCANCODE_E].IsPressed())
    {
        mWorldState.mState = WorldState::States::eToGame;
        coords.mSmoothCameraPosition = true;
        mWorldState.mModeSwitchTimeout = SDL_GetTicks() + kSwitchTimeMs;

        const s32 mouseCamX = static_cast<s32>(mousePosWorld.x / mWorldState.kCameraBlockSize.x);
        const s32 mouseCamY = static_cast<s32>(mousePosWorld.y / mWorldState.kCameraBlockSize.y);

        mWorldState.mCameraPosition.x = (mouseCamX * mWorldState.kCameraBlockSize.x) + (mWorldState.kVirtualScreenSize.x / 2);
        mWorldState.mCameraPosition.y = (mouseCamY * mWorldState.kCameraBlockSize.y) + (mWorldState.kVirtualScreenSize.y / 2);

        if (mWorldState.mEntityManager.GetSystem<CameraSystem>()->mTarget)
        {
            // mWorldState.mCameraSubject->mXPos = mWorldState.mCameraPosition.x;
            // mWorldState.mCameraSubject->mYPos = mWorldState.mCameraPosition.y;
        }
    }

    bool bGoFaster = false;
    if (input.mKeys[SDL_SCANCODE_LCTRL].IsDown())
    {
        if (input.mKeys[SDL_SCANCODE_W].IsPressed()) { mEditorCamZoom -= 0.1f; }
        else if (input.mKeys[SDL_SCANCODE_S].IsPressed()) { mEditorCamZoom += 0.1f; }

        mEditorCamZoom = glm::clamp(mEditorCamZoom, 0.1f, 3.0f);
    }
    else
    {
        if (input.mKeys[SDL_SCANCODE_LSHIFT].IsDown()) { bGoFaster = true; }

        f32 editorCamSpeed = 10.0f * mEditorCamZoom;
        if (bGoFaster)
        {
            editorCamSpeed *= 4.0f;
        }

        if (input.mKeys[SDL_SCANCODE_W].IsDown()) { mWorldState.mCameraPosition.y -= editorCamSpeed; }
        else if (input.mKeys[SDL_SCANCODE_S].IsDown()) { mWorldState.mCameraPosition.y += editorCamSpeed; }

        if (input.mKeys[SDL_SCANCODE_A].IsDown()) { mWorldState.mCameraPosition.x -= editorCamSpeed; }
        else if (input.mKeys[SDL_SCANCODE_D].IsDown()) { mWorldState.mCameraPosition.x += editorCamSpeed; }
    }

    coords.SetScreenSize(glm::vec2(coords.Width(), coords.Height()) * mEditorCamZoom);
    coords.SetCameraPosition(mWorldState.mCameraPosition);

    // Find out what line is under the mouse pos, if any
	// TODO: Wire CollisionSystem here
	/*
    const s32 lineIdx = CollisionLine::Pick(mWorldState.mCollisionItems, mousePosWorld, mWorldState.mState == WorldState::States::eInGame ? 1.0f : mEditorCamZoom);

    if (lineIdx >= 0)
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND)); // SDL_SYSTEM_CURSOR_CROSSHAIR
    }
    else
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
    }
	*/

    if (input.mMouseButtons[0].IsReleased())
    {
        mSelectionState = eSelectionState::eNone;
    }

    if (input.mKeys[SDL_SCANCODE_LCTRL].IsDown())
    {
        if (input.mKeys[SDL_SCANCODE_Z].IsPressed())
        {
            mUndoStack.Undo();
        }
        else if (input.mKeys[SDL_SCANCODE_Y].IsPressed())
        {
            mUndoStack.Redo();
        }
    }

    if (input.mMouseButtons[0].IsPressed())
    {
        mMergeCommand = false; // New mouse press/selection, start a new undo command
		// TODO: Wire CollisionSystem here
		/*
        if (lineIdx >= 0)
        {
            const bool bGroupSelection = input.mKeys[SDL_SCANCODE_LCTRL].IsDown();
            bool updateState = true;
            if (bGroupSelection)
            {
                LOG_INFO("Toggle line selected status");
                // Only change state if we select a new line, when we de-select there is no selection area to update
                updateState = !mWorldState.mCollisionItems[lineIdx]->IsSelected();
                mUndoStack.Push<CommandSelectOrDeselectLine>(mWorldState.mCollisionItems, mSelection, lineIdx, !mWorldState.mCollisionItems[lineIdx]->IsSelected());
                if (!updateState)
                {
                    mSelectionState = eSelectionState::eNone;
                }
            }
            else
            {
                // If clicking on a line that is already selected then don't clear out any other lines that are already selected
                if (mSelection.SelectedLines().find(lineIdx) == std::end(mSelection.SelectedLines()))
                {
                    LOG_INFO("Select single line");
                    if (mSelection.HasSelection())
                    {
                        mUndoStack.Push<CommandClearSelection>(mWorldState.mCollisionItems, mSelection);
                    }
                    mUndoStack.Push<CommandSelectOrDeselectLine>(mWorldState.mCollisionItems, mSelection, lineIdx, true);
                }
                else
                {
                    LOG_INFO("Line already selected, do nothing");
                }
            }

            if (updateState)
            {
                if (mSelection.SelectedLines().size() > 1)
                {
                    mSelectionState = eSelectionState::eMoveSelected;
                }
                else if (mSelection.SelectedLines().size() == 1)
                {
                    const f32 lineLength = mWorldState.mCollisionItems[lineIdx]->mLine.Length();
                    const f32 distToP1 = glm::distance(mWorldState.mCollisionItems[lineIdx]->mLine.mP1, mousePosWorld) / lineLength;
                    const f32 distToP2 = glm::distance(mWorldState.mCollisionItems[lineIdx]->mLine.mP2, mousePosWorld) / lineLength;
                    if (distToP1 < 0.2f)
                    {
                        mSelectionState = eSelectionState::eLineP1Selected;
                    }
                    else if (distToP2 < 0.2f)
                    {
                        mSelectionState = eSelectionState::eLineP2Selected;
                    }
                    else
                    {
                        mSelectionState = eSelectionState::eLineMiddleSelected;
                    }
                }
                else
                {
                    mSelectionState = eSelectionState::eNone;
                }
            }
            mLastMousePos = mousePosWorld;
        }
        else
        {
            if (mSelection.HasSelection())
            {
                LOG_INFO("Nothing clicked, clear selected");
                mUndoStack.Push<CommandClearSelection>(mWorldState.mCollisionItems, mSelection);
            }
        }
		*/
    }
    else if (input.mMouseButtons[0].IsDown() && mSelectionState != eSelectionState::eNone && mLastMousePos != mousePosWorld)
    {
        // TODO: Update mouse cursor to reflect action
        switch (mSelectionState)
        {
        case eSelectionState::eLineP1Selected:
        case eSelectionState::eLineP2Selected:
            // TODO: Handle dis/connect to other lines when moving end points
			// TODO: Wire CollisionSystem here
            // mUndoStack.PushMerge<CommandMoveLinePoint>(mMergeCommand, mWorldState.mCollisionItems, mSelection, mousePosWorld, mSelectionState == eSelectionState::eLineP1Selected);
            break;

        case eSelectionState::eMoveSelected:
        case eSelectionState::eLineMiddleSelected:
            // TODO: Disconnect from other lines if moved away
			// TODO: Wire CollisionSystem here
            // mUndoStack.PushMerge<MoveSelection>(mMergeCommand, mWorldState.mCollisionItems, mSelection, mousePosWorld - mLastMousePos);
            break;

        case eSelectionState::eNone:
            break;
        }

        mLastMousePos = mousePosWorld;
        mMergeCommand = true;
    }
}


void EditorMode::Render(AbstractRenderer& rend) const
{
    if (Debugging().mDrawCameras)
    {
        // Draw every cam
        for (auto x = 0u; x < mWorldState.mScreens.size(); x++)
        {
            for (auto y = 0u; y < mWorldState.mScreens[x].size(); y++)
            {
                // screen can be null while the array is being populated during loading
                GridScreen* screen = mWorldState.mScreens[x][y].get();
                if (screen)
                {
                    if (!screen->hasTexture())
                        continue;

                    screen->Render(rend, (x * mWorldState.kCameraBlockSize.x) + mWorldState.kCameraBlockImageOffset.x,
                        (y * mWorldState.kCameraBlockSize.y) + mWorldState.kCameraBlockImageOffset.y,
                        mWorldState.kVirtualScreenSize.x, mWorldState.kVirtualScreenSize.y);
                }
            }
        }
    }
    mWorldState.RenderDebug(rend);
}

void EditorMode::ClearUndoStack()
{
    mUndoStack.Clear();
}
