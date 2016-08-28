#pragma once

#include "engine.hpp"

class GameSelectionScreen : public IState
{
public:
    GameSelectionScreen(StateMachine& stateMachine,
            const std::vector<GameDefinition>& gameDefinitions,
            GuiContext* gui,
            class Fmv& fmv,
            class Sound& sound, 
            class Level& level,
            ResourceLocator& resLocator,
            IFileSystem& newFs)
      : IState(stateMachine),
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
    virtual void Render(int w, int h, Renderer& renderer) override;
    virtual void Update() override;
    virtual void EnterState() override;
    virtual void ExitState() override;
private:
    void FilterGameDefinitions();

    void RenderSelectGame();
    void RenderFindDataSets();
    void LoadGameDefinition();

    const std::vector<GameDefinition>& mGameDefinitions;
    std::vector<const GameDefinition*> mVisibleGameDefinitions;
    struct GuiContext *mGui = nullptr;
    Fmv& mFmv;
    Sound& mSound;
    Level& mLevel;
    ResourceLocator& mResLocator;
    size_t mSelectedGameDefintionIndex = 0;
    IFileSystem& mFs;
    enum class eState
    {
        eSelectGame,
        eFindDataSets
    };
    eState mState = eState::eSelectGame;
    std::vector<std::string> mMissingDataPaths;
    std::vector<std::string> mRequiredDataSetNames;
    DataSetMap mRequiredDataSets;
};
