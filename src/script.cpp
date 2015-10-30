#include "script.hpp"
#include "filesystem.hpp"
#include "logger.hpp"

static void report_errors(lua_State *L, const int status)
{
    if (status != 0)
    {
        LOG_ERROR( "-- " << lua_tostring(L, -1));
        lua_pop(L, 1); // remove error message
    }
}

static int execute_program(lua_State *L)
{
    return lua_pcall(L, 0, LUA_MULTRET, 0);
}

extern "C"
{

    static int l_my_print(lua_State* L)
    {
        int nargs = lua_gettop(L);
        std::cout << "in my_print:";
        for (int i = 1; i <= nargs; ++i)
        {
            std::cout << lua_tostring(L, i);
        }
        std::cout << std::endl;
        return 0;
    }

    static const struct luaL_Reg printlib[] =
    {
        { "print", l_my_print },
        { NULL, NULL }
    };

    extern int luaopen_luamylib(lua_State *L)
    {
        lua_getglobal(L, "_G");
        luaL_register(L, NULL, printlib);
        lua_pop(L, 1);
        return 0;
    }

}



Script::Script()
{

}

Script::~Script()
{

}

bool Script::Init(FileSystem& fs)
{
    if (!mState.State())
    {
        LOG_ERROR("Failed to create lua state");
        return false;
    }

    luaopen_base(mState);
    luaopen_luamylib(mState);

    const std::string scriptFileName = fs.GameData().BasePath() + "data/scripts/main.lua";
    mScript = luaL_loadfile(mState, scriptFileName.c_str());
    if (mScript == LUA_ERRSYNTAX)
    {
        LOG_ERROR("Script syntax error");
    }
    else if (mScript == LUA_ERRMEM)
    {
        LOG_ERROR("Out of memory");
    }



    return true;
}

void Script::Update()
{
    if (mScript == 0)
    {
        int ret = execute_program(mState);
        LOG_INFO("ret is " << ret);
        report_errors(mState, mScript);
    }
}
