#include "gameselectionscreen.hpp"
#include "developerscreen.hpp"
#include "gui.h"
#include "nfd.h"

void GameSelectionScreen::Update()
{

}

void GameSelectionScreen::Input(InputState& /*input*/)
{

}

void GameSelectionScreen::Render(int /*w*/, int /*h*/, Renderer& /*renderer*/)
{
    gui_begin_window(mGui, "Select game");

    for (size_t idx = 0; idx < mVisibleGameDefinitions.size(); idx++)
    {
        const GameDefinition& gd = *mVisibleGameDefinitions[idx];
        if (gui_radiobutton(mGui, gd.Name().c_str(), mSelectedGameDefintionIndex == idx))
        {
            mSelectedGameDefintionIndex = idx;
        }

    }

    if (gui_button(mGui, "Start game"))
    {

        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

        if (result == NFD_OKAY) {
            puts("Success!");
            puts(outPath);
            free(outPath);
        }
        else if (result == NFD_CANCEL) {
            puts("User pressed cancel.");
        }
        else {
            printf("Error: %s\n", NFD_GetError());
        }


        const GameDefinition& userSelectedGameDef = *mVisibleGameDefinitions[mSelectedGameDefintionIndex];

        DataSetMap requiredDataSets;
        std::set<std::string> missingDataSets;

        std::vector<const GameDefinition*> allGameDefs;
        for (const auto& t : mGameDefinitions)
        {
            allGameDefs.push_back(&t);
        }

        // Check we have the required data sets
        GameDefinition::GetDependencies(requiredDataSets, missingDataSets, &userSelectedGameDef, allGameDefs);
        if (!missingDataSets.empty())
        {
            // Need user to download missing game defs, no in game way to recover from this
            LOG_ERROR(missingDataSets.size() << " data sets are missing");
        }

        const BuiltInAndModGameDefs sorted = GameDefinition::SplitInToBuiltInAndMods(requiredDataSets);

        // Check we have a valid path to the "builtin" (i.e original) game files
        std::vector<std::string> setNames;
        for (const auto& dSetName : sorted.gameDefs)
        {
            setNames.push_back(dSetName->mDataSetName);
        }

        auto missingDataPaths = mResLocator.GetDataPaths().MissingDataSetPaths(setNames);
        if (!missingDataPaths.empty())
        {
            // Some are missing so ask the user for them
            for (const auto& dp : missingDataPaths)
            {
                LOG_ERROR(dp << " data path is missing");
            }
           
        }

        // Get the paths for the datasets
        for (PriorityDataSet& pds : requiredDataSets)
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

        if (/*gd.DataSetName() == "Developer" &&*/ missingDataPaths.empty() && missingDataSets.empty())
        {
            LOG_INFO("Loading " << userSelectedGameDef.DataSetName());
            
            // Set active data sets for the resource loader to use
            mResLocator.GetDataPaths().SetActiveDataPaths(mFs, requiredDataSets);
            
            // Temp/debug
            auto res = mResLocator.LocateAnimation("ABEBSIC.BAN_10_31");
            //res = mResourceLocator->Locate<Animation>("ABEBSIC.BAN_10_31", "AePc");

            mStateChanger.ToState(std::make_unique<DeveloperScreen>(mGui, mStateChanger, mFmv, mSound, mLevel, mResLocator));
        }
    }

    gui_end_window(mGui);
}
