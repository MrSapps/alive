#pragma once

#include "engine.hpp"

class GameSelectionState
{
public:
    GameSelectionState(GameSelectionState&&) = delete;
    GameSelectionState& operator = (GameSelectionState&&) = delete;
    GameSelectionState(const std::vector<GameDefinition>& gameDefinitions,
            ResourceLocator& resLocator,
            IFileSystem& newFs);
    void Render(AbstractRenderer& renderer);
    EngineStates Update(const InputState& input, CoordinateSpace& coords);
    const GameDefinition& SelectedGame() const { return *mVisibleGameDefinitions[mSelectedGameDefintionIndex]; }
private:
    void UpdateVisibleGameDefinitions();
    EngineStates RenderSelectGame();
    void RenderFindDataSets();
    void LoadGameDefinition();

    const std::vector<GameDefinition>& mGameDefinitions;
    std::vector<const GameDefinition*> mVisibleGameDefinitions;
    ResourceLocator& mResLocator;
    size_t mSelectedGameDefintionIndex = 0;
    IFileSystem& mFs;
    enum class GameSelectionStates
    {
        eSelectGame,
        eFindDataSets
    };
    GameSelectionStates mState = GameSelectionStates::eSelectGame;
    std::vector<std::string> mMissingDataPaths;
    std::vector<std::string> mRequiredDataSetNames;
    DataPaths::PathVector mRequiredDataSets;
};
