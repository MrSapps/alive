#include "gamefilesystem.hpp"
#ifdef _WIN32
#   include <windows.h>
#   undef DeleteFile
#   if !defined(__MINGW32__)
#       pragma warning(push)
#       pragma warning(disable:4917)
#       pragma warning(disable:4091) //  'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#   endif
#   include <shlobj.h>
#   if !defined(__MINGW32__)
#       pragma warning(pop)
#   endif
#else
#   include <dirent.h>
#   include <unistd.h>
#   include <sys/stat.h>
#   include <sys/types.h>
#endif

#include <SDL.h>

#include "string_util.hpp"
#include "logger.hpp"

bool GameFileSystem::Init()
{
    std::unique_lock<std::recursive_mutex> lock(mMutex);

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
    std::unique_lock<std::recursive_mutex> lock(mMutex);
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
# if defined(__MINGW32__)
        return std::string(userPathW.begin(), userPathW.end());
#else
        return string_util::wstring_to_utf8(userPathW);
#endif
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
    std::unique_lock<std::recursive_mutex> lock(mMutex);

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
    std::unique_lock<std::recursive_mutex> lock(mMutex);

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
    std::unique_lock<std::recursive_mutex> lock(mMutex);

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
    std::unique_lock<std::recursive_mutex> lock(mMutex);
    std::string ret = path;
    for (const auto& namedPath : mNamedPaths)
    {
        string_util::replace_all(ret, namedPath.first, namedPath.second);
    }
    NormalizePath(ret);
    return ret;
}
