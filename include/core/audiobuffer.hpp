#pragma once

#include <set>
#include <atomic>
#include <SDL.h>
#include "types.hpp"

class IAudioPlayer;

class IAudioController
{
public:
    virtual ~IAudioController() = default;
    virtual void AddPlayer(IAudioPlayer* player) = 0;
    virtual void RemovePlayer(IAudioPlayer* player) = 0;
    virtual void SetAudioSpec(u16 frameSize, int freq) = 0;
};

class IAudioPlayer
{
public:
    virtual ~IAudioPlayer() = default;
    virtual void Play(u8* stream, u32 len) = 0;
};

class SdlAudioWrapper : public IAudioController
{
public:
    SdlAudioWrapper();
    virtual void AddPlayer(IAudioPlayer* player) override;
    virtual void RemovePlayer(IAudioPlayer* player) override;
    virtual void SetAudioSpec(u16 frameSize, s32 freq) override;
    ~SdlAudioWrapper();
private:
    void Open(u16 frameSize, int freq);
    void Close();
    static void StaticAudioCallback(void *udata, u8 *stream, int len);
    void AudioCallback(u8 *stream, int len);
private:
    void OpenImpl(const char* deviceName, u16 frameSize, int freq);

    std::set<IAudioPlayer*> mAudioPlayers;
    int mDevice = 0;
    u16 mFrameSize = 0;
    s32 mFreq = 0;
};
