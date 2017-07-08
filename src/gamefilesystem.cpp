#include "gamefilesystem.hpp"
#ifdef _WIN32
#include <windows.h>

#undef DeleteFile

#pragma warning(push)
#pragma warning(disable:4917)
#pragma warning(disable:4091) //  'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared 
#include <shlobj.h>
#pragma warning(pop)
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <SDL.h>

#include "string_util.hpp"
#include "logger.hpp"

bool GameFileSystem::Init()
{
    auto basePath = InitBasePath();
    if (basePath.empty())
    {
        LOG_ERROR("Failed to resolve {GameDir}");
        return false;
    }
    mNamedPaths["{GameDir}"] = basePath;

    auto userPath = InitUserPath();
    if (userPath.empty())
    {
        LOG_ERROR("Failed to resolve {UserDir}");
        return false;
    }
    mNamedPaths["{UserDir}"] = userPath;

    auto cachePath = InitCachePath();
    if (cachePath.empty())
    {
        LOG_ERROR("Failed to resolve {CacheDir}");
        return false;
    }
    mNamedPaths["{CacheDir}"] = cachePath;

    return true;
}

std::string GameFileSystem::FsPath() const
{
    return mNamedPaths.find("{GameDir}")->second;
}
#ifdef _WIN32
static std::string W32CreateDirectory(const wchar_t* dirName)
{
    wchar_t myDocsPath[MAX_PATH] = {};
    const HRESULT hr = ::SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, myDocsPath);
    if (SUCCEEDED(hr))
    {
        std::wstring userPathW = myDocsPath;
        userPathW += L"\\";
        userPathW += dirName;
        if (!::CreateDirectoryW(userPathW.c_str(), NULL))
        {
            const DWORD error = ::GetLastError();
            if (error != ERROR_ALREADY_EXISTS)
            {
                LOG_ERROR("Create CreateDirectoryA failed with Win32 error: " << error);
            }
        }
        return string_util::wstring_to_utf8(userPathW);
    }
    else
    {
        LOG_ERROR("SHGetFolderPathA failed with HRESULT: " << hr);
        return "";
    }
}
#endif

std::string GameFileSystem::InitUserPath()
{
    std::string userPath;
#ifdef _WIN32
    userPath = W32CreateDirectory(L"ALIVE User files");
#else
    char* pUserPath = SDL_GetPrefPath("PaulsApps", "ALIVE User files");
    if (pUserPath)
    {
        userPath = pUserPath;
        SDL_free(pUserPath);
    }
#endif
    NormalizePath(userPath);
    return userPath;
}

std::string GameFileSystem::InitCachePath()
{
    std::string userPath;
#ifdef _WIN32
    userPath = W32CreateDirectory(L"ALIVE User files\\CacheFiles");
#else
    // TODO: Check if this will really create the dir on linux/osx
    char* pUserPath = SDL_GetPrefPath("PaulsApps", "ALIVE User files/CacheFiles");
    if (pUserPath)
    {
        userPath = pUserPath;
        SDL_free(pUserPath);
    }
#endif
    NormalizePath(userPath);
    return userPath;
}

std::string GameFileSystem::InitBasePath()
{
    char* pBasePath = SDL_GetBasePath();
    std::string basePath;
    if (pBasePath)
    {
        basePath = pBasePath;
        SDL_free(pBasePath);

        // If it looks like we're running from the IDE/dev build then attempt to fix up the path to the correct location to save
        // manually setting the correct working directory.
        const bool bIsDebugPath = string_util::contains(basePath, "/alive/bin/") || string_util::contains(basePath, "\\alive\\bin\\");
        if (bIsDebugPath)
        {
            if (string_util::contains(basePath, "/alive/bin/"))
            {
                LOG_WARNING("We appear to be running from the IDE (Linux) - fixing up basePath to be ../");
                basePath += "../";
            }
            else
            {
                LOG_WARNING("We appear to be running from the IDE (Win32) - fixing up basePath to be ../");
                basePath += "..\\..\\";
            }
        }
    }
    else
    {
        LOG_ERROR("SDL_GetBasePath failed");
        return "";
    }
    LOG_INFO("basePath is " << basePath);
    NormalizePath(basePath);
    return basePath;
}

std::string GameFileSystem::ExpandPath(const std::string& path)
{
    std::string ret = path;
    for (const auto& namedPath : mNamedPaths)
    {
        string_util::replace_all(ret, namedPath.first, namedPath.second);
    }
    NormalizePath(ret);
    return ret;
}
