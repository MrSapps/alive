#pragma once

#include <memory>

class Script
{
public:
    Script();
    ~Script();
    bool Init(class FileSystem& fs);
    void Update();
private:
    std::unique_ptr<class LuaScript> mScript;
};
