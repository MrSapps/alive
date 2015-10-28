#pragma once

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

class LuaState
{
public:
    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    LuaState()
        : mState(lua_open())
    {
    }

    ~LuaState()
    {
        if (mState)
        {
            lua_close(mState);
        }
    }

    // implicitly act as a lua_State pointer
    inline operator lua_State*()
    {
        return mState;
    }

    lua_State* State()
    {
        return mState;
    }

private:
    lua_State* mState = nullptr;
};

class Script
{
public:
    Script();
    ~Script();
    bool Init(class FileSystem& fs);
    void Update();
private:
    int mScript = -1;
    // Acts as the lua "VM"
    LuaState mState;
};
