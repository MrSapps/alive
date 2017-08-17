#include "anim_logger.hpp"
#include "gamefilesystem.hpp"
#include "window_hooks.hpp"
#include "debug_dialog.hpp"

Oddlib::AnimSerializer* AnimLogger::AddAnim(u32 id, u8* ptr, u32 size)
{
    auto it = mAnimCache.find(id);
    if (it == std::end(mAnimCache))
    {
        std::vector<u8> data(
            reinterpret_cast<u8*>(ptr),
            reinterpret_cast<u8*>(ptr) + size);

        Oddlib::MemoryStream ms(std::move(data));
        mAnimCache[id] = std::make_unique<Oddlib::AnimSerializer>(ms, false);
        return AddAnim(id, ptr, size);
    }
    else
    {
        return it->second.get();
    }
}

void AnimLogger::Add(u32 id, u32 idx, Oddlib::AnimSerializer* anim)
{
    auto key = std::make_pair(id, idx);
    auto it = mAnims.find(key);
    if (it == std::end(mAnims))
    {
        mAnims[key] = anim;
    }

    LogAnim(id, idx);
}

std::string AnimLogger::Find(u32 id, u32 idx)
{
    std::string toFind = Utils::IsAe() ? "AePc" : "AoPc";
    for (const auto& mapping : mResources->mAnimMaps)
    {
        for (const auto& location : mapping.second.mLocations)
        {
            if (location.mDataSetName == toFind)
            {
                for (const auto& file : location.mFiles)
                {
                    if (file.mAnimationIndex == idx && file.mId == id)
                    {
                        return mapping.first;
                    }
                }
            }
        }
    }
    return "";
}

void AnimLogger::ResourcesInit()
{
    TRACE_ENTRYEXIT;
    mFileSystem = std::make_unique<GameFileSystem>();
    if (!mFileSystem->Init())
    {
        LOG_ERROR("File system init failure");
    }
    mResources = std::make_unique<ResourceMapper>(*mFileSystem,
        "../../data/dataset_contents.json",
        "../../data/animations.json",
        "../../data/sounds.json",
        "../../data/paths.json",
        "../../data/fmvs.json");
}

void AnimLogger::LogAnim(u32 id, u32 idx)
{
    if (!mFileSystem)
    {
        ResourcesInit();
    }

    std::string resName = Find(id, idx);
    if (resName != mLastResName)
    {
        //std::string s = "id: " + std::to_string(id) + " idx: " + std::to_string(idx) + " resource name: " + resName;
        //std::cout << "ANIM: " << s << std::endl; // use cout directly, don't want the function name etc here
        mLastResName = resName;

        gDebugUi->LogAnimation(resName);
    }

    //::Sleep(128);
}

void AnimLogger::ReloadJson()
{
    std::cout << "RELOAD RESOURCES" << std::endl;
    mFileSystem = nullptr;
    mResources = nullptr;
}

std::string AnimLogger::LookUpSoundEffect(const std::string vabName, DWORD program, DWORD note)
{
    std::string cacheKey = vabName + "_" + std::to_string(program) + "_" + std::to_string(note);
    auto it = mSoundNameCache.find(cacheKey);
    if (it != std::end(mSoundNameCache))
    {
        return it->second;
    }

    std::string vabNameNoExt = string_util::split(vabName, '.')[0];

    std::string foundName;
    for (const SoundBankLocation& sbl : mResources->mSoundResources.mSoundBanks)
    {
        if (sbl.mSoundBankName == vabNameNoExt)
        {
            foundName = sbl.mName;
            break;
        }
    }

    if (!foundName.empty())
    {
        for (const SoundResource& sr : mResources->mSoundResources.mSounds)
        {
            for (const SoundEffectResourceLocation& serl : sr.mSoundEffect.mSoundBanks)
            {
                if (serl.mProgram == (s32)program && serl.mTone == (s32)note)
                {
                    for (const std::string& sb : serl.mSoundBanks)
                    {
                        if (sb == foundName)
                        {
                            mSoundNameCache[cacheKey] = sr.mResourceName;
                            return sr.mResourceName;
                        }
                    }
                }
            }
        }
    }

    mSoundNameCache[cacheKey] = cacheKey;
    return cacheKey;
}

const ResourceMapper::PathMapping* AnimLogger::LoadPath(const std::string& name)
{
    if (!mResources)
    {
        ResourcesInit();
    }
    return mResources->FindPath(name.c_str());
}

AnimLogger& GetAnimLogger()
{
    static AnimLogger logger;
    return logger;
}
