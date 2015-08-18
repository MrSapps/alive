#include "core/audiobuffer.hpp"
#include "oddlib/exceptions.hpp"
#include <string>

#define AUDIO_BUFFER_CHANNELS 2
#define AUDIO_BUFFER_FORMAT AUDIO_S16
//#define AUDIO_BUFFER_FORMAT_SIZE (SDL_AUDIO_BITSIZE(AUDIO_BUFFER_FORMAT) / 8)
//#define AUDIO_BUFFER_SAMPLE_SIZE (AUDIO_BUFFER_FORMAT_SIZE * AUDIO_BUFFER_CHANNELS)

class SdlAudioLocker
{
public:
    SdlAudioLocker()
    {
        SDL_LockAudio();
    }

    ~SdlAudioLocker()
    {
        SDL_UnlockAudio();
    }
};

void SdlAudioWrapper::Open(Uint16 frameSize, int freq)
{
    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        throw Oddlib::Exception((std::string("SDL_Init for SDL_INIT_AUDIO failed: ") + SDL_GetError()).c_str());
    }

    SDL_AudioSpec audioSpec = {};
    audioSpec.userdata = this;
    audioSpec.callback = StaticAudioCallback;
    audioSpec.channels = AUDIO_BUFFER_CHANNELS;
    audioSpec.freq = freq;
    audioSpec.samples = frameSize;
    audioSpec.format = AUDIO_BUFFER_FORMAT;

    // Open the audio device
    if (SDL_OpenAudio(&audioSpec, NULL) < 0)
    {
        throw Oddlib::Exception((std::string("SDL_OpenAudio failed: ") + SDL_GetError()).c_str());
    }

    // Start the call back
    SDL_PauseAudio(0);
}

void SdlAudioWrapper::AddPlayer(IAudioPlayer* player)
{
    // Protect from concurrent access in the audio call back
    SdlAudioLocker audioLocker;
    mAudioPlayers.insert(player);
}

void SdlAudioWrapper::RemovePlayer(IAudioPlayer* player)
{
    // Protect from concurrent access in the audio call back
    SdlAudioLocker audioLocker;
    mAudioPlayers.erase(player);
}

void SdlAudioWrapper::ChangeAudioSpec(Uint16 frameSize, int freq)
{
    SdlAudioLocker audioLocker;
    Close();
    mPlayedSamples = 0;
    Open(frameSize, freq);
}

SdlAudioWrapper::~SdlAudioWrapper()
{
    // Ensure call back won't be called with invalid this ptr
    Close();
}

void SdlAudioWrapper::Close()
{
    SdlAudioLocker audioLocker;
    SDL_CloseAudio();
}

// Called in the context of the audio thread
void SdlAudioWrapper::StaticAudioCallback(void *udata, Uint8 *stream, int len)
{
    if (udata)
    {
        reinterpret_cast<SdlAudioWrapper*>(udata)->AudioCallback(stream, len);
    }
}

// Called in the context of the audio thread
void SdlAudioWrapper::AudioCallback(Uint8 *stream, int len)
{
    memset(stream, 0, len);
    for (auto& player : mAudioPlayers)
    {
        player->Play(stream, len);
    }
}
