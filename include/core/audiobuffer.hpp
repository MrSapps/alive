#pragma once

#include <set>
#include <atomic>
#include <SDL.h>

class IAudioPlayer;

class IAudioController
{
public:
    virtual ~IAudioController() = default;
    virtual void AddPlayer(IAudioPlayer* player) = 0;
    virtual void RemovePlayer(IAudioPlayer* player) = 0;
    virtual void SetAudioSpec(Uint16 frameSize, int freq) = 0;
};

class IAudioPlayer
{
public:
    virtual ~IAudioPlayer() = default;
    virtual void Play(Uint8* stream, Uint32 len) = 0;
};

class SdlAudioWrapper : public IAudioController
{
public:
    SdlAudioWrapper();
    virtual void AddPlayer(IAudioPlayer* player) override;
    virtual void RemovePlayer(IAudioPlayer* player) override;
    virtual void SetAudioSpec(Uint16 frameSize, int freq) override;
    ~SdlAudioWrapper();
private:
    void Open(Uint16 frameSize, int freq);
    void Close();
    static void StaticAudioCallback(void *udata, Uint8 *stream, int len);
    void AudioCallback(Uint8 *stream, int len);
private:
    std::set<IAudioPlayer*> mAudioPlayers;
    int mDevice = 0;
};
