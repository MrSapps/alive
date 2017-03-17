#include "editormode.hpp"
#include "engine.hpp"
#include "debug.hpp"
#include "gui.h"

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

void UndoStack::DebugRenderCommandList(GuiContext& gui) const
{
    if (Debugging().mShowDebugUi)
    {
        gui_begin_window(&gui, "Undo stack");
        for (const auto& cmd : mUndoStack)
        {
            gui_label(&gui, cmd->Message().c_str());
        }
        gui_end_window(&gui);
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

EditorMode::EditorMode(GridMapState& mapState)
    : mMapState(mapState)
{

}

void EditorMode::Update(const InputState& input, CoordinateSpace& coords)
{
    const glm::vec2 mousePosWorld = coords.ScreenToWorld({ input.mMousePosition.mX, input.mMousePosition.mY });

    if (input.mKeys[SDL_SCANCODE_E].IsPressed())
    {
        mMapState.mState = GridMapState::eStates::eToGame;
        coords.mSmoothCameraPosition = true;
        mMapState.mModeSwitchTimeout = SDL_GetTicks() + kSwitchTimeMs;

        const s32 mouseCamX = static_cast<s32>(mousePosWorld.x / mMapState.kCameraBlockSize.x);
        const s32 mouseCamY = static_cast<s32>(mousePosWorld.y / mMapState.kCameraBlockSize.y);

        mMapState.mCameraPosition.x = (mouseCamX * mMapState.kCameraBlockSize.x) + (mMapState.kVirtualScreenSize.x / 2);
        mMapState.mCameraPosition.y = (mouseCamY * mMapState.kCameraBlockSize.y) + (mMapState.kVirtualScreenSize.y / 2);

        if (mMapState.mCameraSubject)
        {
            mMapState.mCameraSubject->mXPos = mMapState.mCameraPosition.x;
            mMapState.mCameraSubject->mYPos = mMapState.mCameraPosition.y;
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

        if (input.mKeys[SDL_SCANCODE_W].IsDown()) { mMapState.mCameraPosition.y -= editorCamSpeed; }
        else if (input.mKeys[SDL_SCANCODE_S].IsDown()) { mMapState.mCameraPosition.y += editorCamSpeed; }

        if (input.mKeys[SDL_SCANCODE_A].IsDown()) { mMapState.mCameraPosition.x -= editorCamSpeed; }
        else if (input.mKeys[SDL_SCANCODE_D].IsDown()) { mMapState.mCameraPosition.x += editorCamSpeed; }
    }

    coords.SetScreenSize(glm::vec2(coords.Width(), coords.Height()) * mEditorCamZoom);
    coords.SetCameraPosition(mMapState.mCameraPosition);

    // Find out what line is under the mouse pos, if any
    const s32 lineIdx = CollisionLine::Pick(mMapState.mCollisionItems, mousePosWorld, mMapState.mState == GridMapState::eStates::eInGame ? 1.0f : mEditorCamZoom);

    if (lineIdx >= 0)
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND)); // SDL_SYSTEM_CURSOR_CROSSHAIR
    }
    else
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
    }

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
        if (lineIdx >= 0)
        {
            const bool bGroupSelection = input.mKeys[SDL_SCANCODE_LCTRL].IsDown();
            bool updateState = true;
            if (bGroupSelection)
            {
                LOG_INFO("Toggle line selected status");
                // Only change state if we select a new line, when we de-select there is no selection area to update
                updateState = !mMapState.mCollisionItems[lineIdx]->IsSelected();
                mUndoStack.Push<CommandSelectOrDeselectLine>(mMapState.mCollisionItems, mSelection, lineIdx, !mMapState.mCollisionItems[lineIdx]->IsSelected());
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
                        mUndoStack.Push<CommandClearSelection>(mMapState.mCollisionItems, mSelection);
                    }
                    mUndoStack.Push<CommandSelectOrDeselectLine>(mMapState.mCollisionItems, mSelection, lineIdx, true);
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
                    const f32 lineLength = mMapState.mCollisionItems[lineIdx]->mLine.Length();
                    const f32 distToP1 = glm::distance(mMapState.mCollisionItems[lineIdx]->mLine.mP1, mousePosWorld) / lineLength;
                    const f32 distToP2 = glm::distance(mMapState.mCollisionItems[lineIdx]->mLine.mP2, mousePosWorld) / lineLength;
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
                mUndoStack.Push<CommandClearSelection>(mMapState.mCollisionItems, mSelection);
            }
        }
    }
    else if (input.mMouseButtons[0].IsDown() && mSelectionState != eSelectionState::eNone && mLastMousePos != mousePosWorld)
    {
        // TODO: Update mouse cursor to reflect action
        switch (mSelectionState)
        {
        case eSelectionState::eLineP1Selected:
        case eSelectionState::eLineP2Selected:
            // TODO: Handle dis/connect to other lines when moving end points
            mUndoStack.PushMerge<CommandMoveLinePoint>(mMergeCommand, mMapState.mCollisionItems, mSelection, mousePosWorld, mSelectionState == eSelectionState::eLineP1Selected);
            break;

        case eSelectionState::eMoveSelected:
        case eSelectionState::eLineMiddleSelected:
            // TODO: Disconnect from other lines if moved away
            mUndoStack.PushMerge<MoveSelection>(mMergeCommand, mMapState.mCollisionItems, mSelection, mousePosWorld - mLastMousePos);
            break;

        case eSelectionState::eNone:
            break;
        }

        mLastMousePos = mousePosWorld;
        mMergeCommand = true;
    }
}


void EditorMode::Render(Renderer& rend, GuiContext& gui) const
{
    // rend.beginLayer(gui_layer(&gui) + 1);

    mUndoStack.DebugRenderCommandList(gui);

    // Draw every cam
    for (auto x = 0u; x < mMapState.mScreens.size(); x++)
    {
        for (auto y = 0u; y < mMapState.mScreens[x].size(); y++)
        {
            GridScreen *screen = mMapState.mScreens[x][y].get();
            if (!screen->hasTexture())
                continue;

            screen->Render((x * mMapState.kCameraBlockSize.x) + mMapState.kCameraBlockImageOffset.x,
                (y * mMapState.kCameraBlockSize.y) + mMapState.kCameraBlockImageOffset.y,
                mMapState.kVirtualScreenSize.x, mMapState.kVirtualScreenSize.y);
        }
    }

    mMapState.RenderDebug(rend);
}
