#pragma once

#include <string>

class FileSystem
{
public:
    FileSystem();
    ~FileSystem();
    bool Init();
    const std::string& BasePath() const
    {
        return mBasePath;
    }
private:
    void InitBasePath();
private:
    std::string mBasePath;
};
