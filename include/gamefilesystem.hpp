#pragma once

#include "filesystem.hpp"
#include <string>
#include <map>

class GameFileSystem : public OSBaseFileSystem
{
public:
    GameFileSystem() = default;
    GameFileSystem(GameFileSystem&&) = delete;
    GameFileSystem& operator = (GameFileSystem&&) = delete;

    virtual bool Init() override final;
    virtual std::string FsPath() const override;
    std::string InitUserPath();
    std::string InitBasePath();
    virtual std::string ExpandPath(const std::string& path) override final;
private:
    std::map<std::string, std::string> mNamedPaths;
};
