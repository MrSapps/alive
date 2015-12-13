#pragma once

#include <memory>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

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
