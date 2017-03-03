#include "gamedefinition.hpp"
#include <jsonxx/jsonxx.h>

bool DataPaths::SetActiveDataPaths(IFileSystem& fs, const DataSetPathVector& paths)
{
    mActiveDataPaths.clear();

    // Add paths in order, including mod zips
    for (const DataSetPath& pds : paths)
    {
        if (!pds.mDataSetPath.empty())
        {
            auto dataSetFs = IFileSystem::Factory(fs, pds.mDataSetPath);
            if (dataSetFs)
            {
                mActiveDataPaths.emplace_back(FileSystemInfo(pds.mDataSetName, pds.mSourceGameDefinition->IsMod(), std::move(dataSetFs)));
            }
            else
            {
                // Couldn't get an FS for the data path, fail
                return false;
            }
        }
    }
    return true;
}

void GameDefinition::Parse(const std::string& json)
{
    jsonxx::Object root;
    root.parse(json);

    mName = root.get<jsonxx::String>("Name");
    mDescription = root.get<jsonxx::String>("Description");
    mAuthor = root.get<jsonxx::String>("Author");
    if (root.has<jsonxx::String>("InitialLevel"))
    {
        mInitialLevel = root.get<jsonxx::String>("InitialLevel");
    }
    mDataSetName = root.get<jsonxx::String>("DatasetName");
    if (root.has<jsonxx::Boolean>("Hidden"))
    {
        mHidden = root.get<jsonxx::Boolean>("Hidden");
    }

    if (root.has<jsonxx::Array>("RequiredDatasets"))
    {
        const auto requiredSets = root.get<jsonxx::Array>("RequiredDatasets");
        mRequiredDataSets.reserve(requiredSets.size());
        for (size_t i = 0; i < requiredSets.size(); i++)
        {
            mRequiredDataSets.emplace_back(requiredSets.get<jsonxx::String>(static_cast<u32>(i)));
        }
    }
}