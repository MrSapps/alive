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
#include <GL/gl3w.h>

class GameData;
class IAudioController;
class FileSystem;
class Renderer;
struct GuiContext;


class IMovie : public IAudioPlayer
{
public:
    static std::unique_ptr<IMovie> Factory(
        IAudioController& audioController,
        std::unique_ptr<Oddlib::IStream> stream,
        std::unique_ptr<SubTitleParser> subtitles,
        Uint32 startSector, Uint32 endSector);

    IMovie(IAudioController& controller, std::unique_ptr<SubTitleParser> subtitles);

    virtual ~IMovie();

    // Main thread context
    void OnRenderFrame(Renderer& rend, GuiContext &gui, int /*screenW*/, int /*screenH*/);

    // Main thread context
    bool IsEnd();
protected:
    virtual bool EndOfStream() = 0;
    virtual bool NeedBuffer() = 0;
    virtual void FillBuffers() = 0;

    // Audio thread context, from IAudioPlayer
    virtual void Play(Uint8* stream, Uint32 len) override;


    void RenderFrame(Renderer& rend, GuiContext& gui, int width, int height, const GLvoid* pixels, const char* subtitles);

protected:
    struct Frame
    {
        size_t mFrameNum;
        Uint32 mW;
        Uint32 mH;
        std::vector<Uint8> mPixels;
    };
    Frame mLast;
    size_t mFrameCounter = 0;
    size_t mConsumedAudioBytes = 0;
    std::mutex mAudioBufferMutex;
    std::deque<Uint8> mAudioBuffer;
    std::deque<Frame> mVideoBuffer;
    IAudioController& mAudioController;
    int mAudioBytesPerFrame = 1;
    std::unique_ptr<SubTitleParser> mSubTitles;
private:
    //AutoMouseCursorHide mHideMouseCursor;
};


class Fmv
{
public:
    Fmv(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    virtual ~Fmv();
    void Play(const std::string& name);
    bool IsPlaying() const;
    void Stop();
    void Update();
    virtual void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH);
protected:
    GameData& mGameData;
    IAudioController& mAudioController;
    FileSystem& mFileSystem;
    std::unique_ptr<class IMovie> mFmv;

};

class DebugFmv : public Fmv
{
public:
    DebugFmv(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    virtual ~DebugFmv();
    virtual void Render(Renderer& rend, GuiContext& gui, int screenW, int screenH) override;
private:
    void RenderVideoUi(GuiContext& gui);
    std::unique_ptr<class FmvUi> mFmvUi;
};
