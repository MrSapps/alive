#pragma once

#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
#include "core/audiobuffer.hpp"
#include "bitutils.hpp"
#include "logger.hpp"
#include "gamedefinition.hpp"
#include "abstractrenderer.hpp"
#include "oddlib/sdl_raii.hpp"
#include "input.hpp"
#include "jobsystem.hpp"
#include <future>

class InputState;
class ResourceLocator;
class World;

class SquirrelVm
{
public:
    SquirrelVm(const SquirrelVm&) = delete;
    SquirrelVm& operator = (const SquirrelVm&) = delete;

    SquirrelVm(int stackSize = 1024)
    {
        mVm = sq_open(stackSize);
        
        sqstd_register_iolib(mVm);
        //sqstd_printcallstack(mVm);

        sq_setprintfunc(mVm, OnPrint, OnPrint);
        sq_newclosure(mVm, OnVmError, 0);
        sq_seterrorhandler(mVm);
        sq_setcompilererrorhandler(mVm, OnVmCompileError);


        Sqrat::DefaultVM::Set(mVm);
        Sqrat::ErrorHandling::Enable(true);
    }

    ~SquirrelVm()
    {
        TRACE_ENTRYEXIT;
        if (mVm)
        {
            sq_close(mVm);
        }
    }

    static void CheckError()
    {
        const HSQUIRRELVM vm = Sqrat::DefaultVM::Get();

        if (Sqrat::Error::Occurred(vm))
        {
            std::string err = Sqrat::Error::Message(vm);
            Sqrat::Error::Clear(vm);
            throw Oddlib::Exception(err);
        }
    }

    static void CompileAndRun(ResourceLocator& resourceLocator, const std::string& scriptName);
    static void CompileAndRun(const std::string& scriptName, const std::string& script);

    HSQUIRRELVM Handle() const { return mVm; }
private:
    static void OnPrint(HSQUIRRELVM, const SQChar* s, ...)
    {
        va_list vl;
        va_start(vl, s);
        vprintf(s, vl);
        va_end(vl);
    }

    static void OnVmCompileError(HSQUIRRELVM, const SQChar* desc, const SQChar* source, SQInteger line, SQInteger column)
    {
        LOG_ERROR(
            "Compiler error (line: " << line 
            << ", file: " << (*source != 0 ? source : "unknown")
            << ", column: " << column << "): " 
            << desc);
    }

    static SQInteger OnVmError(HSQUIRRELVM v)
    {
        if (sq_gettop(v) >= 1)
        {
            const SQChar* sErr = 0;
            std::string err;
            if (SQ_SUCCEEDED(sq_getstring(v, 2, &sErr)))
            {
                err = sErr;
            }

            SQStackInfos si = {};
            if (SQ_SUCCEEDED(sq_stackinfos(v, 1, &si)))
            {
                std::string sMsg;
                if (si.funcname)
                {
                    sMsg += std::string("(function: ") + si.funcname + ", line: " + std::to_string(si.line) + ", file: " + (*si.source != 0 ? si.source : "unknown") + "): ";
                }
                sMsg += err;
                err = sMsg;
            }

            LOG_ERROR(err);
        }
        return 0;
    }

    HSQUIRRELVM mVm = 0;
};

enum class EngineStates
{
    ePreEngineInit,
    eEngineInit,
    eRunSelectedGame,
    eSelectGame,
    eQuit
};

using SoundId = u32;

class Engine;
class World;

class EngineJob : public IJob
{
public:
    enum class eJobTypes
    {
        eInitResources
    };

    EngineJob(Engine& e, eJobTypes jobType);

    virtual void OnExecute(const CancelFlag& quitFlag) override;

    virtual void OnFinished() override;

private:
    eJobTypes mJobType;
    Engine& mEngine;
};

class LoadingIcon
{
public:
    LoadingIcon(IFileSystem& fileSystem);
    void Update(CoordinateSpace& coords);
    void Render(AbstractRenderer& renderer);
    void SetEnabled(bool enabled);
private:
    SDL_SurfacePtr mLoadingIcon;
    bool mEnabled = false;
};

class Engine final
{
public:
    Engine(const std::vector<std::string>& commandLineArguments);
    ~Engine();
    bool Init();
    int Run();
    void OnJobStart(EngineJob::eJobTypes jobType, const CancelFlag& quitFlag);
    void OnJobFinished(EngineJob::eJobTypes jobType);
private:
    void RunInitScript();
    void Include(const std::string& scriptName);
    void Update();
    void Render();
    bool InitSDL();
    void AddGameDefinitionsFrom(const char* path);
    void AddModDefinitionsFrom(const char* path);
    void AddDirectoryBasedModDefinitionsFrom(std::string path);
    void AddZipedModDefinitionsFrom(std::string path);
    void InitResources();
    void InitImGui();
    void ImGui_WindowResize();
protected:
    void BindScriptTypes();
    void InitSubSystems();
    
    // Audio must init early
    SdlAudioWrapper mAudioHandler;

    JobSystem mJobSystem;

    std::unique_ptr<class OSBaseFileSystem> mFileSystem;
 
    SDL_Window* mWindow = nullptr;

    std::unique_ptr<class ResourceLocator> mResourceLocator;
    std::unique_ptr<class AbstractRenderer> mRenderer;
    std::unique_ptr<class Sound> mSound;

    std::vector<GameDefinition> mGameDefinitions;

    InputState mInputState;

    SquirrelVm mSquirrelVm;
    TextureHandle mGuiFontHandle = {};
    bool mTryDirectX9 = false;

    EngineStates mState = EngineStates::ePreEngineInit;
    std::unique_ptr<World> mWorld;
    std::unique_ptr<class GameSelectionState> mGameSelectionScreen;

    std::unique_ptr<LoadingIcon> mLoadingIcon;
};
