#include "gameselectionscreen.hpp"
#include "developerscreen.hpp"
#include "nfd.h"
#include "animation.hpp"

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

void GameSelectionScreen::Update(const InputState& /*input*/, CoordinateSpace& /*coords*/)
{
    switch (mState)
    {
    case eState::eFindDataSets:
        RenderFindDataSets();
        break;

    case eState::eSelectGame:
        RenderSelectGame();
        break;
    }
}

void GameSelectionScreen::EnterState()
{

}

void GameSelectionScreen::ExitState()
{

}

void GameSelectionScreen::LoadGameDefinition()
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

    mStateMachine.ToState(std::make_unique<DeveloperScreen>(mStateMachine, mSound, mLevel, mResLocator));
}

void GameSelectionScreen::RenderFindDataSets()
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
            mState = eState::eSelectGame;
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

void GameSelectionScreen::RenderSelectGame()
{
    if (ImGui::Begin("Select game"))
    {
        for (size_t idx = 0; idx < mVisibleGameDefinitions.size(); idx++)
        {
            const GameDefinition& gd = *mVisibleGameDefinitions[idx];
            if (ImGui::RadioButton(gd.Name().c_str(), mSelectedGameDefintionIndex == idx))
            {
                mSelectedGameDefintionIndex = idx;
            }
        }

        // Select developer mode by default
        static bool setFirstIndex = false;
        if (!setFirstIndex)
        {
            if (!mVisibleGameDefinitions.empty())
            {
                for (size_t idx = 0; idx < mVisibleGameDefinitions.size(); idx++)
                {
                    const GameDefinition& gd = *mVisibleGameDefinitions[idx];
                    if (gd.Name() == "Developer mode")
                    {
                        mSelectedGameDefintionIndex = idx;
                        break;
                    }
                }
            }
            setFirstIndex = true;
        }

        if (ImGui::Button("Start game"))
        {
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
                    mState = eState::eFindDataSets;
                }
                else
                {
                    LoadGameDefinition();
                }
            }
            else
            {
                // Need user to download missing game defs, no in game way to recover from this
                LOG_ERROR(missingDataSets.size() << " data sets are missing");
            }
        }
    }
    ImGui::End();
}

void GameSelectionScreen::Render(int /*w*/, int /*h*/, AbstractRenderer& /*renderer*/)
{

}
