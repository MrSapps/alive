#include "SDL.h"
#include "engine.hpp"
#include "logger.hpp"
#include "fmv.hpp"

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

class TestEngine : public Engine
{
public:
    virtual void InitSubSystems() override
    {
        mFmv = std::make_unique<DebugFmv>(mGameData, mAudioHandler, mFileSystem);
    }

    virtual bool Init() override
    {
        return Engine::Init();
    }

    virtual int Run() override
    {
        return Engine::Run();
    }
};

int main(int /*argc*/, char** /*argv*/)
{
    TRACE_ENTRYEXIT;

    TestEngine e;
    if (!e.Init())
    {
        return 1;
    }


    return e.Run();
}
