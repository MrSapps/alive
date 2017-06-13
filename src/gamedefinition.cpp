#include "gamedefinition.hpp"
#include <jsonxx/jsonxx.h>

DataSetIdentifiers::DataSetIdentifiers(IFileSystem& fs, const char* dataSetsIdsFileName)
{
    auto stream = fs.Open(dataSetsIdsFileName);
    if (stream)
    {
        Parse(stream->LoadAllToString());
    }
}

bool DataSetIdentifiers::IsMetaPath(const std::string& dataSetName) const
{
    return mMetaPaths.find(dataSetName) != std::end(mMetaPaths);
}

std::string DataSetIdentifiers::Identify(IFileSystem& fs) const
{
    for (const auto& dataPathId : mDataPathIds)
    {
        if (DoMatchFileSystemPathWithDataSetIdentifier(fs, dataPathId))
        {
            return dataPathId.first;
        }
    }
    return "";
}

bool DataSetIdentifiers::DoMatchFileSystemPathWithDataSetIdentifier(IFileSystem& fs, const std::pair<std::string, DataPathFiles>& dataPathId) const
{
    // Check all of the "must exist files" do exist
    for (const auto& f : dataPathId.second.mContainAllOf)
    {
        std::string fCopy = f;
        if (!fs.FileExists(fCopy))
        {
            // Skip this dataPathId, the path doesn't match all of the "must contain"
            return false;
        }
    }

    // Check that all of the "must not exist" files don't exist
    for (const auto& f : dataPathId.second.mMustNotContain)
    {
        std::string fCopy = f;
        if (fs.FileExists(fCopy))
        {
            // Found a file that shouldn't exist, skip to the next dataPathId
            return false;
        }
    }

    // Check we can find at least one of the "any of" set of files
    bool foundAnyOf = false;
    for (const auto& f : dataPathId.second.mContainAnyOf)
    {
        std::string fCopy = f;
        if (fs.FileExists(fCopy))
        {
            foundAnyOf = true;
            break;
        }
    }

    // If one of the "any of" files exists or we have one or more "must exist" files
    // then this dataPathId matches path
    if (foundAnyOf || dataPathId.second.mContainAllOf.empty() == false)
    {
        return true;
    }

    return false;
}

void DataSetIdentifiers::Parse(const std::string& json)
{
    rapidjson::Document document;
    document.Parse(json.c_str());

    if (document.HasMember("data_set_ids"))
    {
        const auto& dataSetIds = document["data_set_ids"].GetObject();
        for (auto it = dataSetIds.MemberBegin(); it != dataSetIds.MemberEnd(); ++it)
        {
            const auto& dataSetId = it->value.GetObject();

            DataPathFiles dpFiles;
            JsonDeserializer::ReadStringArray("contains_any", dataSetId, dpFiles.mContainAnyOf);
            JsonDeserializer::ReadStringArray("contains_all", dataSetId, dpFiles.mContainAllOf);
            JsonDeserializer::ReadStringArray("not_contains", dataSetId, dpFiles.mMustNotContain);

            if (dpFiles.mContainAnyOf.empty() && dpFiles.mContainAllOf.empty() && dpFiles.mMustNotContain.empty())
            {
                mMetaPaths.insert(it->name.GetString());
            }
            else
            {
                mDataPathIds[it->name.GetString()] = dpFiles;
            }
        }
    }
}

bool DataPaths::SetActiveDataPaths(IFileSystem& fs, const PathVector& paths)
{
    mActiveDataPaths.clear();

    // Add paths in order, including mod zips
    for (const Path& pds : paths)
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