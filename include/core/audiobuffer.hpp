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
    virtual u16 AudioFrameSize() const = 0;
    virtual u32 SampleRate() const = 0;
};

class IAudioPlayer
{
public:
    virtual ~IAudioPlayer() = default;
    virtual bool Play(f32* stream, u32 len) = 0;
};

class SdlAudioWrapper : public IAudioController
{
public:
    SdlAudioWrapper(u16 frameSize, u32 freq);
    virtual void AddPlayer(IAudioPlayer* player) override;
    virtual void RemovePlayer(IAudioPlayer* player) override;
    virtual u16 AudioFrameSize() const override;
    virtual u32 SampleRate() const override;
    ~SdlAudioWrapper();
private:
    void SetAudioSpec(u16 frameSize, s32 freq);
    void Open(u16 frameSize, int freq);
    void Close();
    static void StaticAudioCallback(void *udata, u8 *stream, s32 len);
    void AudioCallback(u8 *stream, int len);
private:
    void OpenImpl(const char* deviceName, u16 frameSize, u32 freq);

    std::set<IAudioPlayer*> mAudioPlayers;
    int mDevice = 0;
    u16 mFrameSize = 0;
    s32 mFreq = 0;
};
