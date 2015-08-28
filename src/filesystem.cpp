#include "filesystem.hpp"
#include "logger.hpp"
#include "string_util.hpp"
#include "SDL.h"
#include "oddlib/stream.hpp"
#include "oddlib/cdromfilesystem.hpp"
#include <algorithm>
#include <fstream>

#ifdef _WIN32
const char kDirSeperator = '\\';
#else
const char kDirSeperator = '/';
#endif

static std::string GetFilePath(const std::string& basePath, const std::string& fileName)
{
    std::string fullFileName;
    if (string_util::ends_with(basePath, std::string(1, kDirSeperator)))
    {
        fullFileName = basePath + fileName;
    }
    else
    {
        fullFileName = basePath + kDirSeperator + fileName;
    }
    return fullFileName;
}

static bool FileExists(const std::string& fileName)
{
    std::ifstream fileStream;
    fileStream.open(fileName, std::ios::in | std::ios::binary | std::ios::ate);
    if (fileStream)
    {
        LOG_INFO("File: " << fileName << " not found");
        return true;
    }
    return false;
}

FileSystem::Directory::Directory(const std::string& path, int priority)
    : IResourcePathAbstraction(path, priority)
{
}

std::unique_ptr<Oddlib::IStream> FileSystem::Directory::Open(const std::string& fileName)
{
    return std::make_unique<Oddlib::Stream>(fileName);
}

bool FileSystem::Directory::Exists(const std::string& fileName) const
{
    return FileExists(fileName);
}

FileSystem::RawCdImagePath::RawCdImagePath(const std::string& path, int priority) 
    : IResourcePathAbstraction(path, priority)
{
    mCdImage = std::make_unique<RawCdImage>(path);
}

std::unique_ptr<Oddlib::IStream> FileSystem::RawCdImagePath::Open(const std::string& fileName)
{
    return mCdImage->ReadFile(fileName, true);
}

bool FileSystem::RawCdImagePath::Exists(const std::string& fileName) const
{
    return mCdImage->FileExists(fileName);
}

FileSystem::FileSystem()
{

}

FileSystem::~FileSystem()
{

}

bool FileSystem::Init()
{
    InitBasePath();
    return true;
}

void FileSystem::AddResourcePath(const std::string& path, int priority)
{
    mResourcePaths.emplace_back(std::make_unique<Directory>(path, priority));
    std::sort(mResourcePaths.begin(), mResourcePaths.end(),
        [](const std::unique_ptr<IResourcePathAbstraction>& lhs, const std::unique_ptr<IResourcePathAbstraction>& rhs)
    {
        return lhs->Priority() < rhs->Priority();
    });
}

bool FileSystem::Exists(const std::string& name) const
{
    return FileExists(GetFilePath(mBasePath,name));
}

std::unique_ptr<Oddlib::IStream> FileSystem::OpenResource(const std::string& name)
{
    LOG_INFO("Opening resource: " << name);

    // Look in each resource path by priority
    for (auto& resourceLocation : mResourcePaths)
    {
        if (resourceLocation->Exists(name))
        {
            return resourceLocation->Open(name);
        }
    }
    return nullptr;
}

std::unique_ptr<Oddlib::IStream> FileSystem::Open(const std::string& name)
{
    const auto fileName = GetFilePath(mBasePath, name);
    LOG_INFO("Opening file: " << fileName);
    return std::make_unique<Oddlib::Stream>(fileName);
}

void FileSystem::InitBasePath()
{
    char* pBasePath = SDL_GetBasePath();
    if (pBasePath)
    {
        mBasePath = std::string(pBasePath);
        SDL_free(pBasePath);

        // If it looks like we're running from the IDE/dev build then attempt to fix up the path to the correct location to save
        // manually setting the correct working directory.
        const bool bIsDebugPath = string_util::contains(mBasePath, "/alive/bin/") || string_util::contains(mBasePath, "\\alive\\bin\\");
        if (bIsDebugPath)
        {
            if (string_util::contains(mBasePath, "/alive/bin/"))
            {
                LOG_WARNING("We appear to be running from the IDE (Linux) - fixing up basePath to be ../");
                mBasePath += "../";
            }
            else
            {
                LOG_WARNING("We appear to be running from the IDE (Win32) - fixing up basePath to be ../");
                mBasePath += "..\\..\\";
            }
        }
    }
    else
    {
        mBasePath = "." + kDirSeperator;
        LOG_ERROR("SDL_GetBasePath failed, falling back to ." << kDirSeperator);
    }
    LOG_INFO("basePath is " << mBasePath);
}
