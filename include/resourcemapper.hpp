#pragma once

#include "jsonxx/jsonxx.h"
#include <unordered_map>
#include <regex>
#include <map>
#include <set>

#ifdef _WIN32
#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4917)
#include <shlobj.h>
#pragma warning(pop)
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "string_util.hpp"
#include "oddlib/cdromfilesystem.hpp"
#include "logger.hpp"
#include <initializer_list>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/anim.hpp"
#include "renderer.hpp"
#include "oddlib/path.hpp"
#include "oddlib/audio/vab.hpp"
#include "debug.hpp"

#if defined(_MSC_VER) && _MSC_VER > 1800
#pragma warning(push)
#pragma warning(disable: 4464)
#endif
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#if defined(_MSC_VER) && _MSC_VER > 1800
#pragma warning(pop)
#endif

// Remove more windows.h pollution
#undef GetObject
#undef min
#undef max


#if defined(_MSC_VER) && _MSC_VER > 1800
#pragma warning(push)
#pragma warning(disable:4464)
#endif
#include "rapidjson/document.h"
#if defined(_MSC_VER) && _MSC_VER > 1800
#pragma warning(pop)
#endif

namespace Oddlib
{
    class IBits;
}

namespace JsonDeserializer
{
    namespace Detail
    {
        template<class SizeType>
        void Reserve(std::vector<std::string>& c, SizeType size)
        {
            c.reserve(size);
        }

        template<class SizeType>
        void Reserve(std::set<std::string>& /*c*/, SizeType /*size*/)
        {
            // Can't preallocate a set
        }

        template<class ElementType>
        void Add(std::vector<std::string>& c, ElementType element)
        {
            c.push_back(element);
        }

        template<class ElementType>
        void Add(std::set<std::string>& c, ElementType element)
        {
            c.insert(element);
        }
    }

    template<typename ObjectType, typename StringVector>
    void ReadStringArray(const char* jsonArrayName, const ObjectType& jsonObject, StringVector& result)
    {
        if (jsonObject.HasMember(jsonArrayName))
        {
            const auto& jsonArray = jsonObject[jsonArrayName].GetArray();
            Detail::Reserve(result, jsonArray.Size());
            for (const auto& arrayElement : jsonArray)
            {
                Detail::Add(result, arrayElement.GetString());
            }
        }
    }
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

class IFileSystem
{
public:
    IFileSystem() = default;
    IFileSystem(const IFileSystem&) = delete;
    IFileSystem& operator = (const IFileSystem&) = delete;

    virtual ~IFileSystem() = default;
    
    virtual bool Init() { return true; }

    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& fileName) = 0;
    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) = 0;

    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) = 0;
    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) = 0;

    virtual bool FileExists(const std::string& fileName) = 0;
    virtual std::string FsPath() const = 0;

    enum EMatchType
    {
        IgnoreCase,
        MatchCase
    };

    static std::unique_ptr<IFileSystem> Factory(IFileSystem& fs, const std::string& path);
    static std::string Parent(const std::string& path);
protected:
    struct DirectoryAndFileName
    {
        DirectoryAndFileName(std::string path)
        {
            NormalizePath(path);

            auto lastDirPos = path.find_last_of('/');
            if (lastDirPos != std::string::npos)
            {
                mDir = path.substr(0, lastDirPos);
                mFile = path.substr(lastDirPos + 1);
            }
            else
            {
                mFile = path;
            }
        }

        std::string mDir;
        std::string mFile;
    };

    static void NormalizePath(std::string& path)
    {
        string_util::replace_all(path, "\\", "/");
        string_util::replace_all(path, "//", "/");
    }

    static bool WildCardMatcher(const std::string& text, std::string wildcardPattern, EMatchType caseSensitive)
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

    static void EscapeRegex(std::string& regex)
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
};


class OSBaseFileSystem : public IFileSystem
{
private:
    static bool IsDots(const std::string& name)
    {
        return name == "." || name == "..";
    }
public:

    virtual bool Init() override
    {
        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override
    {
        return std::make_unique<Oddlib::FileStream>(ExpandPath(fileName), Oddlib::IStream::ReadMode::ReadOnly);
    }

    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& fileName) override
    {
        return std::make_unique<Oddlib::FileStream>(ExpandPath(fileName), Oddlib::IStream::ReadMode::ReadWrite);
    }

#ifdef _WIN32
private:
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

    std::vector<std::string> DoEnumerate(const std::string& directory, bool files, const char* filter)
    {
        std::vector<std::string> ret;
        WIN32_FIND_DATA findData = {};
        // Wild carding is supported directly by the host OS API
        const std::string dirPath = ExpandPath(directory) + (filter ?  std::string("/") + filter : "/*");
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

public:
    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) override
    {
        return DoEnumerate(directory, true, filter);
    }

    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) override
    {
        return DoEnumerate(directory, false, nullptr);
    }

#else
private:
    struct closedirDeleter
    {
        typedef DIR* pointer;
        void operator()(DIR* dir)
        {
            closedir(dir);
        }
    };
    typedef std::unique_ptr<DIR, closedirDeleter> closedirHandle;

    std::vector<std::string> DoEnumerate(const std::string& directory, const char* filter, bool files)
    {
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

public:
    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) override
    {
        return DoEnumerate(directory, filter, true);
    }

    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) override
    {
        return DoEnumerate(directory, nullptr, false);
    }

#endif

#ifdef _WIN32
    bool FileExists(const std::string& fileName) override
    {
        const auto name = ExpandPath(fileName);
        const DWORD dwAttrib = GetFileAttributes(name.c_str());
        return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }
#else
    bool FileExists(const std::string& fileName) override
    {
        struct stat statbuf = {};
        return stat(ExpandPath(fileName).c_str(), &statbuf) == 0 && !S_ISDIR(statbuf.st_mode);
    }
#endif

    virtual std::string ExpandPath(const std::string& path) = 0;
};

class GameFileSystem : public OSBaseFileSystem
{
public:
    virtual bool Init() override final
    {
        auto basePath = InitBasePath();
        if (basePath.empty())
        {
            LOG_ERROR("Failed to resolve ${GameDir}");
            return false;
        }
        mNamedPaths["{GameDir}"] = basePath;

        auto userPath = InitUserPath();
        if (userPath.empty())
        {
            LOG_ERROR("Failed to resolve ${UserDir}");
            return false;
        }
        mNamedPaths["{UserDir}"] = userPath;
        return true;
    }

    virtual std::string FsPath() const override
    {
        return mNamedPaths.find("{GameDir}")->second;
    }

    std::string InitUserPath()
    {
        std::string userPath;
#ifdef _WIN32
        wchar_t myDocsPath[MAX_PATH] = {};
        const HRESULT hr = ::SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, myDocsPath);
        if (SUCCEEDED(hr))
        {
            std::wstring userPathW = myDocsPath;
            userPathW += L"\\ALIVE User files";
            if (!::CreateDirectoryW(userPathW.c_str(), NULL))
            {
                const DWORD error = ::GetLastError();
                if (error != ERROR_ALREADY_EXISTS)
                {
                    LOG_ERROR("Create CreateDirectoryA failed with Win32 error: " << error);
                }
            }
            userPath = string_util::wstring_to_utf8(userPathW);
        }
        else
        {
            LOG_ERROR("SHGetFolderPathA failed with HRESULT: " << hr);
        }
#else
        char* pUserPath = SDL_GetPrefPath("PaulsApps", "ALIVE");

        if (pUserPath)
        {
            userPath = pUserPath;
            SDL_free(pUserPath);
        }
#endif
        NormalizePath(userPath);
        return userPath;
    }

    std::string InitBasePath()
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

    virtual std::string ExpandPath(const std::string& path) override final
    {
        std::string ret = path;
        for (const auto& namedPath : mNamedPaths)
        {
            string_util::replace_all(ret, namedPath.first, namedPath.second);
        }
        NormalizePath(ret);
        return ret;
    }

private:
    std::map<std::string, std::string> mNamedPaths;
};

class DirectoryLimitedFileSystem : public IFileSystem
{
public:
    DirectoryLimitedFileSystem(IFileSystem& fs, const std::string& directory)
        : mFs(fs)
    {
        mBasePath = directory;
        NormalizePath(mBasePath);
    }

    virtual std::string FsPath() const override
    {
        return mBasePath;
    }

    virtual bool Init() override final
    {
        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override final
    {
        return mFs.Open(LimitPath(fileName));
    }

    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& fileName) override final
    {
        return mFs.Create(LimitPath(fileName));
    }

    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) override final
    {
        return mFs.EnumerateFiles(LimitPath(directory), filter);
    }

    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) override final
    {
        return mFs.EnumerateFolders(LimitPath(directory));
    }

    virtual bool FileExists(const std::string& fileName) override final
    {
        return mFs.FileExists(LimitPath(fileName));
    }

private:
    std::string LimitPath(const std::string& path)
    {
        return mBasePath + "/" + path;
    }

    IFileSystem& mFs;
    std::string mBasePath;
};

class CdIsoFileSystem : public IFileSystem
{
public:
    explicit CdIsoFileSystem(const char* fileName)
        : mRawCdImage(fileName), mCdImagePath(fileName)
    {

    }

    virtual ~CdIsoFileSystem() = default;

    virtual std::string FsPath() const override
    {
        return mCdImagePath;
    }

    virtual bool Init() override
    {
        // TODO
        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override
    {
        // Only PSX FMV's need raw sector reading, everything else is a "normal" file
        return mRawCdImage.ReadFile(fileName, string_util::ends_with(fileName, ".MOV", true));
    }

    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& /*fileName*/) override
    {
        TRACE_ENTRYEXIT;
        throw Oddlib::Exception("Create is not implemented");
    }

    virtual std::vector<std::string> EnumerateFiles(const std::string& /*directory*/, const char* /*filter*/) override
    {
        // TODO
        LOG_ERROR("Not implemented");
        abort();
    }

    virtual std::vector<std::string> EnumerateFolders(const std::string& /*directory*/) override
    {
        // TODO
        LOG_ERROR("Not implemented");
        abort();
    }

    virtual bool FileExists(const std::string& fileName) override
    {
        return mRawCdImage.FileExists(fileName) != -1;
    }

private:
    RawCdImage mRawCdImage;
    std::string mCdImagePath;
};

class DataPathIdentities
{
public:
    DataPathIdentities() = default;

    DataPathIdentities(IFileSystem& fs, const char* dataSetsIdsFileName)
    {
        auto stream = fs.Open(dataSetsIdsFileName);
        if (stream)
        {
            Parse(stream->LoadAllToString());
        }
    }

    bool IsMetaPath(const std::string& id) const
    {
        return mMetaPaths.find(id) != std::end(mMetaPaths);
    }

    std::string Identify(IFileSystem& fs) const
    {
        for (const auto& dataPathId : mDataPathIds)
        {
            if (DoMatchPathWithDataPathId(fs, dataPathId))
            {
                return dataPathId.first;
            }
        }
        return "";
    }

private:
    struct DataPathFiles
    {
        std::vector<std::string> mContainAnyOf;
        std::vector<std::string> mContainAllOf;
        std::vector<std::string> mMustNotContain;
    };

    std::map<std::string, DataPathFiles> mDataPathIds;

    // A meta path is one that exists just to include other paths
    // e.g AePsx includes AePsxCd1 and AxePsxCd1, but AePsx won't actually have a path as such
    std::set<std::string> mMetaPaths;

    bool DoMatchPathWithDataPathId(IFileSystem& fs, const std::pair<std::string, DataPathFiles>& dataPathId) const
    {
        // Check all of the "must exist files" do exist
        for (const auto& f : dataPathId.second.mContainAllOf)
        {
            if (!fs.FileExists(f.c_str()))
            {
                // Skip this dataPathId, the path doesn't match all of the "must contain"
                return false;
            }
        }

        // Check that all of the "must not exist" files don't exist
        for (const auto& f : dataPathId.second.mMustNotContain)
        {
            if (fs.FileExists(f.c_str()))
            {
                // Found a file that shouldn't exist, skip to the next dataPathId
                return false;
            }
        }

        // Check we can find at least one of the "any of" set of files
        bool foundAnyOf = false;
        for (const auto& f : dataPathId.second.mContainAnyOf)
        {
            if (fs.FileExists(f.c_str()))
            {
                foundAnyOf = true;
                break;
            }
        }

        // If one of the "any of" files exists or we have one or more "must exist" files
        // then this dataPathId matches path
        if (foundAnyOf || dataPathId.second.mContainAllOf.empty() == false)
        {
            return true;
        }

        return false;
    }

    void Parse(const std::string& json)
    {
        rapidjson::Document document;
        document.Parse(json.c_str());

        if (document.HasMember("data_set_ids"))
        {
            const auto& dataSetIds = document["data_set_ids"].GetObject();
            for (auto it = dataSetIds.MemberBegin(); it != dataSetIds.MemberEnd(); ++it)
            {
                const auto& dataSetId = it->value.GetObject();

                DataPathFiles dpFiles;
                JsonDeserializer::ReadStringArray("contains_any", dataSetId, dpFiles.mContainAnyOf);
                JsonDeserializer::ReadStringArray("contains_all", dataSetId, dpFiles.mContainAllOf);
                JsonDeserializer::ReadStringArray("not_contains", dataSetId, dpFiles.mMustNotContain);

                if (dpFiles.mContainAnyOf.empty() && dpFiles.mContainAllOf.empty() && dpFiles.mMustNotContain.empty())
                {
                    mMetaPaths.insert(it->name.GetString());
                }
                else
                {
                    mDataPathIds[it->name.GetString()] = dpFiles;
                }
            }
        }
    }
};

struct PriorityDataSet
{
public:
    PriorityDataSet(std::string dataSetName, const class GameDefinition* sourceGameDefinition)
        : mDataSetName(dataSetName), mSourceGameDefinition(sourceGameDefinition)
    {

    }

    bool operator == (const PriorityDataSet& other) const
    {
        return (mDataSetName == other.mDataSetName && mSourceGameDefinition == other.mSourceGameDefinition);
    }

    std::string mDataSetName;
    std::string mDataSetPath;
    const GameDefinition* mSourceGameDefinition;
};

using DataSetMap = std::vector<PriorityDataSet>;

class DataPaths
{
public:
    DataPaths(const DataPaths&) = delete;
    DataPaths& operator = (const DataPaths&) = delete;

    DataPaths(DataPaths&& other)
        : mGameFs(other.mGameFs)
    {
        *this = std::move(other);
    }

    DataPaths& operator = (DataPaths&& other)
    {
        mPathsFileName = std::move(other.mPathsFileName);
        mIds = std::move(other.mIds);
        mPaths = std::move(other.mPaths);
        return *this;
    }

    DataPaths(IFileSystem& fs, const char* idsFileName, const char* pathsFileName)
        : mGameFs(fs), mPathsFileName(pathsFileName), mIds(fs, idsFileName)
    {
        TRACE_ENTRYEXIT;
        if (mGameFs.FileExists(mPathsFileName))
        {
            auto stream = mGameFs.Open(mPathsFileName);
            std::vector<std::string> paths = Parse(stream->LoadAllToString());
            for (const auto& path : paths)
            {
                Add(path);
            }
        }
    }

    void Persist()
    {
        rapidjson::Document jsonDoc;
        jsonDoc.SetObject();
        rapidjson::Value pathsArray(rapidjson::kArrayType);
        rapidjson::Document::AllocatorType& allocator = jsonDoc.GetAllocator();

        for (const auto& path : mPaths)
        {
            rapidjson::Value str;
            str.SetString(path.second.c_str(), allocator);
            pathsArray.PushBack(str, allocator);
        }

        jsonDoc.AddMember("paths", pathsArray, allocator);
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        jsonDoc.Accept(writer);

        const char* jsonString = strbuf.GetString();
        auto stream = mGameFs.Create(mPathsFileName);
        stream->Write(std::string(jsonString));
    }

    bool Add(const std::string& path, const std::string& expectedId = "")
    {
        std::string pathCopy = path;
        std::string id = Identify(pathCopy); // May change pathCopy to parent dir
        if (!id.empty())
        {
            auto it = mPaths.find(id);
            if (it == std::end(mPaths))
            {
               
                if (expectedId.empty() == false && id != expectedId)
                {
                    LOG_ERROR("Path " << pathCopy << " identified as " << id << " but we are looking for " << expectedId);
                    return false;
                }
                else
                {
                    LOG_INFO("Path " << pathCopy << " identified as " << id);
                    mPaths[id] = pathCopy;
                    return true;
                }
            }
            else
            {
                LOG_INFO("Path " << pathCopy
                    << " identified as " << id
                    << " but ignoring because we already have the following path "
                    << it->second
                    << " for " << id);
                return false;
            }
        }
        return false;
    }

    const std::string& PathFor(const std::string& id)
    {
        auto it = mPaths.find(id);
        if (it == std::end(mPaths))
        {
            return mNotFoundResult;
        }
        else
        {
            return it->second;
        }
    }

    std::vector<std::string> MissingDataSetPaths(const std::vector<std::string>& requiredSets)
    {
        std::vector<std::string> ret;
        for (const auto& dataset : requiredSets)
        {
            if (mIds.IsMetaPath(dataset))
            {
                continue;
            }

            if (!PathFor(dataset).empty())
            {
                continue;
            }

            ret.emplace_back(dataset);
        }
        return ret;
    }

    bool SetActiveDataPaths(IFileSystem& fs, const DataSetMap& paths);

    struct FileSystemInfo
    {
        FileSystemInfo(const std::string& name, bool isMod, std::unique_ptr<IFileSystem> fs)
            : mDataSetName(name), mIsMod(isMod), mFileSystem(std::move(fs))
        {

        }

        FileSystemInfo(FileSystemInfo&& rhs)
        {
            *this = std::move(rhs);
        }

        FileSystemInfo& operator = (FileSystemInfo&& rhs)
        {
            mDataSetName = std::move(rhs.mDataSetName);
            mIsMod = rhs.mIsMod;
            mFileSystem = std::move(rhs.mFileSystem);
            return *this;
        }

        std::string mDataSetName;
        bool mIsMod;
        std::unique_ptr<IFileSystem> mFileSystem;
    };
    const std::vector<FileSystemInfo>& ActiveDataPaths() const { return mActiveDataPaths; }
    IFileSystem& GameFs() const { return mGameFs; }
private:

    std::string Identify(std::string& path) const
    {
        std::string id;
        auto dataSetFs = IFileSystem::Factory(mGameFs, path);
        if (dataSetFs)
        {
            id = mIds.Identify(*dataSetFs);
        }
        if (id.empty() || !dataSetFs)
        {
            LOG_WARNING("Path " << path << " could not be identified, trying parent directory..");
            path = IFileSystem::Parent(path);
            dataSetFs = IFileSystem::Factory(mGameFs, path);
            if (dataSetFs)
            {
                id = mIds.Identify(*dataSetFs);
            }

            if (id.empty())
            {
                LOG_ERROR("Path " << path << " could not be identified");
            }
        }
        return id;
    }

    std::vector<FileSystemInfo> mActiveDataPaths;

    std::map<std::string, std::string> mPaths;

    std::vector<std::string> Parse(const std::string& json)
    {
        std::vector<std::string> paths;

        rapidjson::Document document;
        document.Parse(json.c_str());

        JsonDeserializer::ReadStringArray("paths", document, paths);

        return paths;
    }

    // The "owner" file system
    IFileSystem& mGameFs;

    std::string mPathsFileName;

    // To match to what a game def wants (AePcCd1, AoDemoPsx etc)
    // we use SLUS codes for PSX or if it contains ABEWIN.EXE etc then its AoPc.
    DataPathIdentities mIds;
    const /*static*/ std::string mNotFoundResult;
};


struct BuiltInAndModGameDefs
{
    std::vector< const PriorityDataSet* > gameDefs;
    std::vector< const PriorityDataSet* > modDefs;
};

class GameDefinition
{
private:
    static bool Exists(const std::string& dataSetName, const DataSetMap& dataSets)
    {
        for (const PriorityDataSet& dataSet : dataSets)
        {
            if (dataSet.mDataSetName == dataSetName)
            {
                return true;
            }
        }
        return false;
    }

    static const GameDefinition* Find(const std::string& dataSetName, const std::vector<const GameDefinition*>& gds)
    {
        for (const GameDefinition* gd : gds)
        {
            if (gd->DataSetName() == dataSetName)
            {
                return gd;
            }
        }
        return nullptr;
    }

    static void GetDependencies(int& priority, DataSetMap& requiredDataSets, std::set<std::string>& missingDataSets, const GameDefinition* gd, const std::vector<const GameDefinition*>& gds)
    {
        requiredDataSets.emplace_back(gd->DataSetName(), gd);
        for (const std::string& dataSetName : gd->RequiredDataSets())
        {
            // Skip if already processed to avoid inf recursion on cycles
            if (!Exists(dataSetName, requiredDataSets))
            {
                // Find the game def that contains the dataset that the current game def requires
                const GameDefinition* foundGameDef = Find(dataSetName, gds);
                if (foundGameDef)
                {
                    // Recursively check the this game defs dependencies too
                    GetDependencies(priority, requiredDataSets, missingDataSets, foundGameDef, gds);
                }
                else
                {
                    missingDataSets.insert(dataSetName);
                }
            }
        }
    }

public:
    static BuiltInAndModGameDefs SplitInToBuiltInAndMods(const DataSetMap& requiredDataSets)
    {
        BuiltInAndModGameDefs sorted;
        for (const auto& dataSetPair : requiredDataSets)
        {
            if (dataSetPair.mSourceGameDefinition->IsMod())
            {
                sorted.modDefs.push_back(&dataSetPair);
            }
            else
            {
                sorted.gameDefs.push_back(&dataSetPair);
            }
        }
        return sorted;
    }

    // DFS graph walk
    // This means if we have:
    // mod -> [ AePsx, AePc]
    // Where AePsx depends on AePsxCd1 and AePsxCd1 then the search order of resources would be:
    // mod, AePsx, AePsxCd1, AePsxCd2, AePc.
    static void GetDependencies(DataSetMap& requiredDataSets, std::set<std::string>& missingDataSets, const GameDefinition* gd, const std::vector<const GameDefinition*>& gds)
    {
        int priority = 0;
        GetDependencies(priority, requiredDataSets, missingDataSets, gd, gds);
    }

    static std::vector<const GameDefinition*> GetVisibleGameDefinitions(const std::vector<GameDefinition>& gameDefinitions)
    {
        std::vector<const GameDefinition*> ret;
        ret.reserve(gameDefinitions.size());
        for (const auto& gd : gameDefinitions)
        {
            if (!gd.Hidden())
            {
                ret.emplace_back(&gd);
            }
        }
        return ret;
    }

    GameDefinition(const GameDefinition&) = default;
    GameDefinition& operator = (const GameDefinition&) = default;

    GameDefinition(IFileSystem& fileSystem, const char* gameDefinitionFile, bool isMod)
        : mIsMod(isMod)
    {
        auto stream = fileSystem.Open(gameDefinitionFile);
        assert(stream != nullptr);
        const auto jsonData = stream->LoadAllToString();
        Parse(jsonData);
        mContainingArchive = fileSystem.FsPath();
    }

    GameDefinition(std::string name, std::string dataSetName, std::vector<std::string> requiredDataSets, bool isMod)
        : mName(name), mDataSetName(dataSetName), mRequiredDataSets(requiredDataSets), mIsMod(isMod)
    {

    }

    GameDefinition() = default;
    
    const std::string& Name() const { return mName; }
    const std::string& Description() const { return mDescription; }
    const std::string& Author() const { return mAuthor; }
    const std::string& InitialLevel() const { return mInitialLevel; }
    const std::string& DataSetName() const { return mDataSetName; }
    const std::vector<std::string> RequiredDataSets() const { return mRequiredDataSets; }
    bool Hidden() const { return mHidden; }
    bool IsMod() const { return mIsMod; }
    const std::string ContainingArchive() const { return mContainingArchive; }
private:

    void Parse(const std::string& json)
    {
        jsonxx::Object root;
        root.parse(json);
        
        mName = root.get<jsonxx::String>("Name");
        mDescription = root.get<jsonxx::String>("Description");
        mAuthor = root.get<jsonxx::String>("Author");
        if (root.has<jsonxx::String>("InitialLevel"))
        {
            mInitialLevel = root.get<jsonxx::String>("InitialLevel");
        }
        mDataSetName = root.get<jsonxx::String>("DatasetName");
        if (root.has<jsonxx::Boolean>("Hidden"))
        {
            mHidden = root.get<jsonxx::Boolean>("Hidden");
        }

        if (root.has<jsonxx::Array>("RequiredDatasets"))
        {
            const auto requiredSets = root.get<jsonxx::Array>("RequiredDatasets");
            mRequiredDataSets.reserve(requiredSets.size());
            for (size_t i = 0; i < requiredSets.size(); i++)
            {
                mRequiredDataSets.emplace_back(requiredSets.get<jsonxx::String>(static_cast<u32>(i)));
            }
        }
    }

    std::string mName;
    std::string mDescription;
    std::string mAuthor;
    std::string mInitialLevel;
    std::string mDataSetName; // Name of this data set
    bool mHidden = false;
    std::vector<std::string> mRequiredDataSets;
    bool mIsMod = false;
    std::string mContainingArchive;
};

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
    std::vector<std::tuple<const char*, const char*, bool>> DebugUi(class Renderer& renderer, struct GuiContext* gui, const char* filter);

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

    friend class FmvUi;
};

class Animation
{
public:
    // Keeps the LVL and AnimSet shared pointers in scope for as long as the Animation lives.
    // On destruction if its the last instance of the lvl/animset the lvl will be closed and removed
    // from the cache, and the animset will be deleted/freed.
    struct AnimationSetHolder
    {
    public:
        AnimationSetHolder(std::shared_ptr<Oddlib::LvlArchive> sLvlPtr, std::shared_ptr<Oddlib::AnimationSet> sAnimSetPtr, u32 animIdx)
            : mLvlPtr(sLvlPtr), mAnimSetPtr(sAnimSetPtr)
        {
            mAnim = mAnimSetPtr->AnimationAt(animIdx);
        }

        const Oddlib::Animation& Animation() const
        {
            return *mAnim;
        }

        u32 MaxW() const 
        {
            return mAnimSetPtr->MaxW();
        }

        u32 MaxH() const
        {
            return mAnimSetPtr->MaxH();
        }
    private:
        std::shared_ptr<Oddlib::LvlArchive> mLvlPtr;
        std::shared_ptr<Oddlib::AnimationSet> mAnimSetPtr;
        const Oddlib::Animation* mAnim;
    };

    Animation(const Animation&) = delete;
    Animation& operator = (const Animation&) = delete;
    Animation() = delete;

    Animation(AnimationSetHolder anim, bool isPsx, bool scaleFrameOffsets, u32 defaultBlendingMode, const std::string& sourceDataSet)
        : mAnim(anim), mIsPsx(isPsx), mScaleFrameOffsets(scaleFrameOffsets), mSourceDataSet(sourceDataSet)
    {
        switch (defaultBlendingMode)
        {
        case 0:
            mBlendingMode = BlendMode::normal();
            break;
        case 1:
            mBlendingMode = BlendMode::B100F100();
            break;
        default:
            // TODO: Other required blending modes
            mBlendingMode = BlendMode::normal();
            LOG_WARNING("Unknown blending mode: " << defaultBlendingMode);
        }
    }

    s32 FrameCounter() const
    {
        return mCounter;
    }

    bool Update()
    {
        bool ret = false;
        mCounter++;
        if (mCounter >= mFrameDelay)
        {
            ret = true;
            mFrameDelay = mAnim.Animation().Fps(); // Because mFrameDelay is 1 initially and Fps() can be > 1
            mCounter = 0;
            mFrameNum++;
            if (mFrameNum >= mAnim.Animation().NumFrames())
            {
                if (mAnim.Animation().Loop())
                {
                    mFrameNum = mAnim.Animation().LoopStartFrame();
                }
                else
                {
                    mFrameNum = mAnim.Animation().NumFrames() - 1;
                }

                // Reached the final frame, animation has completed 1 cycle
                mCompleted = true;
            }

            // Are we *on* the last frame?
            mIsLastFrame = (mFrameNum == mAnim.Animation().NumFrames() - 1);
        }
        return ret;
    }

    bool IsLastFrame() const { return mIsLastFrame; }
    bool IsComplete() const { return mCompleted; }

    // TODO: Position calculation should be refactored
    void Render(Renderer& rend, bool flipX) const
    {
        /*
        static std::string msg;
        std::stringstream s;
        s << "Render frame number: " << mFrameNum;
        if (s.str() != msg)
        {
            LOG_INFO(s.str());
        }
        msg = s.str();
        */

        const Oddlib::Animation::Frame& frame = mAnim.Animation().GetFrame(mFrameNum == -1 ? 0 : mFrameNum);

        f32 xFrameOffset = (mScaleFrameOffsets ? static_cast<f32>(frame.mOffX / kPcToPsxScaleFactor) : static_cast<f32>(frame.mOffX)) * mScale;
        const f32 yFrameOffset = static_cast<f32>(frame.mOffY) * mScale;

        const f32 xpos = static_cast<f32>(mXPos);
        const f32 ypos = static_cast<f32>(mYPos);

        if (flipX)
        {
            xFrameOffset = -xFrameOffset;
        }
        // Render sprite as textured quad
        const Color color = Color::white();
        const int textureId = rend.createTexture(GL_RGBA, frame.mFrame->w, frame.mFrame->h, GL_RGBA, GL_UNSIGNED_BYTE, frame.mFrame->pixels, true);
        rend.drawQuad(
            textureId, 
            xpos + xFrameOffset, 
            ypos + yFrameOffset, 
            static_cast<f32>(frame.mFrame->w) * (flipX ? -ScaleX() : ScaleX()), 
            static_cast<f32>(frame.mFrame->h) * mScale, 
            color, 
            mBlendingMode);
        rend.destroyTexture(textureId);

        if (Debugging().mAnimBoundingBoxes)
        {
            // Render bounding box
            rend.beginPath();
            const ::Color c{ 1.0f, 0.0f, 1.0f, 1.0f };
            rend.strokeColor(c);
            rend.resetTransform();
            const f32 width = static_cast<f32>(std::abs(frame.mTopLeft.x - frame.mBottomRight.x)) * mScale;

            const glm::vec4 rectScreen(rend.WorldToScreenRect(xpos + (static_cast<f32>(flipX ? -frame.mTopLeft.x : frame.mTopLeft.x) * mScale),
                ypos + (static_cast<f32>(frame.mTopLeft.y) * mScale),
                flipX ? -width : width,
                static_cast<f32>(std::abs(frame.mTopLeft.y - frame.mBottomRight.y)) * mScale));

            rend.rect(
                rectScreen.x,
                rectScreen.y,
                rectScreen.z,
                rectScreen.w);
            rend.stroke();
            rend.closePath();
        }

        if (Debugging().mAnimDebugStrings)
        {
            // Render frame pos and frame number
            const glm::vec2 xyposScreen(rend.WorldToScreen(glm::vec2(xpos, ypos)));
            rend.text(xyposScreen.x, xyposScreen.y,
                (mSourceDataSet
                    + " x: " + std::to_string(xpos)
                    + " y: " + std::to_string(ypos)
                    + " f: " + std::to_string(FrameNumber())
                    ).c_str());
        }
    }

    void SetFrame(u32 frame)
    {
        mCounter = 0;
        mFrameDelay = mAnim.Animation().Fps();
        mFrameNum = frame;
        mIsLastFrame = false;
        mCompleted = false;
    }

    void Restart()
    {
        mCounter = 0;
        mFrameDelay = 1;
        mFrameNum = -1;
        mIsLastFrame = false;
        mCompleted = false;
    }

    bool Collision(s32 x, s32 y) const
    {
        const Oddlib::Animation::Frame& frame = mAnim.Animation().GetFrame(FrameNumber());

        // TODO: Refactor rect calcs
        f32 xpos = mScaleFrameOffsets ? static_cast<f32>(frame.mOffX / kPcToPsxScaleFactor) : static_cast<f32>(frame.mOffX);
        f32 ypos = static_cast<f32>(frame.mOffY);

        ypos = mYPos + (ypos * mScale);
        xpos = mXPos + (xpos * mScale);

        f32 w = static_cast<f32>(frame.mFrame->w) * ScaleX();
        f32 h = static_cast<f32>(frame.mFrame->h) * mScale;


        return PointInRect(static_cast<f32>(x), static_cast<f32>(y), xpos, ypos, w, h);
    }

    void SetXPos(s32 xpos) { mXPos = xpos; }
    void SetYPos(s32 ypos) { mYPos = ypos; }
    s32 XPos() const { return mXPos; }
    s32 YPos() const { return mYPos; }
    u32 MaxW() const { return static_cast<u32>(mAnim.MaxW()*ScaleX()); }
    u32 MaxH() const { return static_cast<u32>(mAnim.MaxH()*mScale); }
    s32 FrameNumber() const { return mFrameNum; }
    u32 NumberOfFrames() const { return mAnim.Animation().NumFrames(); }
    void SetScale(f32 scale) { mScale = scale; }
private:
    bool PointInRect(f32 px, f32 py, f32 x, f32 y, f32 w, f32 h) const
    {
        if (px < x) return false;
        if (py < y) return false;
        if (px >= x + w) return false;
        if (py >= y + h) return false;
        return true;
    }

    // 640 (pc xres) / 368 (psx xres) = 1.73913043478 scale factor
    const static f32 kPcToPsxScaleFactor;

    f32 ScaleX() const
    {
        // PC sprites have a bigger width as they are higher resolution
        return mIsPsx ? (mScale) : (mScale / kPcToPsxScaleFactor);
    }


    AnimationSetHolder mAnim;
    bool mIsPsx = false;
    bool mScaleFrameOffsets = false;
    std::string mSourceDataSet;
    BlendMode mBlendingMode = BlendMode::normal();

    // The "FPS" of the animation, set to 1 first so that the first Update() brings us from frame -1 to 0
    u32 mFrameDelay = 1;

    // When >= mFrameDelay, mFrameNum is incremented
    u32 mCounter = 0;
    s32 mFrameNum = -1;
    
    s32 mXPos = 100;
    s32 mYPos = 100;
    f32 mScale = 3;

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

    std::vector<std::tuple<const char*, const char*, bool>> DebugUi(class Renderer& renderer, struct GuiContext* gui, const char* filter);
private:
    std::unique_ptr<Vab> LocateSoundBank(const char* resourceName);

    std::unique_ptr<Animation> DoLocateAnimation(const DataPaths::FileSystemInfo& fs, const char* resourceName, const ResourceMapper::AnimMapping& animMapping);

    std::unique_ptr<IMovie> DoLocateFmv(IAudioController& audioController, const char* resourceName, const DataPaths::FileSystemInfo& fs, const ResourceMapper::FmvMapping& fmvMapping);

    std::unique_ptr<Oddlib::IBits> DoLocateCamera(const char* resourceName, bool ignoreMods);

    std::shared_ptr<Oddlib::LvlArchive> OpenLvl(IFileSystem& fs, const std::string& dataSetName, const std::string& lvlName);

    ResourceCache mCache;
    ResourceMapper mResMapper;
    DataPaths mDataPaths;

    friend class FmvUi; // TODO: Temp debug ui
    friend class Level; // TODO: Temp debug ui
    friend class Sound; // TODO: Temp debug ui
};
