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

class InputReader;
class ResourceLocator;
class World;

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

    InputReader mInputState;

    TextureHandle mGuiFontHandle = {};
    bool mTryDirectX9 = false;

    EngineStates mState = EngineStates::ePreEngineInit;
    std::unique_ptr<World> mWorld;
    std::unique_ptr<class GameSelectionState> mGameSelectionScreen;

    std::unique_ptr<LoadingIcon> mLoadingIcon;
};
