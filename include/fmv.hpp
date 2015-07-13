#pragma once

#include <vector>
#include <memory>
#include "oddlib/masher.hpp"
#include "SDL.h"
#include "oddlib/PSXADPCMDecoder.h"
#include "oddlib/PSXMDECDecoder.h"

class GameData;

class Fmv
{
public:
    Fmv(GameData& gameData);
    ~Fmv();
    void Play();
    void Stop();
    void Update();
    void Render();
private:
    void RenderVideoUi();
private:
    GameData& mGameData;
    std::unique_ptr<Oddlib::Masher> video;
    std::vector<std::unique_ptr<class FmvUi>> mFmvUis;
    SDL_Surface* videoFrame = NULL;

    PSXMDECDecoder mMdec;
    PSXADPCMDecoder mAdpcm;
    FILE* mFp = nullptr;
    int frameW = 0;
    int frameH = 0;
};
