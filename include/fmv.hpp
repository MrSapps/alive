#pragma once

#include <vector>
#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
#include "oddlib/PSXADPCMDecoder.h"
#include "oddlib/PSXMDECDecoder.h"

class GameData;
class IAudioController;
class FileSystem;
class Renderer;
struct GuiContext;

class IMovie;

/*
static std::unique_ptr<IMovie> FmvFactory(
    IAudioController& audioController,
    std::unique_ptr<Oddlib::IStream> stream);
    */

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
