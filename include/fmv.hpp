#pragma once

#include <vector>
#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
#include "oddlib/PSXADPCMDecoder.h"
#include "oddlib/PSXMDECDecoder.h"

class GameData;
class IAudioController;

class Fmv
{
public:
    Fmv(GameData& gameData, IAudioController& audioController);
    ~Fmv();
    void Play();
    void Stop();
    void Update();
    void Render();
private:
    void RenderVideoUi();
private:
    GameData& mGameData;
    IAudioController& mAudioController;
    std::unique_ptr<class IMovie> mFmv;
    std::vector<std::unique_ptr<class FmvUi>> mFmvUis;
};
