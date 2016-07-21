#include "resourcemapper.hpp"
#include "zipfilesystem.hpp"
#include "gui.h"
#include "fmv.hpp"
#include "oddlib/bits_factory.hpp"
#include "lodepng/lodepng.h"

const /*static*/ float Animation::kPcToPsxScaleFactor = 1.73913043478f;

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

std::vector<std::tuple<const char*, const char*, bool>> ResourceMapper::DebugUi(class Renderer& /*renderer*/, GuiContext* gui, const char* filter)
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
        bool found = false;
        if (filter[0] != '\0')
        {
            for (const std::string& subItem : item.mItems)
            {
                if (subItem == filter)
                {
                    found = true;
                    break;
                }
            }
        }
        else
        {
            found = true;
        }

        if (found)
        {
            if (gui_checkbox(gui, gui_str(gui, "checkbox_%i|%s", i++, item.mLabel.c_str()), &item.mLoad))
            {
                for (const std::string& subItem : item.mItems)
                {
                    ret.emplace_back(std::make_tuple(subItem.c_str(), item.mResourceName.c_str(), item.mLoad));
                }
            }
        }
    }
    return ret;
}

ResourceLocator::ResourceLocator(ResourceMapper&& resourceMapper, DataPaths&& dataPaths)
    : mResMapper(std::move(resourceMapper)), mDataPaths(std::move(dataPaths))
{

}

ResourceLocator::~ResourceLocator()
{

}

std::vector<std::tuple<const char*, const char*, bool>> ResourceLocator::DebugUi(class Renderer& renderer, GuiContext* gui, const char* filter)
{
    return mResMapper.DebugUi(renderer, gui, filter);
}

std::unique_ptr<Oddlib::IBits> ResourceLocator::LocateCamera(const char* resourceName)
{
    std::string deltaName;
    std::string modName;

    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod)
        {
            // TODO: Look up the override in the mod fs
            if (modName.empty())
            {
                modName = std::string(resourceName) + ".png";
            }

            if (fs.mFileSystem->FileExists(modName))
            {
                auto stream = fs.mFileSystem->Open(modName);
            }

            if (deltaName.empty())
            {
                deltaName = std::string(resourceName) + ".cam.bmp.png"; // TODO: Rename to something sane

                // TODO: Also requires the original file to work
            }

            if (fs.mFileSystem->FileExists(deltaName))
            {
                auto stream = fs.mFileSystem->Open(deltaName);

                lodepng::State state = {};

                // input color type
                state.info_raw.colortype = LCT_RGB;
                state.info_raw.bitdepth = 8;

                // output color type
                state.info_png.color.colortype = LCT_RGB;
                state.info_png.color.bitdepth = 8;
                state.encoder.auto_convert = 0;

                // decode PNG
                std::vector<unsigned char> out;
                std::vector<unsigned char> in = Oddlib::IStream::ReadAll(*stream);

                unsigned int w = 0;
                unsigned int h = 0;
                const auto decodeRet = lodepng::decode(out, w, h, state, in);
                if (decodeRet == 0)
                {

                }
                else
                {
                    LOG_ERROR(lodepng_error_text(decodeRet));
                }
            }

        }
        else
        {
            const std::vector<ResourceMapper::DataSetFileAttributes>* locationsInThisDataSet = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), resourceName);
            if (locationsInThisDataSet)
            {
                for (const ResourceMapper::DataSetFileAttributes& attributes : *locationsInThisDataSet)
                {
                    std::shared_ptr<Oddlib::LvlArchive> lvl = OpenLvl(*fs.mFileSystem, fs.mDataSetName, attributes.mLvlName);
                    if (lvl)
                    {
                        auto lvlFile = lvl->FileByName(resourceName);
                        if (lvlFile)
                        {
                            auto chunk = lvlFile->ChunkByType(Oddlib::MakeType('B', 'i', 't', 's'));
                            auto stream = chunk->Stream();
                            return Oddlib::MakeBits(*stream, lvl);
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

std::unique_ptr<IMovie> ResourceLocator::LocateFmv(IAudioController& audioController, const char* resourceName)
{
    const ResourceMapper::FmvMapping* fmvMapping = mResMapper.FindFmv(resourceName);
    if (!fmvMapping)
    {
        return nullptr;
    }

    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod)
        {
            // TODO: Look up the override in the mod fs

        }
        else
        {
            auto ret = DoLocateFmv(audioController, resourceName, fs, *fmvMapping);
            if (ret)
            {
                return ret;
            }
        }
    }

    return nullptr;
}


std::unique_ptr<IMovie> ResourceLocator::DoLocateFmv(IAudioController& audioController, const char* resourceName, const DataPaths::FileSystemInfo& fs, const ResourceMapper::FmvMapping& fmvMapping)
{
    // Each each mapping in the resource record that has matched resourceName
    for (const ResourceMapper::FmvFileLocation& location : fmvMapping.mLocations)
    {
        // Check if the mapping applies to the data set that fs is
        if (location.mDataSetName == fs.mDataSetName)
        {
            if (fs.mFileSystem->FileExists(location.mFileName))
            {
                auto stream = fs.mFileSystem->Open(location.mFileName);
                if (stream)
                {
                    std::unique_ptr<SubTitleParser> subTitles;
                    const std::string subTitleFileName = "data/subtitles/" + std::string(resourceName) + ".SRT";
                    if (mDataPaths.GameFs().FileExists(subTitleFileName))
                    {
                        auto subsStream = mDataPaths.GameFs().Open(subTitleFileName);
                        if (subsStream)
                        {
                            subTitles = std::make_unique<SubTitleParser>(std::move(subsStream));
                        }
                    }
                    return IMovie::Factory(audioController, std::move(stream), std::move(subTitles), location.mStartSector, location.mEndSector);
                }
            }
        }
    }
    return nullptr;
}


std::unique_ptr<Animation> ResourceLocator::LocateAnimation(const char* resourceName)
{
    const ResourceMapper::AnimMapping* animMapping = mResMapper.FindAnimation(resourceName);
    if (!animMapping)
    {
        return nullptr;
    }

    // For each data set attempt to find resourceName by mapping
    // to a LVL/file/chunk. Or in the case of a mod dataset something else.
    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod)
        {
            // TODO: Look up the override in the mod fs

            // If this name is not a known resource then it is a new resource for this mod

            // TODO: Handle special case overrides that still need the real file (i.e cam deltas)
        }
        else
        {
            auto ret = DoLocateAnimation(fs, resourceName, *animMapping);
            if (ret)
            {
                return ret;
            }
        }
    }
    return nullptr;
}

std::unique_ptr<Animation> ResourceLocator::LocateAnimation(const char* resourceName, const char* dataSetName)
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

            auto ret = DoLocateAnimation(fs, resourceName, *animMapping);
            if (ret)
            {
                return ret;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<Oddlib::LvlArchive> ResourceLocator::OpenLvl(IFileSystem& fs, const std::string& dataSetName, const std::string& lvlName)
{
    auto lvlPtr = mCache.GetLvl(dataSetName, lvlName);
    if (!lvlPtr)
    {
        // Try to open new lvl since it wasn't in the cache
        auto lvlStream = fs.Open(lvlName);
        if (lvlStream)
        {
            // Cache this lvl
            auto lvl = std::make_unique<Oddlib::LvlArchive>(std::move(lvlStream));
            lvlPtr = mCache.AddLvl(std::move(lvl), dataSetName, lvlName);
        }
    }
    return lvlPtr;
}

std::unique_ptr<Animation> ResourceLocator::DoLocateAnimation(const DataPaths::FileSystemInfo& fs, const char* resourceName, const ResourceMapper::AnimMapping& animMapping)
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
                const std::vector<ResourceMapper::DataSetFileAttributes>* fileLocations = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), animFile.mFile.c_str());
                if (fileLocations)
                {
                    // Loop through each LVL and see if animFile exists there
                    for (const ResourceMapper::DataSetFileAttributes& dataSetFileAttributes : *fileLocations)
                    {
                        auto lvlPtr = OpenLvl(*fs.mFileSystem, fs.mDataSetName, dataSetFileAttributes.mLvlName);
                        if (lvlPtr)
                        {
                            auto animSetPtr = mCache.GetAnimSet(fs.mDataSetName, dataSetFileAttributes.mLvlName, animFile.mFile, animFile.mId);
                            if (!animSetPtr)
                            {
                                // Open the file within the archive
                                auto lvlFile = lvlPtr->FileByName(animFile.mFile);
                                if (lvlFile)
                                {
                                    // Get the chunk within the file that lives in the lvl
                                    Oddlib::LvlArchive::FileChunk* chunk = lvlFile->ChunkById(animFile.mId);
                                    if (chunk)
                                    {
                                        LOG_INFO(resourceName
                                            << " located in data set " << fs.mDataSetName
                                            << " mapped to " << fs.mFileSystem->FsPath()
                                            << " in lvl archive " << dataSetFileAttributes.mLvlName
                                            << " in lvl file " << animFile.mFile
                                            << " with lvl file chunk id " << animFile.mId
                                            << " at anim index " << animFile.mAnimationIndex
                                            << " is psx " << dataSetFileAttributes.mIsPsx
                                            << " scale frame offsets " << dataSetFileAttributes.mScaleFrameOffsets);

                                        auto stream = chunk->Stream();
                                        Oddlib::AnimSerializer as(*stream, dataSetFileAttributes.mIsPsx);
                                        animSetPtr = mCache.AddAnimSet(std::make_unique<Oddlib::AnimationSet>(as), fs.mDataSetName, dataSetFileAttributes.mLvlName, animFile.mFile, animFile.mId);
                                    }
                                }
                            }

                            // Construct the animation from the chunk bytes
                            return std::make_unique<Animation>(
                                Animation::AnimationSetHolder(lvlPtr, animSetPtr, animFile.mAnimationIndex),
                                dataSetFileAttributes.mIsPsx,
                                dataSetFileAttributes.mScaleFrameOffsets,
                                animMapping.mBlendingMode,
                                fs.mDataSetName);

                        }
                    }
                }
            }
        }
    }
    return nullptr;
}