#include "gameselectionstate.hpp"
#include "animationbrowser.hpp"
#include "nfd.h"
#include "sound.hpp"
#include "resourcemapper.hpp"

namespace
{
    enum class SelectResult
    {
        eDone,
        eCancelled
    };

    static SelectResult AskUserForFile(std::string& dir)
    {
        nfdchar_t* outPath = NULL;
        const nfdresult_t result = NFD_OpenDialog("exe,bin", NULL, &outPath);
        if (result == NFD_OKAY)
        {
            dir = outPath;
            free(outPath);
            return SelectResult::eDone;
        }
        return SelectResult::eCancelled;
    }
}

void GameSelectionState::UpdateVisibleGameDefinitions()
{
    mVisibleGameDefinitions = GameDefinition::GetVisibleGameDefinitions(mGameDefinitions);
}

EngineStates GameSelectionState::Update(const InputState& /*input*/, CoordinateSpace& /*coords*/)
{
    switch (mState)
    {
    case GameSelectionStates::eFindDataSets:
        RenderFindDataSets();
        break;

    case GameSelectionStates::eSelectGame:
        return RenderSelectGame();
    }
    return EngineStates::eSelectGame;
}

void GameSelectionState::LoadGameDefinition()
{
    // Save out the paths for next time/purge bad paths
    mResLocator.GetDataPaths().Persist();

    // Get the paths for the datasets
    for (DataPaths::Path& pds : mRequiredDataSets)
    {
        if (!pds.mSourceGameDefinition->IsMod())
        {
            pds.mDataSetPath = mResLocator.GetDataPaths().PathFor(pds.mDataSetName);
        }
        else
        {
            // Set mod zip path from its gd
            pds.mDataSetPath = pds.mSourceGameDefinition->ContainingArchive();
        }
    }

    // Set active data sets for the resource loader to use
    mResLocator.GetDataPaths().SetActiveDataPaths(mFs, mRequiredDataSets);
}

void GameSelectionState::RenderFindDataSets()
{
    bool regenerate = false;
    if (ImGui::Begin("Select missing paths"))
    {      
        for (const std::string& dp : mMissingDataPaths)
        {
            if (ImGui::Button(dp.c_str()))
            {
                std::string dir;
                for (;;)
                {
                    if (AskUserForFile(dir) == SelectResult::eCancelled)
                    {
                        break;
                    }
                    else if (mResLocator.GetDataPaths().Add(dir, dp))
                    {
                        regenerate = true;
                        break;
                    }
                }
            }
        }

        if (ImGui::Button("Back"))
        {
            mMissingDataPaths.clear();
            mRequiredDataSetNames.clear();
            mRequiredDataSets.clear();
            mState = GameSelectionStates::eSelectGame;
        }
    }
    ImGui::End();

    if (regenerate)
    {
        mMissingDataPaths = mResLocator.GetDataPaths().MissingDataSetPaths(mRequiredDataSetNames);
        if (mMissingDataPaths.empty())
        {
            LoadGameDefinition();
        }
    }
}

EngineStates GameSelectionState::RenderSelectGame()
{
    EngineStates nextState = EngineStates::eSelectGame;
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 440), ImGuiSetCond_FirstUseEver);
    if (ImGui::Begin("Select game", nullptr, ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::BeginChild("game list", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()));
        for (size_t idx = 0; idx < mVisibleGameDefinitions.size(); idx++)
        {
            const GameDefinition& gd = *mVisibleGameDefinitions[idx];
            if (ImGui::RadioButton(gd.Name().c_str(), mSelectedGameDefintionIndex == idx))
            {
                mSelectedGameDefintionIndex = idx;
            }
        }
        ImGui::EndChild();

        // Select developer mode by default
        static bool setFirstIndex = false;
        if (!setFirstIndex)
        {
            if (!mVisibleGameDefinitions.empty())
            {
                for (size_t idx = 0; idx < mVisibleGameDefinitions.size(); idx++)
                {
                    const GameDefinition& gd = *mVisibleGameDefinitions[idx];
                    if (gd.Name() == "Oddworld Abe's Exoddus PC")
                    {
                        mSelectedGameDefintionIndex = idx;
                        break;
                    }
                }
            }
            setFirstIndex = true;
        }


        ImGui::BeginGroup();
            ImGui::BeginChild("buttons");
#ifdef _DEBUG
            // Hack to force auto start the selected game definition in a debug build
            static bool attemptAutoStart = true;
            if (ImGui::Button("Start game") || attemptAutoStart)
#else
            if (ImGui::Button("Start game"))
#endif
                {

#ifdef _DEBUG
                    attemptAutoStart = false;
#endif

                    const GameDefinition& userSelectedGameDef = *mVisibleGameDefinitions[mSelectedGameDefintionIndex];

                    std::set<std::string> missingDataSets;

                    std::vector<const GameDefinition*> allGameDefs;
                    for (const GameDefinition& t : mGameDefinitions)
                    {
                        allGameDefs.push_back(&t);
                    }

                    // Check we have the required data sets
                    GameDefinition::GetDependencies(mRequiredDataSets, missingDataSets, &userSelectedGameDef, allGameDefs);
                    if (missingDataSets.empty())
                    {
                        // Check we have a valid path to the "builtin" (i.e original) game files
                        mRequiredDataSetNames.clear();
                        for (const DataPaths::Path& dataSet : mRequiredDataSets)
                        {
                            if (!dataSet.mSourceGameDefinition->IsMod())
                            {
                                mRequiredDataSetNames.push_back(dataSet.mDataSetName);
                            }
                        }

                        mMissingDataPaths = mResLocator.GetDataPaths().MissingDataSetPaths(mRequiredDataSetNames);
                        if (!mMissingDataPaths.empty())
                        {
                            mState = GameSelectionStates::eFindDataSets;
                        }
                        else
                        {
                            LoadGameDefinition();
                            nextState = EngineStates::eRunSelectedGame;
                        }
                    }
                    else
                    {
                        // Need user to download missing game defs, no in game way to recover from this
                        LOG_ERROR(missingDataSets.size() << " data sets are missing");
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Quit"))
                {
                    nextState = EngineStates::eQuit;
                }
            ImGui::EndChild();
        ImGui::EndGroup();

    }
    ImGui::End();
    return nextState;
}

GameSelectionState::GameSelectionState(const std::vector<GameDefinition>& gameDefinitions, ResourceLocator& resLocator, IFileSystem& newFs)
  : mGameDefinitions(gameDefinitions),
    mResLocator(resLocator),
    mFs(newFs)
{
    UpdateVisibleGameDefinitions();
}

void GameSelectionState::Render(AbstractRenderer& renderer)
{
    renderer.Clear(0.0f, 0.0f, 0.0f);
}
