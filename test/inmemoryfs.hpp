#pragma once

#include "resourcemapper.hpp"

// Pseudo in memory file system
class InMemoryFileSystem : public IFileSystem
{
private:
    struct File
    {
        std::string mName;
        std::vector<u8> mData;
    };

    struct Directory
    {
        std::string mName;
        std::vector<Directory> mChildren;
        std::vector<File> mFiles;
    };

public:
    virtual ~InMemoryFileSystem() = default;

    void AddFile(std::string strPath, const std::string& content = "")
    {
        AddFile(strPath, StringToVector(content));
    }

    void AddFile(std::string strPath, const std::vector<u8>& content)
    {
        DirectoryAndFileName path(strPath);
        Directory* dir = FindPath(path.mDir, true);

        // Don't allow duplicated file names
        File* file = FindFile(*dir, path.mFile);
        if (file)
        {
            // Update existing
            file->mData = content;
            return;
        }
        dir->mFiles.emplace_back(File{ path.mFile, content });
    }

    // IFileSystem
    virtual std::string FsPath() const override
    {
        return "FakeFs";
    }

    virtual bool Init() override
    {
        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const std::string& fileName) override
    {
        DirectoryAndFileName path(fileName);
        Directory* dir = FindPath(path.mDir, true);
        if (!dir)
        {
            return nullptr;
        }

        File* file = FindFile(*dir, path.mFile);
        if (!file)
        {
            return nullptr;
        }


        return std::make_unique<Oddlib::MemoryStream>(std::vector<u8>(file->mData));
    }

    virtual std::unique_ptr<Oddlib::IStream> Create(const std::string& /*fileName*/) override
    {
        TRACE_ENTRYEXIT;
        throw Oddlib::Exception("Create is not implemented");
    }

    virtual std::vector<std::string> EnumerateFiles(const std::string& directory, const char* filter) override
    {
        Directory* dir = FindPath(directory, true);
        std::vector<std::string> ret;
        if (!dir)
        {
            return ret;
        }

        for (const File& file : dir->mFiles)
        {
            if (WildCardMatcher(file.mName, filter, IgnoreCase))
            {
                ret.emplace_back(file.mName);
            }
        }

        return ret;
    }

    virtual std::vector<std::string> EnumerateFolders(const std::string& directory) override
    {
        Directory* dir = FindPath(directory, true);
        std::vector<std::string> ret;
        if (!dir)
        {
            return ret;
        }

        for (const Directory& subDir : dir->mChildren)
        {
            ret.emplace_back(subDir.mName);
        }

        return ret;
    }

    virtual bool FileExists(std::string& fileName) override
    {
        DirectoryAndFileName path(fileName);
        Directory* dir = FindPath(path.mDir, true);
        if (!dir)
        {
            return false;
        }

        for (const File& file : dir->mFiles)
        {
            if (file.mName == path.mFile)
            {
                return true;
            }
        }
        return false;
    }

    // Make public so it can be tested
    static bool WildCardMatcher(const std::string& text, std::string wildcardPattern, EMatchType caseSensitive)
    {
        return IFileSystem::WildCardMatcher(text, wildcardPattern, caseSensitive);
    }

    static void NormalizePath(std::string& path)
    {
        IFileSystem::NormalizePath(path);
    }

private:

    File* FindFile(Directory& dir, const std::string& fileName)
    {
        for (File& file : dir.mFiles)
        {
            if (file.mName == fileName)
            {
                return &file;
            }
        }
        return nullptr;
    }

    Directory* FindPath(Directory& currentDir, std::deque<std::string>& paths, bool insert)
    {
        std::string dir = paths.front();
        paths.pop_front();

        for (Directory& subDir : currentDir.mChildren)
        {
            if (subDir.mName == dir)
            {
                if (paths.empty())
                {
                    return &subDir;
                }
                else
                {
                    return FindPath(subDir, paths, insert);
                }
            }
        }

        if (insert)
        {
            currentDir.mChildren.push_back(Directory{ dir, {}, {} });
            if (paths.empty())
            {
                return &currentDir.mChildren.back();
            }
            else
            {
                return FindPath(currentDir.mChildren.back(), paths, insert);
            }
        }
        else
        {
            return nullptr;
        }
    }

    Directory* FindPath(std::string path, bool insert)
    {
        string_util::replace_all(path, "\\", "/");
        auto dirs = string_util::split(path, '/');
        return FindPath(mRoot, dirs, insert);
    }

    Directory mRoot;
};
