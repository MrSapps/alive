#include "SDL.h"
#include "engine.hpp"
#include "logger.hpp"
#include "msvc_sdl_link.hpp"

// Don't use SDL main
#undef main
int main(int /*argc*/, char** /*argv*/)
{
    TRACE_ENTRYEXIT;

    Engine e;
    if (!e.Init())
    {
        return 1;
    }

    return e.Run();
}
