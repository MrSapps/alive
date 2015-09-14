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

class Fmv
{
public:
    Fmv(GameData& gameData, IAudioController& audioController, FileSystem& fs);
    ~Fmv();
    void Play();
    void Stop();
    void Update();
    void Render(struct NVGcontext* ctx);
private:
    void RenderVideoUi();
private:
    GameData& mGameData;
    IAudioController& mAudioController;
    FileSystem& mFileSystem;
    std::unique_ptr<class IMovie> mFmv;
    std::unique_ptr<class FmvUi> mFmvUi;
};
