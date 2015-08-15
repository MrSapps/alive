#include "core/audiobuffer.hpp"


int gAudioBufferSize = 512;
#define AUDIO_BUFFER_CHANNELS 2
#define AUDIO_BUFFER_FORMAT AUDIO_S16
#define AUDIO_BUFFER_FORMAT_SIZE (SDL_AUDIO_BITSIZE(AUDIO_BUFFER_FORMAT) / 8)
#define AUDIO_BUFFER_SAMPLE_SIZE (AUDIO_BUFFER_FORMAT_SIZE * AUDIO_BUFFER_CHANNELS)

std::atomic<Uint64> AudioBuffer::mPlayedSamples;
std::vector<char> AudioBuffer::mBuffer;

void AudioBuffer::AudioCallback(void *udata, Uint8 *stream, int len)
{
    memset(stream, 0, len);


    int currentBufferSampleSize = mBuffer.size() / AUDIO_BUFFER_SAMPLE_SIZE;
    if (!mBuffer.empty())
//    if (currentBufferSampleSize >= gAudioBufferSize)
    {
        int size = mBuffer.size();// *AUDIO_BUFFER_SAMPLE_SIZE;
        if (size > len)
        {
            size = len;
        }
        memmove(stream, mBuffer.data(), size);
        mBuffer.erase(mBuffer.begin(), mBuffer.begin() + size);
        mPlayedSamples += size / AUDIO_BUFFER_SAMPLE_SIZE;
    }
    else
    {
        std::cout << "Nothing to play" << std::endl;
    }

}

void AudioBuffer::ChangeAudioSpec(int frameSize, int freq)
{
    SDL_LockAudio();

    Close();
    mPlayedSamples = 0;
    Open(frameSize, freq);
    SDL_UnlockAudio();
}

void AudioBuffer::Open(int frameSize, int freq)
{
    gAudioBufferSize = frameSize;

    SDL_Init(SDL_INIT_AUDIO);

    SDL_AudioSpec audioSpec;
    audioSpec.callback = AudioCallback;
    audioSpec.channels = AUDIO_BUFFER_CHANNELS;
    audioSpec.freq = freq;
    audioSpec.samples = gAudioBufferSize;
    audioSpec.format = AUDIO_BUFFER_FORMAT;

    /* Open the audio device */
    if (SDL_OpenAudio(&audioSpec, NULL) < 0)
{
        fprintf(stderr, "Failed to initialize audio: %s\n", SDL_GetError());
        exit(-1);
    }

    SDL_PauseAudio(0);
}

void AudioBuffer::Close()
{
    SDL_LockAudio();

    mBuffer.clear();
    SDL_CloseAudio();
    SDL_UnlockAudio();

}

void AudioBuffer::SendSamples(char * sampleData, int size)
{
    SDL_LockAudio();


    int lastSize = mBuffer.size();
    mBuffer.resize(lastSize + size);
    memcpy(mBuffer.data() + lastSize, sampleData, size);


    SDL_UnlockAudio();
}
