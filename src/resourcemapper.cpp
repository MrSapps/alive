#include "resourcemapper.hpp"
#include "zipfilesystem.hpp"
#include "gui.h"
#include "fmv.hpp"
#include "oddlib/bits_factory.hpp"
#include <cmath>

const /*static*/ f32 Animation::kPcToPsxScaleFactor = 1.73913043478f;

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


std::shared_ptr<Oddlib::LvlArchive> ResourceLocator::HackTemp2(const char* fileToFind)
{
    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (!fs.mIsMod)
        {
            const std::vector<ResourceMapper::DataSetFileAttributes>* locationsInThisDataSet = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), fileToFind);
            if (locationsInThisDataSet)
            {
                for (const ResourceMapper::DataSetFileAttributes& attributes : *locationsInThisDataSet)
                {
                    std::shared_ptr<Oddlib::LvlArchive> lvl = OpenLvl(*fs.mFileSystem, fs.mDataSetName, attributes.mLvlName);
                    if (lvl)
                    {
                        return lvl;
                    }
                }
            }

        }
    }
    return nullptr;
}

std::unique_ptr<Oddlib::IStream> ResourceLocator::HackTemp(const char* fileToFind)
{
    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (!fs.mIsMod)
        {
            const std::vector<ResourceMapper::DataSetFileAttributes>* locationsInThisDataSet = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), fileToFind);
            if (locationsInThisDataSet)
            {
                for (const ResourceMapper::DataSetFileAttributes& attributes : *locationsInThisDataSet)
                {
                    std::shared_ptr<Oddlib::LvlArchive> lvl = OpenLvl(*fs.mFileSystem, fs.mDataSetName, attributes.mLvlName);
                    if (lvl)
                    {
                        auto lvlFile = lvl->FileByName(fileToFind);
                        if (lvlFile)
                        {
                            auto chunk = lvlFile->ChunkByIndex(0);
                            return chunk->Stream();
                        }
                    }

                }
            }

        }
    }
    return nullptr;
}

std::unique_ptr<Oddlib::Path> ResourceLocator::LocatePath(const char* resourceName)
{
    const ResourceMapper::PathMapping* mapping = mResMapper.FindPath(resourceName);
    if (mapping)
    {
        for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
        {
            if (fs.mIsMod)
            {
                // TODO: Mod path
            }
            else
            {
                const ResourceMapper::PathLocation* pathLocation = mapping->Find(fs.mDataSetName);
                if (pathLocation)
                {
                    const std::vector<ResourceMapper::DataSetFileAttributes>* locationsInThisDataSet = mResMapper.FindFileLocation(fs.mDataSetName.c_str(), pathLocation->mDataSetFileName.c_str());
                    if (locationsInThisDataSet)
                    {
                        for (const ResourceMapper::DataSetFileAttributes& attributes : *locationsInThisDataSet)
                        {
                            std::shared_ptr<Oddlib::LvlArchive> lvl = OpenLvl(*fs.mFileSystem, fs.mDataSetName, attributes.mLvlName);
                            if (lvl)
                            {
                                auto lvlFile = lvl->FileByName(pathLocation->mDataSetFileName);
                                if (lvlFile)
                                {
                                    auto chunk = lvlFile->ChunkById(mapping->mId);
                                    auto stream = chunk->Stream();
                                    return std::make_unique<Oddlib::Path>(*stream,
                                        mapping->mCollisionOffset,
                                        mapping->mIndexTableOffset,
                                        mapping->mObjectOffset,
                                        mapping->mNumberOfScreensX,
                                        mapping->mNumberOfScreensY,
                                        attributes.mIsAo);
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

std::unique_ptr<Oddlib::IBits> ResourceLocator::LocateCamera(const char* resourceName)
{
    LOG_INFO("Requesting camera " << resourceName);
    return DoLocateCamera(resourceName, false);
}

static bool CanDeltaBeApplied(int camW, int camH, int deltaW, int deltaH)
{
    if (camW == 640 && deltaW == 1440 && camH == 240 && deltaH == 1080)
    {
        return true;
    }
    return false;
}

static SDL_SurfacePtr LoadPng(Oddlib::IStream& stream)
{
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
    std::vector<unsigned char> in = Oddlib::IStream::ReadAll(stream);

    unsigned int w = 0;
    unsigned int h = 0;

    const auto decodeRet = lodepng::decode(out, w, h, state, in);

    if (decodeRet == 0)
    {
        SDL_SurfacePtr scaled;
        scaled.reset(SDL_CreateRGBSurfaceFrom(out.data(), w, h, 24, w * 3, 0xFF, 0x00FF, 0x0000FF, 0));

        SDL_SurfacePtr ownedBuffer(SDL_CreateRGBSurface(0, w, h, 24, 0xFF, 0x00FF, 0x0000FF, 0));
        SDL_BlitSurface(scaled.get(), NULL, ownedBuffer.get(), NULL);
        return ownedBuffer;
    }
    else
    {
        LOG_ERROR(lodepng_error_text(decodeRet));
    }
    return nullptr;
}

static void ApplyDelta(SDL_Surface* deltaSurface, SDL_Surface* originalCameraSurface)
{
    auto dst = static_cast<u8*>(deltaSurface->pixels);
    auto w = static_cast<unsigned int>(deltaSurface->w);
    auto h = static_cast<unsigned int>(deltaSurface->h);

    uint8_t *src = static_cast<uint8_t*>(originalCameraSurface->pixels);
    // Apply delta image over linearly interpolated game cam image
    for (auto y = 0u; y < h; ++y)
    {
        const f32 src_rel_y = 1.f*y / (h - 1); // 0..1
        for (auto x = 0u; x < w; ++x)
        {
            const f32 src_rel_x = 1.f*x / (w - 1); // 0..1

            int src_x = (int)std::floor(src_rel_x*originalCameraSurface->w - 0.5f);
            int src_y = (int)std::floor(src_rel_y*originalCameraSurface->h - 0.5f);

            int src_x_plus = src_x + 1;
            int src_y_plus = src_y + 1;

            f32 lerp_x = src_rel_x*originalCameraSurface->w - (src_x + 0.5f);
            f32 lerp_y = src_rel_y*originalCameraSurface->h - (src_y + 0.5f);
            assert(lerp_x >= 0.0f);
            assert(lerp_x <= 1.0f);
            assert(lerp_y >= 0.0f);
            assert(lerp_y <= 1.0f);

            // Limit source pixels inside image
            src_x = std::max(src_x, 0);
            src_y = std::max(src_y, 0);
            src_x_plus = std::min(src_x_plus, originalCameraSurface->w - 1);
            src_y_plus = std::min(src_y_plus, originalCameraSurface->h - 1);

            // Indices to four neighboring pixels
            const int src_indices[4] =
            {
                src_x * 3 + originalCameraSurface->pitch*src_y,
                src_x_plus * 3 + originalCameraSurface->pitch*src_y,
                src_x * 3 + originalCameraSurface->pitch*src_y_plus,
                src_x_plus * 3 + originalCameraSurface->pitch*src_y_plus
            };

            const int dst_ix = (x + w*y) * 3;

            for (int comp = 0; comp < 3; ++comp)
            {
                // 4 neighboring texels
                f32 a = src[src_indices[0] + comp] / 255.f;
                f32 b = src[src_indices[1] + comp] / 255.f;
                f32 c = src[src_indices[2] + comp] / 255.f;
                f32 d = src[src_indices[3] + comp] / 255.f;

                // 2d linear interpolation
                f32 orig = (a*(1 - lerp_x) + b*lerp_x)*(1 - lerp_y) + (c*(1 - lerp_x) + d*lerp_x)*lerp_y;
                f32 delta = dst[dst_ix + comp] / 255.f;

                // "Grain extract" has been used in creating the delta image
                f32 merged = orig + delta - 0.5f;
                dst[dst_ix + comp] = (uint8_t)(std::max(std::min(merged * 255 + 0.5f, 255.f), 0.0f));
            }
        }
    }
}

std::unique_ptr<Oddlib::IBits> ResourceLocator::DoLocateCamera(const char* resourceName, bool ignoreMods)
{
    std::string deltaName;
    std::string modName;

    for (const DataPaths::FileSystemInfo& fs : mDataPaths.ActiveDataPaths())
    {
        if (fs.mIsMod && !ignoreMods)
        {
            if (modName.empty())
            {
                modName = std::string(resourceName) + ".png";
            }

            // Check for mod trying to fully replace camera with its own, or simply a new camera
            if (fs.mFileSystem->FileExists(modName))
            {
                auto stream = fs.mFileSystem->Open(modName);
                auto surface = LoadPng(*stream);
                if (surface)
                {
                    LOG_INFO("Loaded new or replacement camera from mod " << fs.mDataSetName);
                    return Oddlib::MakeBits(std::move(surface));
                }
            }

            if (deltaName.empty())
            {
                deltaName = std::string(resourceName) + ".cam.bmp.png"; // TODO: Rename to something sane
            }

            if (fs.mFileSystem->FileExists(deltaName))
            {
                auto cam = DoLocateCamera(resourceName, true);
                if (cam)
                {
                    auto originalCameraSurface = cam->GetSurface();
                    auto deltaPngStream = fs.mFileSystem->Open(deltaName);
                    auto deltaSurface = LoadPng(*deltaPngStream);
                    if (deltaSurface)
                    {
                        if (CanDeltaBeApplied(originalCameraSurface->w, originalCameraSurface->h, deltaSurface->w, deltaSurface->h))
                        {
                            ApplyDelta(deltaSurface.get(), originalCameraSurface);
                            LOG_INFO("Applied camera upscaling delta from " << fs.mDataSetName);
                            return Oddlib::MakeBits(std::move(deltaSurface));
                        }
                    }
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
                            LOG_INFO("Loaded original camera from " << fs.mDataSetName);
                            return Oddlib::MakeBits(*stream);
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
                    return IMovie::Factory(resourceName, audioController, std::move(stream), std::move(subTitles), location.mStartSector, location.mEndSector);
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