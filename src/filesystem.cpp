#include "filesystem.hpp"
#include "logger.hpp"
#include "string_util.hpp"
#include "SDL.h"
#include "oddlib/stream.hpp"

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
//    mResourcePaths.push(ResourcePathAbstraction(path, priority));
}

std::unique_ptr<Oddlib::IStream> FileSystem::OpenResource(const std::wstring& name)
{
    // Look in each resource path by priority

    return nullptr;
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
                mBasePath += "../../";
            }
        }
    }
    else
    {
        mBasePath = "./";
        LOG_ERROR("SDL_GetBasePath failed, falling back to ./");
    }
    LOG_INFO("basePath is " << mBasePath);
}
