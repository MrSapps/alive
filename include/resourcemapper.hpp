#pragma once

#include "jsonxx/jsonxx.h"
#include <unordered_map>
#include <map>
#include <set>

#include "string_util.hpp"
#include "logger.hpp"
#include <initializer_list>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/anim.hpp"
#include "abstractrenderer.hpp"
#include "oddlib/path.hpp"
#include "oddlib/audio/vab.hpp"
#include "debug.hpp"
#include "proxy_rapidjson.hpp"
#include "filesystem.hpp"
#include "sound_resources.hpp"
#include "paths_json.hpp"
#include "gamedefinition.hpp" // DataPaths
#include "imgui/imgui.h"
#include <future>

namespace Oddlib
{
    class IBits;
}

inline std::vector<u8> StringToVector(const std::string& str)
{
    return std::vector<u8>(str.begin(), str.end());
}

class ResourceMapper
{
public:
    ResourceMapper() = default;

    ResourceMapper(ResourceMapper&& rhs)
    {
        *this = std::move(rhs);
    }

    ResourceMapper& operator = (ResourceMapper&& rhs)
    {
        if (this != &rhs)
        {
            mAnimMaps = std::move(rhs.mAnimMaps);
            mFmvMaps = std::move(rhs.mFmvMaps);
            mFileLocations = std::move(rhs.mFileLocations);
            mPathMaps = std::move(rhs.mPathMaps);
            mSoundResources = rhs.mSoundResources;
        }
        return *this;
    }

    ResourceMapper(IFileSystem& fileSystem,
        const char* dataSetContentsFile,
        const char* animationResourceFile,
        const char* soundResourceMapFile,
        const char* pathsResourceMapFile,
        const char* fmvsResourceMapFile);

    struct AnimFile
    {
        std::string mFile;
        u32 mId;
        u32 mAnimationIndex;
    };

    struct AnimFileLocations
    {
        std::string mDataSetName;

        // Where copies of this animation file chunk live, usually each LVL
        // has the file chunk duplicated a few times.
        std::vector<AnimFile> mFiles;
    };



    struct FmvFileLocation
    {
        std::string mDataSetName;
        std::string mFileName;
        u32 mStartSector;
        u32 mEndSector;
    };

    struct FmvMapping
    {
        std::vector<FmvFileLocation> mLocations;
    };


    const FmvMapping* FindFmv(const char* resourceName)
    {
        const auto it = mFmvMaps.find(resourceName);
        if (it != std::end(mFmvMaps))
        {
            return &it->second;
        }
        return nullptr;
    }

    const SoundResource* FindSound(const char* resourceName)
    {
        return mSoundResources.FindSound(resourceName);
    }


    struct AnimMapping
    {
        // Shared between all data sets, the default blending mode
        u32 mBlendingMode;

        // Which datasets this animation lives in
        std::vector<AnimFileLocations> mLocations;
    };

    const AnimMapping* FindAnimation(const char* resourceName)
    {
        const auto& am = mAnimMaps.find(resourceName);
        if (am != std::end(mAnimMaps))
        {
            return &am->second;
        }
        return nullptr;
    }

    struct DataSetFileAttributes
    {
        // LVL this data set file lives in
        std::string mLvlName;

        // Is this lvl a PSX lvl?
        bool mIsPsx;

        // Is this Ao or Ae data?
        bool mIsAo;

        // Do we need to scale the frame offsets down to make them correct?
        bool mScaleFrameOffsets;
    };

    const std::vector<DataSetFileAttributes>* FindFileLocation(const char* dataSetName, const char* fileName)
    {
        auto fileIt = mFileLocations.find(fileName);
        if (fileIt != std::end(mFileLocations))
        {
            auto dataSetIt = fileIt->second.find(dataSetName);
            if (dataSetIt != std::end(fileIt->second))
            {
                return &dataSetIt->second;
            }
        }
        return nullptr;
    }


    const DataSetFileAttributes* FindFileAttributes(const std::string& fileName, const std::string& dataSetName, const std::string& lvlName)
    {
        auto fileNameIt = mFileLocations.find(fileName);
        if (fileNameIt == std::end(mFileLocations))
        {
            return nullptr;
        }

        auto dataSetIt = fileNameIt->second.find(dataSetName);
        if (dataSetIt == std::end(fileNameIt->second))
        {
            return nullptr;
        }

        // TODO: More performant look up
        for (const DataSetFileAttributes& attr : dataSetIt->second)
        {
            if (attr.mLvlName == lvlName)
            {
                return &attr;
            }
        }

        return nullptr;
    }

    // Used in testing only - todo make protected
    void AddAnimMapping(const std::string& resourceName, const AnimMapping& mapping)
    {
        mAnimMaps[resourceName] = mapping;
    }

    // Debug UI
    std::vector<std::tuple<const char*, const char*, bool>> DebugUi(const char* dataSetFilter, const char* nameFilter);

    struct UiItem
    {
        std::string mResourceName;
        std::string mLabel;
        bool mLoad = false;
        std::vector<std::string> mItems;
    };

    struct UiContext
    {
        std::vector<UiItem> mItems;
    };
    UiContext mUi;

    const PathsJson& PathMaps() const { return mPathMaps; }
    PathsJson& PathMaps() { return mPathMaps; }

private:
    std::map<std::string, AnimMapping> mAnimMaps;
    std::map<std::string, FmvMapping> mFmvMaps;
    PathsJson mPathMaps;

    SoundResources mSoundResources;

    friend class Sound; // TODO: Temp debug ui
    friend class FmvDebugUi; // TODO: Temp debug ui
    friend class AnimLogger; // TODO: Hook access

    void ParseDataSetContentsJson(const std::string& json)
    {
        TRACE_ENTRYEXIT;

        rapidjson::Document document;
        document.Parse(json.c_str());

        const auto& docRootArray = document.GetArray();
        for (auto& it : docRootArray)
        {
            if (it.HasMember("lvls"))
            {
                ParseFileLocations(it);
            }
        }
    }

    void ParseAnimationResourcesJson(const std::string& json)
    {
        TRACE_ENTRYEXIT;

        rapidjson::Document document;
        document.Parse(json.c_str());

        const auto& docRootArray = document.GetArray();
        for (auto& it : docRootArray)
        {
            const auto& animationsArray = it["animations"].GetArray();
            for (auto& obj : animationsArray)
            {
                ParseAnimResourceJson(obj);
            }
        }
    }

    void ParsePathResourceJson(const std::string& json)
    {
        TRACE_ENTRYEXIT;

        rapidjson::Document document;
        document.Parse(json.c_str());
        mPathMaps.FromJson(document);
    }

    void ParseFmvResourceJson(const std::string& json)
    {
        TRACE_ENTRYEXIT;

        rapidjson::Document document;
        document.Parse(json.c_str());

        const auto& docRootArray = document.GetArray();
        for (auto& it : docRootArray)
        {
            if (it.HasMember("fmvs"))
            {
                const auto& fmvsArray = it["fmvs"].GetArray();
                for (auto& obj : fmvsArray)
                {
                    ParseFmvResourceJson(obj);
                }
            }
        }
    }

    std::map<std::string, std::map<std::string, std::vector<DataSetFileAttributes>>> mFileLocations;

    template<typename JsonObject>
    void ParseFileLocations(const JsonObject& obj)
    {
        // This is slightly tricky as we reverse the mapping of the data that is in the json file
        // the json file maps a data set, if its PSX or not, its lvls and lvl contents.
        // But we want to map a file name to what LVLs it lives in, and a LVL to what data sets it lives in
        // and if that data set is PSX or not.
        DataSetFileAttributes dataSetAttributes;
        const std::string& dataSetName = obj["data_set_name"].GetString();
        dataSetAttributes.mIsPsx = obj["is_psx"].GetBool();
        dataSetAttributes.mIsAo = obj["is_ao"].GetBool();
        dataSetAttributes.mScaleFrameOffsets = obj["scale_frame_offsets"].GetBool();

        const auto& lvls = obj["lvls"].GetArray();
        for (const auto& lvlRecord : lvls)
        {
            const std::string& lvlName = lvlRecord["name"].GetString();

            std::set<std::string> lvlFiles;
            JsonDeserializer::ReadStringArray("files", lvlRecord, lvlFiles);
            for (const std::string& fileName : lvlFiles)
            {
                dataSetAttributes.mLvlName = lvlName;
                mFileLocations[fileName][dataSetName].push_back(dataSetAttributes);
            }
        }
    }

    static u32 ToBlendMode(const std::string& str)
    {
        if (str == "normal")
        {
            return 0;
        }
        else if (str == "B100F100")
        {
            return 1;
        }
        // TODO: Other required modes

        LOG_WARNING("Unknown blending mode: " << str);
        return 0;
    }

    template<typename JsonObject>
    void ParseAnimResourceJson(const JsonObject& obj)
    {
        AnimMapping mapping;

        mapping.mBlendingMode = ToBlendMode(obj["blend_mode"].GetString());

        ParseAnimResourceLocations(obj, mapping);

        const auto& name = obj["name"].GetString();
        if (!mAnimMaps.insert(std::make_pair(name, mapping)).second)
        {
            throw std::runtime_error(std::string(name) + " animation resource was already added! Remove the duplicate from the json.");
        }
    }

    template<typename JsonObject>
    void ParseFmvResourceJson(const JsonObject& obj)
    {
        FmvMapping mapping;

        ParseFmvResourceLocations(obj, mapping);

        const auto& name = obj["name"].GetString();
        mFmvMaps[name] = mapping;
    }

    template<typename JsonObject>
    void ParseAnimResourceLocations(const JsonObject& obj, AnimMapping& mapping)
    {
        const auto& locations = obj["locations"].GetArray();
        for (auto& locationRecord : locations)
        {
            AnimFileLocations animFileLocations;
            animFileLocations.mDataSetName = locationRecord["dataset"].GetString();

            const auto& files = locationRecord["files"].GetArray();
            animFileLocations.mFiles.reserve(files.Size());
            for (auto& file : files)
            {
                AnimFile animFile;

                animFile.mFile = file["filename"].GetString();
                animFile.mId = file["id"].GetInt();
                animFile.mAnimationIndex = file["index"].GetInt();

                animFileLocations.mFiles.push_back(animFile);
            }

            mapping.mLocations.push_back(animFileLocations);
        }
    }

    template<typename JsonObject>
    void ParseFmvResourceLocations(const JsonObject& obj, FmvMapping& mapping)
    {
        const auto& locations = obj["locations"].GetArray();
        for (auto& locationRecord : locations)
        {
            FmvFileLocation fmvFileLocation = {};
            fmvFileLocation.mDataSetName = locationRecord["dataset"].GetString();
            fmvFileLocation.mFileName = locationRecord["file"].GetString();
            if (locationRecord.HasMember("start_sector"))
            {
                fmvFileLocation.mStartSector = locationRecord["start_sector"].GetInt();
                fmvFileLocation.mEndSector = locationRecord["end_sector"].GetInt();
            }
            mapping.mLocations.push_back(fmvFileLocation);
        }
    }
public:
    const SoundBankLocation* FindSoundBank(const std::string& soundBank);
    const MusicTheme* FindSoundTheme(const char* themeName);
    const std::vector<SoundResource>& GetSoundResources() const;
    const std::vector<SoundBankLocation>& GetSoundBankResources() const;
};

// TODO: Move to physics
template<class T>
inline bool PointInRect(T px, T py, T x, T y, T w, T h)
{
    if (px < x) return false;
    if (py < y) return false;
    if (px >= x + w) return false;
    if (py >= y + h) return false;

    return true;
}

class Animation
{
public:
    // Keeps the LVL and AnimSet shared pointers in scope for as long as the Animation lives.
    // On destruction if its the last instance of the lvl/animset the lvl will be closed and removed
    // from the cache, and the animset will be deleted/freed.
    struct AnimationSetHolder
    {
    public:
        AnimationSetHolder(std::shared_ptr<Oddlib::LvlArchive> sLvlPtr, std::shared_ptr<Oddlib::AnimationSet> sAnimSetPtr, u32 animIdx);
        const Oddlib::Animation& Animation() const;
        u32 MaxW() const;
        u32 MaxH() const;
    private:
        std::shared_ptr<Oddlib::LvlArchive> mLvlPtr;
        std::shared_ptr<Oddlib::AnimationSet> mAnimSetPtr;
        const Oddlib::Animation* mAnim;
    };
    Animation(const Animation&) = delete;
    Animation& operator = (const Animation&) = delete;
    Animation() = delete;
    Animation(AnimationSetHolder anim, bool isPsx, bool scaleFrameOffsets, u32 defaultBlendingMode, const std::string& sourceDataSet);
    s32 FrameCounter() const;
    bool Update();
    bool IsLastFrame() const;
    bool IsComplete() const;
    void Render(AbstractRenderer& rend, bool flipX, int layer, s32 xpos, s32 ypos, AbstractRenderer::eCoordinateSystem coordinateSystem = AbstractRenderer::eWorld) const;
    void Render(AbstractRenderer& rend, bool flipX, int layer, AbstractRenderer::eCoordinateSystem coordinateSystem = AbstractRenderer::eWorld) const;
    void SetFrame(u32 frame);
    void Restart();
    bool Collision(s32 x, s32 y) const;
    void SetXPos(s32 xpos);
    void SetYPos(s32 ypos);
    s32 XPos() const;
    s32 YPos() const;
    u32 MaxW() const;
    u32 MaxH() const;
    s32 FrameNumber() const;
    u32 NumberOfFrames() const;
    void SetScale(f32 scale);
private:
    void RenderImpl(AbstractRenderer& rend, bool flipX, int layer, s32 xpos, s32 ypos, AbstractRenderer::eCoordinateSystem coordinateSystem) const;

    // 640 (pc xres) / 368 (psx xres) = 1.73913043478 scale factor
    const static f32 kPcToPsxScaleFactor;

    f32 ScaleX() const;

    AnimationSetHolder mAnim;
    bool mIsPsx = false;
    bool mScaleFrameOffsets = false;
    std::string mSourceDataSet;
    AbstractRenderer::eBlendModes mBlendingMode = AbstractRenderer::eBlendModes::eNormal;

    // The "FPS" of the animation, set to 1 first so that the first Update() brings us from frame -1 to 0
    u32 mFrameDelay = 1;

    // When >= mFrameDelay, mFrameNum is incremented
    u32 mCounter = 0; // TODO: Switch to using Engine::mGlobalFrameCounter
    s32 mFrameNum = -1;
    
    s32 mXPos = 100;
    s32 mYPos = 100;
    f32 mScale = 1.0f;

    bool mIsLastFrame = false;
    bool mCompleted = false;
};

template<typename KeyType, typename ValueType>
class AutoRemoveFromContainerDeleter
{
private:
    using Container = std::map<KeyType, std::weak_ptr<ValueType>>;
    Container* mContainer;
    KeyType mKey;
public:
    AutoRemoveFromContainerDeleter(Container* container, KeyType key)
        : mContainer(container), mKey(key)
    {
    }

    void operator()(ValueType* ptr)
    {
        mContainer->erase(mKey);
        delete ptr;
    }
};

// Thread safe
class ResourceCache
{
public:
    ResourceCache() = default;
    ResourceCache(const ResourceCache&) = delete;
    ResourceCache& operator = (const ResourceCache&) = delete;

    std::shared_ptr<Oddlib::LvlArchive> AddLvl(std::unique_ptr<Oddlib::LvlArchive> uptr, const std::string& dataSetName, const std::string& lvlArchiveFileName)
    {
        std::string key = dataSetName + lvlArchiveFileName;
        return Add(key, mOpenLvls, std::move(uptr));
    }

    std::shared_ptr<Oddlib::LvlArchive> GetLvl(const std::string& dataSetName, const std::string& lvlArchiveFileName)
    {
        std::string key = dataSetName + lvlArchiveFileName;
        return Get<Oddlib::LvlArchive>(key, mOpenLvls);
    }

    std::shared_ptr<Oddlib::AnimationSet> AddAnimSet(std::unique_ptr<Oddlib::AnimationSet> uptr, const std::string& dataSetName, const std::string& lvlArchiveFileName, const std::string& lvlFileName, u32 chunkId)
    {
        std::string key = dataSetName + lvlArchiveFileName + lvlFileName + std::to_string(chunkId);
        return Add(key, mAnimationSets, std::move(uptr));
    }

    std::shared_ptr<Oddlib::AnimationSet> GetAnimSet(const std::string& dataSetName, const std::string& lvlArchiveFileName, const std::string& lvlFileName, u32 chunkId)
    {
        std::string key = dataSetName + lvlArchiveFileName + lvlFileName + std::to_string(chunkId);
        return Get<Oddlib::AnimationSet>(key, mAnimationSets);
    }

private:
    template<class ObjectType, class KeyType, class Container>
    std::shared_ptr<ObjectType> Add(KeyType& key, Container& container, std::unique_ptr<ObjectType> uptr)
    {
        assert(container.find(key) == std::end(container));
        std::lock_guard<std::mutex> lock(mMutex);
        std::shared_ptr<ObjectType> sptr(uptr.release(), AutoRemoveFromContainerDeleter<KeyType, ObjectType>(&container, key));
        container.insert(std::make_pair(key, sptr));
        return sptr;
    }

    template<class ObjectType, class KeyType, class Container>
    std::shared_ptr<ObjectType> Get(KeyType& key, Container& container)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = container.find(key);
        if (it != std::end(container))
        {
            return it->second.lock();
        }
        return nullptr;
    }

    std::mutex mMutex;
    std::map<std::string, std::weak_ptr<Oddlib::LvlArchive>> mOpenLvls;
    std::map<std::string, std::weak_ptr<Oddlib::AnimationSet>> mAnimationSets;
};

// TODO: Provide higher level abstraction
class ISound
{
public:
    virtual void Load() = 0;
    virtual ~ISound() = default;
    virtual void DebugUi() = 0;
    virtual void Play(f32* stream, u32 len) = 0;
    virtual bool AtEnd() const = 0;
    virtual void Restart() = 0;
    virtual void Update() = 0;
    virtual void Stop() = 0;
    virtual const std::string& Name() const = 0;
};
using UP_ISound = std::unique_ptr<ISound>;

class BaseSeqSound : public ISound
{
public:
    BaseSeqSound(const char* soundName, std::unique_ptr<Vab> vab);
    virtual void DebugUi() override;
    virtual void Play(f32* stream, u32 len) override;
    virtual bool AtEnd() const override;
    virtual void Restart() override;
    virtual void Update() override;
    virtual void Stop() override;
    virtual const std::string& Name() const override;

    std::unique_ptr<Vab> mVab;
    std::unique_ptr<class SequencePlayer> mSeqPlayer;
    std::string mSoundName;
};

class SingleSeqSampleSound : public BaseSeqSound
{
public:
    SingleSeqSampleSound(const char* soundName, std::unique_ptr<Vab> vab, u32 program, u32 note, u32 minPitch, u32 maxPitch, u32 /*vol*/);
    virtual void Load() override;
    u32 mProgram = 0;
    u32 mNote = 0;
    u32 mMinPitch = 0;
    u32 mMaxPitch = 0;
};

class SeqSound : public BaseSeqSound
{
public:
    SeqSound(const char* soundName, std::unique_ptr<Vab> vab, std::unique_ptr<Oddlib::IStream> seq);
    virtual void Load() override;

    std::unique_ptr<Oddlib::IStream> mSeqData;
};

using future_UP_Path = std::future<Oddlib::UP_Path>;
using up_future_UP_Path = std::unique_ptr<future_UP_Path>;

class ResourceLocator
{
public:
    ResourceLocator(const ResourceLocator&) = delete;
    ResourceLocator& operator =(const ResourceLocator&) = delete;
    ResourceLocator(ResourceMapper&& resourceMapper, DataPaths&& dataPaths);
    ~ResourceLocator();

    // TOOD: Provide limited interface to this?
    DataPaths& GetDataPaths() // Not thread safe
    {
        return mDataPaths;
    }
    
    // Not thread safe - only used by debug path browsers etc
    const PathsJson& PathMaps() const { return mResMapper.PathMaps(); }
    PathsJson& PathMaps() { return mResMapper.PathMaps(); }

    std::future<std::string> LocateScript(const std::string& scriptName);

    std::future<std::unique_ptr<ISound>> LocateSound(const std::string& resourceName, const std::string& explicitSoundBankName = "", bool useMusicRec = true, bool useSfxRec = true);
    std::future<const MusicTheme*> LocateSoundTheme(const std::string& themeName);

    // TODO: Should be returning higher level abstraction
    up_future_UP_Path LocatePath(const std::string& resourceName);
    std::future<std::unique_ptr<Oddlib::IBits>> LocateCamera(const std::string& resourceName);
    std::future<std::unique_ptr<class IMovie>> LocateFmv(class IAudioController& audioController, const std::string& resourceName, const ResourceMapper::FmvFileLocation* location);
    std::future<std::unique_ptr<Animation>> LocateAnimation(const std::string& resourceName);

    // This method should be used for debugging only - i.e so we can compare what resource X looks like
    // in dataset A and B.
    std::future<std::unique_ptr<Animation>> LocateAnimation(const std::string& resourceName, const std::string& dataSetName);

    // Not thread safe
    std::vector<std::tuple<const char*, const char*, bool>> DebugUi(const char* dataSetFilter, const char* nameFilter);

    std::future<std::unique_ptr<Vab>> LocateVab(const std::string& dataSetName, const std::string& baseVabName);
private:
    std::unique_ptr<ISound> DoLoadSoundEffect(const char* resourceName, const DataPaths::FileSystemInfo& fs, const std::string& strSb, const SoundEffectResource& sfxRes, const SoundEffectResourceLocation& sfxResLoc);
    std::unique_ptr<ISound> DoLoadSoundMusic(const char* resourceName, const DataPaths::FileSystemInfo& fs, const std::string& strSb, const MusicResource& sfxRes);

    std::unique_ptr<Animation> DoLocateAnimation(const DataPaths::FileSystemInfo& fs, const char* resourceName, const ResourceMapper::AnimMapping& animMapping);

    std::unique_ptr<IMovie> DoLocateFmv(IAudioController& audioController, const char* resourceName, const DataPaths::FileSystemInfo& fs, const ResourceMapper::FmvMapping& fmvMapping);

    std::unique_ptr<IMovie> DoLocateFmvFromFileLocation(const ResourceMapper::FmvFileLocation& location, const DataPaths::FileSystemInfo& fs, const char* resourceName, IAudioController& audioController);

    std::unique_ptr<Oddlib::IBits> DoLocateCamera(const char* resourceName, bool ignoreMods);

    std::shared_ptr<Oddlib::LvlArchive> OpenLvl(IFileSystem& fs, const std::string& dataSetName, const std::string& lvlName);

    ResourceCache mCache;
    ResourceMapper mResMapper;
    DataPaths mDataPaths;

    friend class FmvDebugUi; // TODO: Temp debug ui
    friend class World; // TODO: Temp debug ui
    friend class Sound; // TODO: Temp debug ui

    std::mutex mMutex;
public:
    const std::vector<SoundResource>& GetSoundResources() const;
    const std::vector<SoundBankLocation>& GetSoundBankResources() const;
};
