#include "resourcemapper.hpp"
#include "zipfilesystem.hpp"

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
        else if (string_util::ends_with(path, ".zip", true))
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

Resource<Animation> ResourceLocator::Locate(const char* resourceName)
{
    const size_t resNameHash = StringHash(resourceName);

    // Check if the resource is cached
    std::shared_ptr<Animation> cachedRes = mResourceCache.Find<Animation>(resNameHash);
    if (cachedRes)
    {
        return Resource<Animation>(mResourceCache, cachedRes, resNameHash);
    }

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
            const ResourceMapper::AnimMapping* animMapping = mResMapper.FindAnimation(resourceName, fs.mDataSetName.c_str());
            if (animMapping)
            {
                for (const ResourceMapper::AnimMappingData& animData : animMapping->mFiles)
                {
                    const std::vector<std::pair<bool, std::string>>* fileLocations = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), animData.mFile.c_str());
                    if (fileLocations)
                    {
                        for (const std::pair<bool, std::string>& lvlNameIsPsxPair : *fileLocations)
                        {
                            // Open the LVL archive
                            auto lvlStream = fs.mFileSystem->Open(lvlNameIsPsxPair.second);
                            if (lvlStream)
                            {
                                // Open the file within the archive
                                Oddlib::LvlArchive lvl(std::move(lvlStream));
                                auto lvlFile = lvl.FileByName(animData.mFile);
                                if (lvlFile)
                                {
                                    // Get the chunk within the file that lives in the lvl
                                    Oddlib::LvlArchive::FileChunk* chunk = lvlFile->ChunkById(animData.mId);

                                    LOG_INFO(resourceName
                                        << " located in data set " << fs.mDataSetName
                                        << " mapped to " << fs.mFileSystem->FsPath()
                                        << " in lvl archive " << lvlNameIsPsxPair.second
                                        << " in lvl file " << animData.mFile
                                        << " with lvl file chunk id " << animData.mId
                                        << " at anim index " << animData.mAnimationIndex
                                        << " is psx " << lvlNameIsPsxPair.first);

                                    // Construct the animation from the chunk bytes
                                    return Resource<Animation>(resNameHash, mResourceCache, chunk->Stream(),
                                        lvlNameIsPsxPair.first, animData.mAnimationIndex);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // TODO
    return Resource<Animation>(StringHash(""), mResourceCache, nullptr, false, 0);
}

Resource<Animation> ResourceLocator::Locate(const char* resourceName, const std::string& dataSetName)
{
    const std::string uniqueName = std::string(resourceName) + dataSetName;
    const size_t resNameHash = StringHash(uniqueName);

    // Check if the resource is cached
    std::shared_ptr<Animation> cachedRes = mResourceCache.Find<Animation>(resNameHash);
    if (cachedRes)
    {
        return Resource<Animation>(mResourceCache, cachedRes, resNameHash);
    }

    // TODO: Have bespoke method to find animations - pass in possible dataset names
    //const ResourceMapper::AnimMapping* animMapping = mResMapper.Find(resourceName);
    //if (animMapping)
        {
            // TODO: Handle mod zips
            // TODO: Find resource in specific data set 
            //const auto& lvlFileToFind = animMapping->mFile;
            /*
            auto stream = mDataPaths.Open(lvlFileToFind, dataSetName);
            if (stream)
            {
            return Resource<T>(resNameHash, mResourceCache, std::move(stream));
            }*/
        }

    // TODO
    return Resource<Animation>(StringHash(""), mResourceCache, nullptr, false, 0);
}
