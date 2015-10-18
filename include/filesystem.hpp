#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "SDL_stdinc.h"
#include "oddlib/lvlarchive.hpp"
#include "logger.hpp"

namespace Oddlib
{
    class IStream;
}
struct GuiContext;

class UserSettingsFs
{
public:
    std::unique_ptr<Oddlib::IStream> Open(const std::string& /*name*/)
    {
        LOG_ERROR("UserSettings file not implemented");
        throw Oddlib::Exception("Not implemented");
    }

    std::unique_ptr<Oddlib::IStream> Save(const std::string& /*name*/)
    {
        LOG_ERROR("UserSettings file not implemented");
        throw Oddlib::Exception("Not implemented");
    }
    
    bool Init() { return true; }
};

class GameDataFs
{
public:
    GameDataFs() = default;
    std::unique_ptr<Oddlib::IStream> Open(const std::string& name);
    std::string BasePath() const { return mBasePath; }
    bool Init() { InitBasePath(); return true; }
private:
    void InitBasePath();
    std::string mBasePath;
};

class FileSystem;
class ResourcePathAndModsFs
{
public:
    void AddResourcePath(const std::string& path, int priority);
    void ClearAllResourcePaths();
    void AddPcToPsxMapping(const std::string& pcName, const std::string& psxName);
    std::unique_ptr<Oddlib::IStream> Open(const std::string& name);
    std::unique_ptr<Oddlib::IStream> OpenFmv(const std::string& name, bool pickBiggest = false);
    std::unique_ptr<Oddlib::IStream> OpenLvlFileChunkById(const std::string& lvl, const std::string& name, Uint32 id);
    std::unique_ptr<Oddlib::IStream> OpenLvlFileChunkByType(const std::string& lvl, const std::string& name, Uint32 type);
private:
    class IResourcePathAbstraction
    {
    public:
        IResourcePathAbstraction(const IResourcePathAbstraction&) = delete;
        IResourcePathAbstraction& operator = (const IResourcePathAbstraction&) const = delete;
        virtual ~IResourcePathAbstraction() = default;
        IResourcePathAbstraction(const std::string& path, int priority) : mPath(path), mPriority(priority) { }
        int Priority() const { return mPriority; }
        void SetPriority(int priority) { mPriority = priority; }
        const std::string& Path() const { return mPath; }
        virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) = 0;
        virtual Sint64 Exists(const std::string& fileName) const = 0;
    private:
        std::string mPath;
        int mPriority = 0;
    };

    std::unique_ptr<IResourcePathAbstraction> MakeResourcePath(std::string path, int priority);

    class Directory : public IResourcePathAbstraction
    {
    public:
        Directory(const std::string& path, int priority);
        virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override;
        virtual Sint64 Exists(const std::string& fileName) const override;
    };

    class RawCdImagePath : public IResourcePathAbstraction
    {
    public:
        RawCdImagePath(const std::string& path, int priority);
        virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override;
        virtual Sint64 Exists(const std::string& fileName) const override;
    private:
        std::unique_ptr<class RawCdImage> mCdImage;
    };

    void SortPaths();

    IResourcePathAbstraction* FindFile(const std::string& name, bool pickBiggest);
    std::unique_ptr<Oddlib::IStream> FindFile(const std::vector<std::string>& names, bool pickBiggestOfSameName);

private:
    friend class FileSystem;
    std::vector<std::unique_ptr<IResourcePathAbstraction>> mResourcePaths;
    std::map<std::string, std::string> mPcToPsxMappings;
};

class FileSystem
{
public:
    FileSystem() = default;
    bool Init();
    UserSettingsFs& UserSettings() {  return mUserSettingsFs; }
    GameDataFs& GameData() { return mGameDataFs; }
    ResourcePathAndModsFs& ResourcePaths() { return mResourceandModsFs; }
    void DebugUi(GuiContext &gui);
private:
    void InitResourcePaths();
private:
    UserSettingsFs mUserSettingsFs;
    GameDataFs mGameDataFs;
    ResourcePathAndModsFs mResourceandModsFs;
};
