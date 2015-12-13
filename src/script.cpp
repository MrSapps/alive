#include "script.hpp"
#include "filesystem.hpp"
#include "logger.hpp"

class LuaScript
{
public:
    LuaScript()
    {
        InitLua();
    }

    ~LuaScript()
    {
        HaltLua();
    }

    // call a lua function.
    // you must specify the quantity of of params and returns.
    int CallLua(std::string func, int num_params, int num_returns)
    {
        // on error, execute lua function debug.traceback
        // debug is a table, put it on the stack
        lua_getglobal(mLuaState, "debug");

        // -1 is the top of the stack
        lua_getfield(mLuaState, -1, "traceback");

        // traceback is on top, remove debug from 2nd spot
        lua_remove(mLuaState, -2);

        mErrorHandlerStackIndex = lua_gettop(mLuaState);
        
        // get the lua function and execute it
        lua_getglobal(mLuaState, func.c_str());
        const int ret = lua_pcall(mLuaState, num_params, num_returns, mErrorHandlerStackIndex);
        if (ret)
        {
            LOG_ERROR("Lua call failed (" << ErrorToString(ret) << ": " << lua_tostring(mLuaState, -1));
        }

        // remove the error handler from the stack
        lua_pop(mLuaState, 1);

        return ret;
    }

    bool FunctionExists(std::string func)
    {
        // try to put the function on top of the stack
        lua_getglobal(mLuaState, func.c_str());

        // check that value on top of stack is not nil
        bool ret = !lua_isnil(mLuaState, -1);

        // get rid of the value we put on the stack
        lua_pop(mLuaState, 1);

        return ret;
    }

    lua_State* State()
    {
        return mLuaState;
    }

private:
    void InitLua()
    {
        mLuaState = luaL_newstate();
        luaL_openlibs(mLuaState);

        set_redirected_print();
    }

    void HaltLua()
    {
        lua_close(mLuaState);
    }

    static std::string LuaTypeToString(int index)
    {
        switch (index)
        {
        case LUA_TNIL:            return "nil";
        case LUA_TNUMBER:         return "number";
        case LUA_TBOOLEAN:        return "boolean";
        case LUA_TSTRING:         return "string";
        case LUA_TTABLE:          return "table";
        case LUA_TFUNCTION:       return "function";
        case LUA_TUSERDATA:       return "userdata";
        case LUA_TTHREAD:         return "thread";
        case LUA_TLIGHTUSERDATA:  return "light userdata";
        default:                  return "unknown type";
        }
    }

    static std::string ErrorToString(int resultcode)
    {
        switch (resultcode)
        {
        case 0:             return "Success";
        case LUA_ERRRUN:    return "Runtime error";
        case LUA_ERRSYNTAX: return "Syntax error";
        case LUA_ERRERR:    return "Error with error alert mechanism.";
        case LUA_ERRFILE:   return "Couldn't open or read file";
        default:            return "Unknown error: " + std::to_string(resultcode);
        }
    }

    static int script_log(lua_State* L)
    {
        const int nargs = lua_gettop(L);
        for (int i = 1; i <= nargs; ++i)
        {
            if (lua_isstring(L, i))
            {
                LOG_INFO(lua_tostring(L, i));
            }
            else
            {
                LOG_ERROR("TODO: Print this type");
            }
        }
        return 0;
    }

    int set_redirected_print()
    {
        lua_getglobal(mLuaState, "_G");
        lua_register(mLuaState, "print", script_log);
        lua_pop(mLuaState, 1); // global table
        return 0;
    }

private:
    lua_State* mLuaState = nullptr;
    int mErrorHandlerStackIndex = 0;
};


Script::Script()
{

}

Script::~Script()
{

}

bool Script::Init(FileSystem& fs)
{
    const std::string myfile = fs.GameData().BasePath() + "data/scripts/main.lua";


    mScript = std::make_unique<LuaScript>();

    luaL_dofile(mScript->State(), myfile.c_str());

    return true;
}

void Script::Update()
{
    const std::string myfn = "Update";
    const int res = mScript->CallLua(myfn, 0, 0);
    if (res)
    {
        
    }
}
