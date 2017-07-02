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
#include <functional>

class GameData;
class IAudioController;
class AbstractRenderer;

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

    //AutoMouseCursorHide mHideMouseCursor;
};


class Fmv final
{
public:
    static void RegisterScriptBindings();
    Fmv(const Fmv&) = delete;
    Fmv& operator = (const Fmv&) = delete;
    Fmv(IAudioController& audioController, class ResourceLocator& resourceLocator);
    ~Fmv();
    void Play(const std::string& name);
    bool IsPlaying() const;
    void Stop();
    void Update();
    void Render(AbstractRenderer& rend);
protected:
    void DebugUi();

    ResourceLocator& mResourceLocator;
    IAudioController& mAudioController;
    std::unique_ptr<class IMovie> mFmv;
    std::string mFmvName;
    InstanceBinder<class Fmv> mScriptInstance;
};
