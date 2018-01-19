#pragma once

#include <vector>
#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
#include "oddlib/PSXADPCMDecoder.h"
#include "oddlib/PSXMDECDecoder.h"
#include "core/audiobuffer.hpp"
#include "subtitles.hpp"
#include "stdthread.h"
#include "resourcemapper.hpp"
#include <functional>

class GameData;
class IAudioController;
class AbstractRenderer;

class AutoMouseCursorHide
{
public:
    AutoMouseCursorHide()
    {
        SDL_ShowCursor(0);
    }

    ~AutoMouseCursorHide()
    {
        SDL_ShowCursor(1);
    }
};

class IMovie : public IAudioPlayer
{
public:
    static std::unique_ptr<IMovie> Factory(
        const std::string& resourceName,
        IAudioController& audioController,
        std::unique_ptr<Oddlib::IStream> stream,
        std::unique_ptr<SubTitleParser> subtitles,
        u32 startSector, u32 endSector);

    IMovie(const std::string& resourceName, IAudioController& controller, std::unique_ptr<SubTitleParser> subtitles);

    virtual ~IMovie();

    // Main thread context
    void OnRenderFrame(AbstractRenderer& rend);

    // Main thread context
    bool IsEnd();

    void Start();
    void Stop();
protected:
    virtual bool EndOfStream() = 0;
    virtual bool NeedBuffer() = 0;
    virtual void FillBuffers() = 0;

    // Audio thread context, from IAudioPlayer
    virtual bool Play(f32* stream, u32 len) override;


    void RenderFrame(AbstractRenderer& rend, int width, int height, const void* pixels, const char* subtitles);

protected:
    struct Frame
    {
        size_t mFrameNum;
        u32 mW;
        u32 mH;
        std::vector<u8> mPixels;
    };
    size_t mFrameCounter = 0;
    size_t mConsumedAudioBytes = 0;
    std::mutex mAudioBufferMutex;
    std::deque<u8> mAudioBuffer;
    std::deque<Frame> mVideoBuffer;
    IAudioController& mAudioController;
    u32 mAudioBytesPerFrame = 1;
    std::unique_ptr<SubTitleParser> mSubTitles;
    std::string mName;

private:
    bool mPlaying = false;
    AutoMouseCursorHide mHideMouseCursor;
};


class Fmv final
{
public:
    Fmv(const Fmv&) = delete;
    Fmv& operator = (const Fmv&) = delete;
    Fmv(IAudioController& audioController, class ResourceLocator& resourceLocator);
    ~Fmv();
    void Play(const std::string& name, const ResourceMapper::FmvFileLocation* fmvFileLocation = nullptr);
    bool IsPlaying() const;
    void Stop();
    void Update();
    void RenderDebugSubsAndFontAtlas(AbstractRenderer& rend);
    void Render(AbstractRenderer& rend);
protected:
    ResourceLocator& mResourceLocator;
    IAudioController& mAudioController;
    std::unique_ptr<class IMovie> mFmv;
    std::string mFmvName;
};

class PlayFmvState
{
public:
    PlayFmvState(IAudioController& audioController, ResourceLocator& locator);
    void Render(AbstractRenderer& renderer);
    void RenderDebugSubsAndFontAtlas(AbstractRenderer& renderer);
    bool Update(const InputState& input);
private:
    std::unique_ptr<Fmv> mFmv;
public:
    void Play(const char* fmvName, const ResourceMapper::FmvFileLocation* fmvFileLocation = nullptr);
};

class FmvDebugUi
{
public:
    FmvDebugUi(ResourceLocator& resourceLocator);
    bool Ui();
    const std::string& FmvName() const { return mFmvName; }
    const ResourceMapper::FmvFileLocation* FmvFileLocation() const { return mDebugMapping; }
private:
    // To play an FMV from a specific location for debugging
    const ResourceMapper::FmvFileLocation* mDebugMapping = nullptr;
    std::string mFmvName;
    ResourceLocator& mResourceLocator;
};