#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace Oddlib
{
    class IStream;
}

class IFileSystem
{
public:
    IFileSystem() = default;
    IFileSystem(IFileSystem&&) = delete;
    IFileSystem(const IFileSystem&) = delete;
    IFileSystem& operator = (const IFileSystem&) = delete;
    IFileSystem& operator = (IFileSystem&&) = delete;

    virtual ~IFileSystem() = default;

    virtual bool Init() { return true; }

    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& fileName) = 0;
    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) = 0;

    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) = 0;
    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) = 0;

    virtual bool FileExists(std::string& fileName) = 0;
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

    static void NormalizePath(std::string& path);
    static bool WildCardMatcher(const std::string& text, std::string wildcardPattern, EMatchType caseSensitive);
    static void EscapeRegex(std::string& regex);
};


class OSBaseFileSystem : public IFileSystem
{
private:
    static bool IsDots(const std::string& name)
    {
        return name == "." || name == "..";
    }

public:
    OSBaseFileSystem() = default;
    OSBaseFileSystem(OSBaseFileSystem&&) = delete;
    OSBaseFileSystem& operator = (OSBaseFileSystem&&) = delete;

    virtual bool Init() override
    {
        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override;
    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& fileName) override;

    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) override
    {
        std::unique_lock<std::recursive_mutex> lock(mMutex);
        return DoEnumerate(directory, true, filter);
    }

    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) override
    {
        std::unique_lock<std::recursive_mutex> lock(mMutex);
        return DoEnumerate(directory, false, nullptr);
    }

    bool FileExists(std::string& fileName) override;

    virtual std::string ExpandPath(const std::string& path) = 0;

    void DeleteFile(const std::string& path);
    void RenameFile(const std::string& source, const std::string& destination);
private:
    std::vector<std::string> DoEnumerate(const std::string& directory, bool files, const char* filter);
protected:
    mutable std::recursive_mutex mMutex;
};

