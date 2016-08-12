#include "core/audiobuffer.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include <string>
#include <sstream>
#include "stk/include/Stk.h"

#define AUDIO_BUFFER_CHANNELS 2
//#define AUDIO_BUFFER_FORMAT AUDIO_S16
#define AUDIO_BUFFER_FORMAT AUDIO_F32 // HACK: float breaks FMV sound

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

SdlAudioWrapper::SdlAudioWrapper()
{
    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        throw Oddlib::Exception((std::string("SDL_Init for SDL_INIT_AUDIO failed: ") + SDL_GetError()).c_str());
    }
}

void SdlAudioWrapper::Open(u16 frameSize, int freq)
{
    SDL_AudioSpec audioSpec = {};
    audioSpec.userdata = this;
    audioSpec.callback = StaticAudioCallback;
    audioSpec.channels = AUDIO_BUFFER_CHANNELS;
    audioSpec.freq = freq;
    audioSpec.samples = frameSize;
    audioSpec.format = AUDIO_BUFFER_FORMAT;

    SDL_AudioSpec obtained = {};

    // Open the audio device
    mDevice = SDL_OpenAudioDevice(NULL, 0, &audioSpec, &obtained, 0);
    if (mDevice == 0)
    {
        throw Oddlib::Exception((std::string("SDL_OpenAudio failed: ") + SDL_GetError()).c_str());
    }


    if (obtained.samples > audioSpec.samples)
    {
        LOG_WARNING("Got " << obtained.samples << " samples but wanted " << audioSpec.samples << " samples trying again with half the number of samples");
        Close();

        audioSpec.samples = audioSpec.samples / 2;
        mDevice = SDL_OpenAudioDevice(NULL, 0, &audioSpec, &obtained, 0);
        if (mDevice == 0)
        {
            throw Oddlib::Exception((std::string("SDL_OpenAudio failed: ") + SDL_GetError()).c_str());
        }

        if (obtained.samples > audioSpec.samples)
        {
            std::stringstream s;
            s << "Got " << obtained.samples << " samples but wanted " << audioSpec.samples;
            throw Oddlib::Exception(s.str().c_str());
        }
    }

    stk::Stk::setSampleRate(freq);

    // Start the call back
    SDL_PauseAudioDevice(mDevice, 0);
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

void SdlAudioWrapper::SetAudioSpec(u16 frameSize, int freq)
{
    LOG_INFO("SetAudioSpec samples: " << frameSize << " freq " << freq);
    SdlAudioLocker audioLocker;
    Close();
    Open(frameSize, freq);
}

SdlAudioWrapper::~SdlAudioWrapper()
{
    SdlAudioLocker audioLocker;

    mAudioPlayers.clear();

    // Ensure call back won't be called with invalid this ptr
    Close();

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SdlAudioWrapper::Close()
{
    SdlAudioLocker audioLocker;
    if (mDevice != 0)
    {
        SDL_CloseAudioDevice(mDevice);
    }
}

// Called in the context of the audio thread
void SdlAudioWrapper::StaticAudioCallback(void *udata, u8 *stream, int len)
{
    if (udata)
    {
        reinterpret_cast<SdlAudioWrapper*>(udata)->AudioCallback(stream, len);
    }
}

// Called in the context of the audio thread
void SdlAudioWrapper::AudioCallback(u8 *stream, int len)
{
    memset(stream, 0, len);
    for (auto& player : mAudioPlayers)
    {
        player->Play(stream, len);
    }
}
