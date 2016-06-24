#include "resourcemapper.hpp"
#include "zipfilesystem.hpp"
#include "gui.h"

bool DataPaths::SetActiveDataPaths(IFileSystem& fs, const DataSetMap& paths)
{
    mActiveDataPaths.clear();

    // Add paths in order, including mod zips
    for (const PriorityDataSet& pds : paths)
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

/*static*/ std::unique_ptr<IFileSystem> IFileSystem::Factory(IFileSystem& fs, const std::string& path)
{
    TRACE_ENTRYEXIT;

    if (path.empty())
    {
        return nullptr;
    }

    const bool isFile = fs.FileExists(path.c_str());
    std::unique_ptr<IFileSystem> ret;
    if (isFile)
    {
        if (string_util::ends_with(path, ".bin", true))
        {
            LOG_INFO("Creating ISO FS for " << path);
            ret = std::make_unique<CdIsoFileSystem>(path.c_str());
        }
        else if (string_util::ends_with(path, ".zip", true) || string_util::ends_with(path, ".exe", true))
        {
            LOG_INFO("Creating ZIP FS for " << path);
            ret = std::make_unique<ZipFileSystem>(path.c_str(), fs);
        }
        else
        {
            LOG_ERROR("Unknown archive type for: " << path);
            return nullptr;
        }
    }
    else
    {
        LOG_INFO("Creating dir view FS for " << path);
        ret = std::make_unique<DirectoryLimitedFileSystem>(fs, path);
    }

    if (ret)
    {
        if (!ret->Init())
        {
            LOG_ERROR("FS init failed");
            return nullptr;
        }
    }
    return ret;
}

std::vector<std::tuple<const char*, const char*, bool>> ResourceMapper::DebugUi(class Renderer& /*renderer*/, GuiContext* gui)
{
    // Collect the UI data/state
    if (mUi.mItems.empty())
    {
        for (const auto& animMap : mAnimMaps)
        {
            // "Resource" name
            UiItem item;
            std::string dataSets = " (";
            for (const AnimFileLocations& mapping : animMap.second.mLocations)
            {
                // Each dataset this resource lives in
                item.mItems.push_back(mapping.mDataSetName);
                dataSets += mapping.mDataSetName + " ";
            }
            dataSets += ")";
            item.mLabel = animMap.first + dataSets;
            item.mResourceName = animMap.first;
            mUi.mItems.push_back(item);
        }
    }

    // Render it
    std::vector<std::tuple<const char*, const char*, bool>> ret;
    int i = 0;
    for (UiItem& item : mUi.mItems)
    {
        if (gui_checkbox(gui, gui_str(gui, "checkbox_%i|%s", i++, item.mLabel.c_str()), &item.mLoad))
        {
            for (const std::string& subItem : item.mItems)
            {
                ret.emplace_back(std::make_tuple(subItem.c_str(), item.mResourceName.c_str(), item.mLoad));
            }
           
        }
    }
    return ret;
}

std::vector<std::tuple<const char*, const char*, bool>> ResourceLocator::DebugUi(class Renderer& renderer, GuiContext* gui)
{
    return mResMapper.DebugUi(renderer, gui);
}

std::unique_ptr<Animation> ResourceLocator::Locate(const char* resourceName)
{
    // For each data set attempt to find resourceName by mapping
    // to a LVL/file/chunk. Or in the case of a mod dataset something else.
    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod)
        {
            // TODO: Look up the override in the zip fs

            // If this name is not a known resource then it is a new resource for this mod

            // TODO: Handle special case overrides that still need the real file (i.e cam deltas)
        }
        else
        {
            const ResourceMapper::AnimMapping* animMapping = mResMapper.FindAnimation(resourceName);
            if (!animMapping)
            {
                return nullptr;
            }

            auto ret = DoLocate(fs, resourceName, *animMapping);
            if (ret)
            {
                return ret;
            }
        }
    }
    return nullptr;
}

std::unique_ptr<Animation> ResourceLocator::Locate(const char* resourceName, const char* dataSetName)
{
    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mDataSetName == dataSetName)
        {
            const ResourceMapper::AnimMapping* animMapping = mResMapper.FindAnimation(resourceName);
            if (!animMapping)
            {
                return nullptr;
            }

            auto ret = DoLocate(fs, resourceName, *animMapping);
            if (ret)
            {
                return ret;
            }
        }
    }
    return nullptr;
}

std::unique_ptr<Animation> ResourceLocator::DoLocate(const DataPaths::FileSystemInfo& fs, const char* resourceName, const ResourceMapper::AnimMapping& animMapping)
{
    // Each each mapping in the resource record that has matched resourceName
    for (const ResourceMapper::AnimFileLocations& location : animMapping.mLocations)
    {
        // Check if the mapping applies to the data set that fs is
        if (location.mDataSetName == fs.mDataSetName)
        {
            // Loop through all the locations in the data set where resourceName lives
            for (const ResourceMapper::AnimFile& animFile : location.mFiles)
            {
                // Now find all of the LVLs where animFile lives
                const std::vector<std::pair<bool, std::string>>* fileLocations = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), animFile.mFile.c_str());
                if (fileLocations)
                {
                    // Loop through each LVL and see if animFile exists there
                    for (const std::pair<bool, std::string>& lvlNameIsPsxPair : *fileLocations)
                    {
                        // Open the LVL archive
                        auto lvlStream = fs.mFileSystem->Open(lvlNameIsPsxPair.second);
                        if (lvlStream)
                        {
                            // Open the file within the archive
                            Oddlib::LvlArchive lvl(std::move(lvlStream));
                            auto lvlFile = lvl.FileByName(animFile.mFile);
                            if (lvlFile)
                            {
                                // Get the chunk within the file that lives in the lvl
                                Oddlib::LvlArchive::FileChunk* chunk = lvlFile->ChunkById(animFile.mId);
                                if (chunk)
                                {
                                    LOG_INFO(resourceName
                                        << " located in data set " << fs.mDataSetName
                                        << " mapped to " << fs.mFileSystem->FsPath()
                                        << " in lvl archive " << lvlNameIsPsxPair.second
                                        << " in lvl file " << animFile.mFile
                                        << " with lvl file chunk id " << animFile.mId
                                        << " at anim index " << animFile.mAnimationIndex
                                        << " is psx " << lvlNameIsPsxPair.first);

                                    // Construct the animation from the chunk bytes
                                    return std::make_unique<Animation>(
                                        chunk->Stream(),
                                        lvlNameIsPsxPair.first,
                                        animMapping.mBlendingMode,
                                        animMapping.mFrameOffsets,
                                        animFile.mAnimationIndex,
                                        fs.mDataSetName);
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    return nullptr;
}