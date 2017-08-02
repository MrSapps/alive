#include "filesystem.hpp"

#include <regex>

#ifdef _WIN32
#include <windows.h>
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

#include "string_util.hpp"
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include "zipfilesystem.hpp"
#include "directorylimitedfilesystem.hpp"
#include "cdromfilesystem.hpp"

/*static*/ std::unique_ptr<IFileSystem> IFileSystem::Factory(IFileSystem& fs, const std::string& path)
{
    TRACE_ENTRYEXIT;

    if (path.empty())
    {
        return nullptr;
    }

    std::string pathCopy = path;
    const bool isFile = fs.FileExists(pathCopy);
    std::unique_ptr<IFileSystem> ret;
    if (isFile)
    {
        if (string_util::ends_with(pathCopy, ".bin", true))
        {
            LOG_INFO("Creating ISO FS for " << pathCopy);
            ret = std::make_unique<CdIsoFileSystem>(pathCopy.c_str());
        }
        else if (string_util::ends_with(pathCopy, ".zip", true) || string_util::ends_with(pathCopy, ".exe", true))
        {
            LOG_INFO("Creating ZIP FS for " << pathCopy);
            ret = std::make_unique<ZipFileSystem>(pathCopy.c_str(), fs);
        }
        else
        {
            LOG_ERROR("Unknown archive type for: " << pathCopy);
            return nullptr;
        }
    }
    else
    {
        LOG_INFO("Creating dir view FS for " << pathCopy);
        ret = std::make_unique<DirectoryLimitedFileSystem>(fs, pathCopy);
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

/*static*/ std::string IFileSystem::Parent(const std::string& path)
{
    std::string ret = path;
    NormalizePath(ret);

    auto pos = ret.find_last_of('/');
    if (pos != std::string::npos)
    {
        ret = ret.substr(0, pos);
    }
    return ret;
}

/*static*/ void IFileSystem::NormalizePath(std::string& path)
{
    string_util::replace_all(path, "\\", "/");
    string_util::replace_all(path, "//", "/");
}

/*static*/ bool IFileSystem::WildCardMatcher(const std::string& text, std::string wildcardPattern, EMatchType caseSensitive)
{
    // Escape all regex special chars
    EscapeRegex(wildcardPattern);

    // Convert chars '*?' back to their regex equivalents
    string_util::replace_all(wildcardPattern, "\\?", ".");
    string_util::replace_all(wildcardPattern, "\\*", ".*");

    std::regex pattern(wildcardPattern,
        caseSensitive == MatchCase ?
        std::regex_constants::ECMAScript :
        std::regex_constants::ECMAScript | std::regex_constants::icase);

    return std::regex_match(text, pattern);
}

/*static*/ void IFileSystem::EscapeRegex(std::string& regex)
{
    string_util::replace_all(regex, "\\", "\\\\");
    string_util::replace_all(regex, "^", "\\^");
    string_util::replace_all(regex, ".", "\\.");
    string_util::replace_all(regex, "$", "\\$");
    string_util::replace_all(regex, "|", "\\|");
    string_util::replace_all(regex, "(", "\\(");
    string_util::replace_all(regex, ")", "\\)");
    string_util::replace_all(regex, "[", "\\[");
    string_util::replace_all(regex, "]", "\\]");
    string_util::replace_all(regex, "*", "\\*");
    string_util::replace_all(regex, "+", "\\+");
    string_util::replace_all(regex, "?", "\\?");
    string_util::replace_all(regex, "/", "\\/");
}

std::unique_ptr<Oddlib::IStream> OSBaseFileSystem::Open(const std::string& fileName)
{
    std::unique_lock<std::recursive_mutex> lock(mMutex);
    return std::make_unique<Oddlib::FileStream>(ExpandPath(fileName), Oddlib::IStream::ReadMode::ReadOnly);
}

std::unique_ptr<Oddlib::IStream> OSBaseFileSystem::Create(const std::string& fileName)
{
    std::unique_lock<std::recursive_mutex> lock(mMutex);
    return std::make_unique<Oddlib::FileStream>(ExpandPath(fileName), Oddlib::IStream::ReadMode::ReadWrite);
}

#ifdef _WIN32
struct FindCloseDeleter
{
    typedef HANDLE pointer;
    void operator()(HANDLE hFind)
    {
        if (hFind != INVALID_HANDLE_VALUE)
        {
            ::FindClose(hFind);
        }
    }
};
typedef std::unique_ptr<HANDLE, FindCloseDeleter> FindCloseHandle;

std::vector<std::string> OSBaseFileSystem::DoEnumerate(const std::string& directory, bool files, const char* filter)
{
    std::unique_lock<std::recursive_mutex> lock(mMutex);

    std::vector<std::string> ret;
    WIN32_FIND_DATA findData = {};
    // Wild carding is supported directly by the host OS API
    const std::string dirPath = ExpandPath(directory) + (filter ? std::string("/") + filter : "/*");
    FindCloseHandle ptr(::FindFirstFile(dirPath.c_str(), &findData));
    if (ptr.get() != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!IsDots(findData.cFileName))
            {
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !files)
                {
                    ret.emplace_back(findData.cFileName);
                }
                else if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && files)
                {
                    ret.emplace_back(findData.cFileName);
                }
            }
        } while (::FindNextFile(ptr.get(), &findData));
        LOG_INFO(ret.size() << " items enumerated from " << dirPath);
    }
    else
    {
        LOG_ERROR("Failed to enumerate directory " << dirPath);
    }
    return ret;
}
#else
struct closedirDeleter
{
    typedef DIR* pointer;
    void operator()(DIR* dir)
    {
        closedir(dir);
    }
};
typedef std::unique_ptr<DIR, closedirDeleter> closedirHandle;

std::vector<std::string> OSBaseFileSystem::DoEnumerate(const std::string& directory, bool files, const char* filter)
{
    std::unique_lock<std::mutex> lock(mMutex);

    std::vector<std::string> ret;
    const std::string dirPath = ExpandPath(directory) + "/";
    closedirHandle dir(opendir(dirPath.c_str()));
    if (dir)
    {
        dirent* ent = nullptr;
        do
        {
            ent = readdir(dir.get());
            if (ent)
            {
                const std::string itemName = ent->d_name;
                if (!IsDots(itemName))
                {
                    bool add = true;
                    if (filter)
                    {
                        const std::string strFilter(filter);
                        add = WildCardMatcher(itemName, strFilter, IgnoreCase);
                    }

                    if (add)
                    {
                        struct stat statbuf;
                        LOG_INFO("Stat of " << dirPath + "/" + itemName);
                        if (stat((dirPath + "/" + itemName).c_str(), &statbuf) == 0)
                        {
                            const bool isFile = !S_ISDIR(statbuf.st_mode);
                            if ((files && isFile) || (!files && !isFile))
                            {
                                ret.emplace_back(itemName);
                                LOG_INFO("Name is " << itemName);
                            }
                        }
                    }
                }
            }
        } while (ent);
        LOG_INFO(ret.size() << " items enumerated from " << dirPath);
    }
    else
    {
        LOG_ERROR("Failed to enumerate directory " << dirPath);
    }
    return ret;
}

#endif


void OSBaseFileSystem::DeleteFile(const std::string& path)
{
    std::unique_lock<std::recursive_mutex> lock(mMutex);
#ifdef _WIN32
    if (!::DeleteFileA(path.c_str())) // TODO: Should convert to unicode to handle unicode paths
    {
        LOG_ERROR("Failed to delete " << path << " error " << ::GetLastError());
    }
#else
    if (remove(path.c_str()) != 0)
    {
        LOG_ERROR("Failed to delete " << path << " error " << errno);
    }
#endif
}

#ifdef _WIN32
bool OSBaseFileSystem::FileExists(std::string& fileName)
{
    std::unique_lock<std::recursive_mutex> lock(mMutex);

    const auto name = ExpandPath(fileName);
    const DWORD dwAttrib = GetFileAttributes(name.c_str()); // TODO: Unicode
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
#else
bool OSBaseFileSystem::FileExists(std::string& fileName)
{
    std::unique_lock<std::mutex> lock(mMutex);

    const std::string dirPart = Parent(fileName);
    std::vector<std::string> files = DoEnumerate(dirPart, true, nullptr);

    std::string filePart;
    if (dirPart != fileName)
    {
        filePart = fileName.substr(dirPart.length() + 1);
    }

    for (const auto& file : files)
    {
        if (string_util::iequals(file, filePart))
        {
            fileName = dirPart + "/" + file;
            return true;
        }
    }
    return false;
}
#endif
