#pragma once

#include "filesystem.hpp"
#include "data_set_type.hpp"
#include "oddlib/lvlarchive.hpp"

class IDataInspector
{
public:
    void HandleDataSet(IFileSystem& parentFs, eDataSetType eType, const std::string& resourcePath, const std::vector<std::string>& lvls)
    {
        auto fs = IFileSystem::Factory(parentFs, resourcePath);
        if (!fs)
        {
            throw std::runtime_error("FS init failed");
        }

        for (const std::string& lvl : lvls)
        {
            std::unique_ptr<Oddlib::LvlArchive> archive = std::make_unique<Oddlib::LvlArchive>(fs->Open(lvl));
            HandleLvl(eType, lvl, std::move(archive));
        }
    }

    std::vector<const Oddlib::LvlArchive::File*> GetFilesOfType(const Oddlib::LvlArchive& archive, const std::string& extension)
    {
        std::vector<const Oddlib::LvlArchive::File*> ret;
        for (u32 i = 0; i < archive.FileCount(); i++)
        {
            const Oddlib::LvlArchive::File* file = archive.FileByIndex(i);
            const auto parts = string_util::split(file->FileName(), '.');
            if (parts.size() == 2 && string_util::iequals(parts[1], extension))
            {
                ret.push_back(file);
            }
        }
        return ret;
    }

    virtual ~IDataInspector() = default;
    virtual void HandleLvl(eDataSetType eType, const std::string& lvlName, std::unique_ptr<Oddlib::LvlArchive> lvl) = 0;
    virtual void OnFinished() = 0;
};
