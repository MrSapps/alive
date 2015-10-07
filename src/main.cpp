#include "SDL.h"
#include "engine.hpp"
#include "logger.hpp"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

    // Hack to make SDL2 link
    #if _MSC_VER == 1900
    void *__iob_func()
    {
        static FILE f[] = { *stdin, *stdout, *stderr };
        return f;
    }
    #endif
}

int main(int /*argc*/, char** /*argv*/)
{
    TRACE_ENTRYEXIT;

    lua_State *L = lua_open();
    if (L)
    {
        lua_close(L);
    }

    Engine e;
    if (!e.Init())
    {
        return 1;
    }


    return e.Run();
}
