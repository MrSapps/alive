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
    if (isFile)
    {
        if (string_util::ends_with(path, ".bin", true))
        {
            return std::make_unique<CdIsoFileSystem>(path.c_str());
        }
        else if (string_util::ends_with(path, ".zip", true))
        {
            // TODO
            LOG_ERROR("ZIP FS TODO");
            return std::make_unique<ZipFileSystem>(path.c_str(), fs);
        }
        else
        {
            LOG_ERROR("Unknown archive type for: " << path);
            return nullptr;
        }
    }
    else
    {
        return std::make_unique<DirectoryLimitedFileSystem>(fs, path);
    }
}
