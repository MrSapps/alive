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

#include "gamedefinition.hpp" // DataPaths
#include "imgui/imgui.h"

namespace Oddlib
{
    class IBits;
}

namespace Detail
{
    const auto kFnvOffsetBais = 14695981039346656037U;
    const auto kFnvPrime = 1099511628211U;

    inline void HashInternal(Uint64& result, const char* s)
    {
        if (s)
        {
            while (*s)
            {
                result = (kFnvPrime * result) ^ static_cast<unsigned char>(*s);
                s++;
            }
        }
    }

    inline void HashInternal(Uint64& result, const std::string& s)
    {
        HashInternal(result, s.c_str());
    }

    // Treat each byte of u32 as unsigned char in FNV hashing
    inline void HashInternal(Uint64 result, u32 value)
    {
        result = (kFnvPrime * result) ^ static_cast<unsigned char>(value >> 24);
        result = (kFnvPrime * result) ^ static_cast<unsigned char>(value >> 16);
        result = (kFnvPrime * result) ^ static_cast<unsigned char>(value >> 8);
        result = (kFnvPrime * result) ^ static_cast<unsigned char>(value);
    }
}

template<typename... Args>
inline Uint64 StringHash(Args... args)
{
    Uint64 result = Detail::kFnvOffsetBais;
    // Cast to void since initializer_list isn't actually used, its just here so that we 
    // can call HashInternal for each parameter
    (void)std::initializer_list<int>
    {
        (Detail::HashInternal(result, std::forward<Args>(args)), 0)...
    };
    return result;
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
            mMusicMaps = std::move(rhs.mMusicMaps);
            mSoundBankMaps = std::move(rhs.mSoundBankMaps);
            mSoundEffectMaps = std::move(rhs.mSoundEffectMaps);
        }
        return *this;
    }

    ResourceMapper(IFileSystem& fileSystem, const char* resourceMapFile)
    {
        auto stream = fileSystem.Open(resourceMapFile);
        assert(stream != nullptr);
        const auto jsonData = stream->LoadAllToString();
        Parse(jsonData);
    }

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

    struct PathLocation
    {
        std::string mDataSetName;
        std::string mDataSetFileName;
    };

    struct PathMapping
    {
        u32 mId;
        u32 mCollisionOffset;
        u32 mIndexTableOffset;
        u32 mObjectOffset;
        u32 mNumberOfScreensX;
        u32 mNumberOfScreensY;
        std::vector<PathLocation> mLocations;

        const PathLocation* Find(const std::string& dataSetName) const
        {
            for (const PathLocation& location : mLocations)
            {
                if (location.mDataSetName == dataSetName)
                {
                    return &location;
                }
            }
            return nullptr;
        }
    };

    struct MusicMapping
    {
        std::string mDataSetName;
        std::string mFileName;
        std::string mLvl;
        std::string mSoundBankName;
        u32 mIndex;
    };

    struct SoundBankMapping
    {
        std::string mDataSetName;
        std::string mLvl;
        std::string mVabBody;
        std::string mVabHeader;
    };

    struct SoundEffectMapping
    {
        std::string mDataSetName;
        u32 mNote = 0;
        u32 mProgram = 0;
        std::string mSoundBankName;
        f32 mMinPitch = 1.0f;
        f32 mMaxPitch = 1.0f;
    };

    const MusicMapping* FindMusic(const char* resourceName)
    {
        const auto it = mMusicMaps.find(resourceName);
        if (it != std::end(mMusicMaps))
        {
            return &it->second;
        }
        return nullptr;
    }

    const SoundBankMapping* FindSoundBank(const char* resourceName)
    {
        const auto it = mSoundBankMaps.find(resourceName);
        if (it != std::end(mSoundBankMaps))
        {
            return &it->second;
        }
        return nullptr;
    }

    const SoundEffectMapping* FindSoundEffect(const char* resourceName)
    {
        const auto it = mSoundEffectMaps.find(resourceName);
        if (it != std::end(mSoundEffectMaps))
        {
            return &it->second;
        }
        return nullptr;
    }

    const PathMapping* FindPath(const char* resourceName)
    {
        const auto it = mPathMaps.find(resourceName);
        if (it != std::end(mPathMaps))
        {
            return &it->second;
        }
        return nullptr;
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
private:

    std::map<std::string, AnimMapping> mAnimMaps;
    std::map<std::string, FmvMapping> mFmvMaps;
    std::map<std::string, PathMapping> mPathMaps;
    std::map<std::string, MusicMapping> mMusicMaps;
    std::map<std::string, SoundBankMapping> mSoundBankMaps;
    std::map<std::string, SoundEffectMapping> mSoundEffectMaps;

    friend class Level; // TODO: Temp debug ui
    friend class Sound; // TODO: Temp debug ui
    friend class Fmv; // TODO: Temp debug ui

    void Parse(const std::string& json)
    {
        TRACE_ENTRYEXIT;

        rapidjson::Document document;
        document.Parse(json.c_str());

        const auto& docRootArray = document.GetArray();

        for (auto& it : docRootArray)
        {
            if (it.HasMember("animations"))
            {
                const auto& animationsArray = it["animations"].GetArray();
                for (auto& obj : animationsArray)
                {
                    ParseAnimResourceJson(obj);
                }
            }
            
            if (it.HasMember("fmvs"))
            {
                const auto& fmvsArray = it["fmvs"].GetArray();
                for (auto& obj : fmvsArray)
                {
                    ParseFmvResourceJson(obj);
                }
            }
            
            if (it.HasMember("paths"))
            {
                const auto& pathsArray = it["paths"].GetArray();
                for (auto& obj : pathsArray)
                {
                    ParsePathResourceJson(obj);
                }
            }

            if (it.HasMember("lvls"))
            {
                ParseFileLocations(it);
            }

            if (it.HasMember("musics"))
            {
                ParseMusics(it);
            }

            if (it.HasMember("sound_banks"))
            {
                ParseSoundBanks(it);
            }

            if (it.HasMember("sound_effects"))
            {
                ParseSoundEffects(it);
            }
        }
    }

    std::map<std::string, std::map<std::string, std::vector<DataSetFileAttributes>>> mFileLocations;

    template<typename JsonObject>
    void ParseSoundBanks(const JsonObject& obj)
    {
        const auto& soundBanks = obj["sound_banks"].GetArray();
        for (const auto& soundBank : soundBanks)
        {
            const std::string& name = soundBank["resource_name"].GetString();
            SoundBankMapping mapping;
            mapping.mDataSetName = soundBank["data_set"].GetString();
            mapping.mLvl = soundBank["lvl"].GetString();
            mapping.mVabBody = soundBank["vab_body"].GetString();
            mapping.mVabHeader = soundBank["vab_header"].GetString();

            mSoundBankMaps[name] = mapping;
        }
    }

    template<typename JsonObject>
    void ParseSoundEffects(const JsonObject& obj)
    {
        const auto& soundEffects = obj["sound_effects"].GetArray();
        for (const auto& soundEffect : soundEffects)
        {
            const std::string& name = soundEffect["resource_name"].GetString();
            SoundEffectMapping mapping;
            mapping.mDataSetName = soundEffect["data_set"].GetString();
            mapping.mNote = soundEffect["note"].GetInt();
            mapping.mProgram = soundEffect["program"].GetInt();
            mapping.mSoundBankName = soundEffect["sound_bank"].GetString();
            if (soundEffect.HasMember("pitch_range"))
            {
                const auto& pitchRange = soundEffect["pitch_range"].GetArray();
                if (pitchRange.Size() != 2)
                {
                    LOG_ERROR("pitch_range must contain 2 entries min and max, but contains " << pitchRange.Size() << " entries");
                }
                else
                {
                    mapping.mMinPitch = pitchRange[0].GetFloat();
                    mapping.mMaxPitch = pitchRange[1].GetFloat();
                }
            }

            mSoundEffectMaps[name] = mapping;
        }
    }

    template<typename JsonObject>
    void ParseMusics(const JsonObject& obj)
    {
        const auto& musics = obj["musics"].GetArray();
        for (const auto& musicRecord : musics)
        {
            const std::string& name = musicRecord["resource_name"].GetString();
            MusicMapping mapping;
            mapping.mSoundBankName = musicRecord["sound_bank"].GetString();
            mapping.mDataSetName = musicRecord["data_set"].GetString();
            mapping.mFileName = musicRecord["file_name"].GetString();
            mapping.mIndex = musicRecord["index"].GetInt();
            mapping.mLvl = musicRecord["lvl"].GetString();

            mMusicMaps[name] = mapping;
        }
    }

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
    void ParsePathResourceJson(const JsonObject& obj)
    {
        PathMapping mapping;
        mapping.mId = obj["id"].GetInt();
        mapping.mCollisionOffset = obj["collision_offset"].GetInt();
        mapping.mIndexTableOffset = obj["object_indextable_offset"].GetInt();
        mapping.mObjectOffset = obj["object_offset"].GetInt();
        mapping.mNumberOfScreensX = obj["number_of_screens_x"].GetInt();
        mapping.mNumberOfScreensY = obj["number_of_screens_y"].GetInt();

        const auto& locations = obj["locations"].GetArray();
        for (auto& locationRecord : locations)
        {
            const auto& dataSet = locationRecord["dataset"].GetString();
            const auto& dataSetFileName = locationRecord["file_name"].GetString();
            mapping.mLocations.push_back(PathLocation{ dataSet, dataSetFileName });
        }

        const auto& name = obj["resource_name"].GetString();
        mPathMaps[name] = mapping;
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
    u32 mCounter = 0;
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
        std::shared_ptr<ObjectType> sptr(uptr.release(), AutoRemoveFromContainerDeleter<KeyType, ObjectType>(&container, key));
        container.insert(std::make_pair(key, sptr));
        return sptr;
    }

    template<class ObjectType, class KeyType, class Container>
    std::shared_ptr<ObjectType> Get(KeyType& key, Container& container)
    {
        auto it = container.find(key);
        if (it != std::end(container))
        {
            return it->second.lock();
        }
        return nullptr;
    }

    std::map<std::string, std::weak_ptr<Oddlib::LvlArchive>> mOpenLvls;
    std::map<std::string, std::weak_ptr<Oddlib::AnimationSet>> mAnimationSets;
};

// TODO: Provide higher level abstraction so this can be a vab/seq or ogg stream
class IMusic
{
public:
    IMusic(std::unique_ptr<Vab> vab, std::unique_ptr<Oddlib::IStream> seq)
        : mVab(std::move(vab)), mSeqData(std::move(seq))
    {

    }

    virtual ~IMusic() = default;

    std::unique_ptr<Vab> mVab;
    std::unique_ptr<Oddlib::IStream> mSeqData;
};


// TODO: Provide higher level abstraction
class ISoundEffect
{
public:
    ISoundEffect(std::unique_ptr<Vab> vab, u32 program, u32 note, f32 minPitch, f32 maxPitch)
        : mVab(std::move(vab)), mProgram(program), mNote(note), mMinPitch(minPitch), mMaxPitch(maxPitch)
    {

    }
    virtual ~ISoundEffect() = default;

    std::unique_ptr<Vab> mVab;
    u32 mProgram = 0;
    u32 mNote = 0;
    f32 mMinPitch = 0.0f;
    f32 mMaxPitch = 0.0f;
};

class ResourceLocator
{
public:
    ResourceLocator(const ResourceLocator&) = delete;
    ResourceLocator& operator =(const ResourceLocator&) = delete;
    ResourceLocator(ResourceMapper&& resourceMapper, DataPaths&& dataPaths);
    ~ResourceLocator();

    // TOOD: Provide limited interface to this?
    DataPaths& GetDataPaths()
    {
        return mDataPaths;
    }

    std::string LocateScript(const char* scriptName);

    std::unique_ptr<ISoundEffect> LocateSoundEffect(const char* resourceName);
    std::unique_ptr<IMusic> LocateMusic(const char* resourceName);

    // TODO: Should be returning higher level abstraction
    std::unique_ptr<Oddlib::Path> LocatePath(const char* resourceName);
    std::unique_ptr<Oddlib::IBits> LocateCamera(const char* resourceName);
    std::unique_ptr<class IMovie> LocateFmv(class IAudioController& audioController, const char* resourceName);
    std::unique_ptr<Animation> LocateAnimation(const char* resourceName);


    // This method should be used for debugging only - i.e so we can compare what resource X looks like
    // in dataset A and B.
    std::unique_ptr<Animation> LocateAnimation(const char* resourceName, const char* dataSetName);

    std::vector<std::tuple<const char*, const char*, bool>> DebugUi(const char* dataSetFilter, const char* nameFilter);
private:
    std::unique_ptr<Vab> LocateSoundBank(const char* resourceName);

    std::unique_ptr<Animation> DoLocateAnimation(const DataPaths::FileSystemInfo& fs, const char* resourceName, const ResourceMapper::AnimMapping& animMapping);

    std::unique_ptr<IMovie> DoLocateFmv(IAudioController& audioController, const char* resourceName, const DataPaths::FileSystemInfo& fs, const ResourceMapper::FmvMapping& fmvMapping);

    std::unique_ptr<Oddlib::IBits> DoLocateCamera(const char* resourceName, bool ignoreMods);

    std::shared_ptr<Oddlib::LvlArchive> OpenLvl(IFileSystem& fs, const std::string& dataSetName, const std::string& lvlName);

    ResourceCache mCache;
    ResourceMapper mResMapper;
    DataPaths mDataPaths;

    friend class Fmv; // TODO: Temp debug ui
    friend class Level; // TODO: Temp debug ui
    friend class Sound; // TODO: Temp debug ui
};
