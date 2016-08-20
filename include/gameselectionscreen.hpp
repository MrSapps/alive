#pragma once

#include "engine.hpp"

class GameSelectionScreen : public EngineState
{
public:
    GameSelectionScreen(
            IEngineStateChanger& stateChanger,
            const std::vector<GameDefinition>& gameDefinitions,
            GuiContext* gui,
            class Fmv& fmv,
            class Sound& sound, 
            class Level& level,
            ResourceLocator& resLocator,
            IFileSystem& newFs)
      : EngineState(stateChanger), 
        mGameDefinitions(gameDefinitions),
        mGui(gui),
        mFmv(fmv),
        mSound(sound),
        mLevel(level),
        mResLocator(resLocator),
        mFs(newFs)
    {
        mVisibleGameDefinitions = GameDefinition::GetVisibleGameDefinitions(mGameDefinitions);
    }
    virtual void Input(InputState& input) override;
    virtual void Update() override;
    virtual void Render(int w, int h, Renderer& renderer) override;
private:
    void FilterGameDefinitions();

    const std::vector<GameDefinition>& mGameDefinitions;
    std::vector<const GameDefinition*> mVisibleGameDefinitions;
    struct GuiContext *mGui = nullptr;
    Fmv& mFmv;
    Sound& mSound;
    Level& mLevel;
    ResourceLocator& mResLocator;
    size_t mSelectedGameDefintionIndex = 0;
    IFileSystem& mFs;
};
