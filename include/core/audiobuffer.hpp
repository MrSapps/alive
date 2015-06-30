#pragma once

#include <SDL.h>
#include <iostream>
#include <vector>
#include <mutex>

// Hacky audio class to play data from masher.
class AudioBuffer
{
public:
    static void Open(int frameSize, int freq);
    static void ChangeAudioSpec(int frameSize, int freq);
    static void Close();

    static void AudioCallback(void *udata, Uint8 *stream, int len);

    // Sample data must be 16 bit pcm.
    static void SendSamples(char * sampleData, int size);

    // Internal use
    static unsigned __int64 volatile mPlayedSamples; // This will overflow when playing roughly 10000 years worth of video.
    static std::vector<char> mBuffer;
    static std::mutex mBufferMutex;
};

