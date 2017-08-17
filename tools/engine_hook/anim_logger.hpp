#pragma once

#include <windows.h>
#include <memory>
#include <map>
#include <string>
#include "types.hpp"
#include "resourcemapper.hpp"

namespace Oddlib
{
    class AnimSerializer;
}

class AnimLogger
{
public:
    Oddlib::AnimSerializer* AddAnim(u32 id, u8* ptr, u32 size);
    void Add(u32 id, u32 idx, Oddlib::AnimSerializer* anim);
    std::string Find(u32 id, u32 idx);
    void ResourcesInit();
    void LogAnim(u32 id, u32 idx);
    void ReloadJson();
    std::string LookUpSoundEffect(const std::string vabName, DWORD program, DWORD note);
    const ResourceMapper::PathMapping* LoadPath(const std::string& name);
private:
    std::unique_ptr<class IFileSystem> mFileSystem;
    std::unique_ptr<ResourceMapper> mResources;
    std::map<u32, std::unique_ptr<Oddlib::AnimSerializer>> mAnimCache;
    std::map<std::pair<u32, u32>, Oddlib::AnimSerializer*> mAnims;
    std::string mLastResName;
    std::map<std::string, std::string> mSoundNameCache;
};

AnimLogger& GetAnimLogger();
