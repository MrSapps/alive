#pragma once

#include <set>
#include <atomic>
#include <SDL.h>

class IAudioPlayer;

class IAudioController
{
public:
    virtual ~IAudioController() = default;
    virtual void Open(Uint16 frameSize, int freq) = 0;
    virtual void AddPlayer(IAudioPlayer* player) = 0;
    virtual void RemovePlayer(IAudioPlayer* player) = 0;
    virtual void ChangeAudioSpec(Uint16 frameSize, int freq) = 0;
};

class IAudioPlayer
{
public:
    virtual ~IAudioPlayer() = default;
    virtual void Play(Uint8* stream, Sint32 len) = 0;
};

class SdlAudioWrapper : public IAudioController
{
public:
    virtual void Open(Uint16 frameSize, int freq) override;
    virtual void AddPlayer(IAudioPlayer* player) override;
    virtual void RemovePlayer(IAudioPlayer* player) override;
    virtual void ChangeAudioSpec(Uint16 frameSize, int freq) override;
    ~SdlAudioWrapper();
private:
    void Close();
    static void StaticAudioCallback(void *udata, Uint8 *stream, int len);
    void AudioCallback(Uint8 *stream, int len);
private:
    // This will overflow when playing roughly 10000 years worth of video.
    std::atomic<Uint64> mPlayedSamples; 
    std::set<IAudioPlayer*> mAudioPlayers;
};
