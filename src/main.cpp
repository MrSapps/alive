#include "SDL.h"
#include "engine.hpp"

// Don't use SDL main
#undef main
int main(int argc, char** argv)
{
    TRACE_ENTRYEXIT;

    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
    {
        args.push_back(argv[i]);
    }

    try
    {
        Engine e(args);

        if (!e.Init())
        {
            return 1;
        }
        return e.Run();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Caught unhandled exception: " << e.what());
        return 1;
    }
}
