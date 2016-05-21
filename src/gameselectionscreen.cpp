#include "gameselectionscreen.hpp"
#include "developerscreen.hpp"
#include "gui.h"

void GameSelectionScreen::Update()
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

    bool gotoDevMode = false;
    if (gui_button(mGui, "Start game"))
    {
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

        auto missingDataPaths = mResLocator.GetDataPaths().MissingDataSets(setNames);
        if (!missingDataPaths.empty())
        {
            // Some are missing so ask the user for them
            LOG_ERROR(missingDataPaths.size() << " data paths are missing");
        }

        if (/*gd.DataSetName() == "Developer" &&*/ missingDataPaths.empty() && missingDataSets.empty())
        {
            LOG_INFO("Loading " << userSelectedGameDef.DataSetName());
            
            //mResLocator.GetDataPaths().SetActiveDataPaths(requiredDataSets);

            gotoDevMode = true;
        }
    }

    gui_end_window(mGui);
    if (gotoDevMode)
    {
        // "this" will be deleted after this call, so must be last call in the function

        mStateChanger.ToState(std::make_unique<DevloperScreen>(mGui, mStateChanger, mFmv, mSound, mLevel, mFsOld));
    }
}
