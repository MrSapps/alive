#include "resourcemapper.hpp"
#include "zipfilesystem.hpp"

/*static*/ std::unique_ptr<IFileSystem> IFileSystem::Factory(IFileSystem& fs, const std::string& path)
{
    TRACE_ENTRYEXIT;

    if (path.empty())
    {
        return nullptr;
    }

    const bool isFile = fs.FileExists(path.c_str());
    std::unique_ptr<IFileSystem> ret;
    if (isFile)
    {
        if (string_util::ends_with(path, ".bin", true))
        {
            LOG_INFO("Creating ISO FS for " << path);
            ret = std::make_unique<CdIsoFileSystem>(path.c_str());
        }
        else if (string_util::ends_with(path, ".zip", true))
        {
            LOG_INFO("Creating ZIP FS for " << path);
            ret = std::make_unique<ZipFileSystem>(path.c_str(), fs);
        }
        else
        {
            LOG_ERROR("Unknown archive type for: " << path);
            return nullptr;
        }
    }
    else
    {
        LOG_INFO("Creating dir view FS for " << path);
        ret = std::make_unique<DirectoryLimitedFileSystem>(fs, path);
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
