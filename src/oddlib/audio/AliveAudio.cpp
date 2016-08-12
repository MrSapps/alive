#include "oddlib/audio/AliveAudio.h"
#include "filesystem.hpp"

static f32 RandFloat(f32 a, f32 b)
{
    return ((b - a)*((f32)rand() / RAND_MAX)) + a;
}

void AliveAudio::LoadJsonConfig(std::string filePath, FileSystem& fs)
{
    std::string jsonData = fs.GameData().Open(filePath)->LoadAllToString();
    jsonxx::Object obj;
    obj.parse(jsonData);

    m_Config.import(obj);
}

void AliveAudio::AliveInitAudio(FileSystem& fs)
{
    /*
    SDL_Init(SDL_INIT_AUDIO);
    // UGLY FIX ALL OF THIS
    // |
    // V
    SDL_AudioSpec waveSpec;
    waveSpec.callback = AliveAudioSDLCallback;
    waveSpec.userdata = nullptr;
    waveSpec.channels = 2;
    waveSpec.freq = AliveAudioSampleRate;
    waveSpec.samples = 512;
    waveSpec.format = AUDIO_F32;

    // Open the audio device
    if (SDL_OpenAudio(&waveSpec, NULL) < 0)
    {
        fprintf(stderr, "Failed to initialize audio: %s\n", SDL_GetError());
        exit(-1);
    }
    */

    LoadJsonConfig("data/themes.json", fs);
    LoadJsonConfig("data/sfx_list.json", fs);

    /*
    auto soundsDatSteam = fs.ResourcePaths().Open("sounds.dat");
    if (!soundsDatSteam)
    {
        throw Oddlib::Exception("sounds.dat not found");
    }

    m_SoundsDat = std::vector<unsigned char>(soundsDatSteam->Size()); // Plus one, just in case interpolating tries to go that one byte further!
    
    soundsDatSteam->Read(m_SoundsDat);
    */

    SDL_PauseAudio(0);
}

void AliveAudio::CleanVoices()
{
    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);

    std::vector<AliveAudioVoice *> deadVoices;

    for (auto& voice : m_Voices)
    {
        if (voice->b_Dead)
        {
            deadVoices.push_back(voice);
        }
    }

    for (auto& obj : deadVoices)
    {
        delete obj;

        m_Voices.erase(std::remove(m_Voices.begin(), m_Voices.end(), obj), m_Voices.end());
    }

}

void AliveAudio::AliveRenderAudio(f32 * AudioStream, int StreamLength)
{
    //static f32 tick = 0;
    //static int note = 0;

    //AliveAudioSoundbank * currentSoundbank = AliveAudio::m_CurrentSoundbank;

    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);

    // Reset buffers
    for (int i = 0; i < StreamLength; ++i)
    {
        m_DryChannelBuffer[i] = 0;
        m_ReverbChannelBuffer[i] = 0;
    }

    const size_t voiceCount = m_Voices.size();
    /*
    if (voiceCount == 0)
    {
        // Bail out early if nothing to render
        currentSampleIndex += StreamLength;
        return;
    }
    */

    AliveAudioVoice ** rawPointer = m_Voices.data(); // Real nice speed boost here.

    for (int i = 0; i < StreamLength; i += 2)
    {
        for (size_t v = 0; v < voiceCount; v++)
        {
            AliveAudioVoice * voice = rawPointer[v]; // Raw pointer skips all that vector bottleneck crap

            voice->f_TrackDelay--;

            if (voice->m_UsesNoteOffDelay)
            {
                voice->f_NoteOffDelay--;
            }

            if (voice->m_UsesNoteOffDelay && voice->f_NoteOffDelay <= 0 && voice->b_NoteOn == true)
            {
                voice->b_NoteOn = false;
                //printf("off");
            }

            if (voice->b_Dead || voice->f_TrackDelay > 0)
            {
                continue;
            }

            f32 centerPan = voice->m_Tone->f_Pan;
            f32 leftPan = 1.0f;
            f32 rightPan = 1.0f;

            if (centerPan > 0)
            {
                leftPan = 1.0f - std::abs(centerPan);
            }

            if (centerPan < 0)
            {
                rightPan = 1.0f - std::abs(centerPan);
            }

            // TODO FIX ME
            f32  s = voice->GetSample(Interpolation, AntiAliasFilteringEnabled);
            f32 leftSample = (s * leftPan);
            f32 rightSample = (s * rightPan);

            if (voice->m_Tone->Reverbate || ForceReverb)
            {
                m_ReverbChannelBuffer[i] += leftSample;
                m_ReverbChannelBuffer[i + 1] += rightSample;
            }
            else
            {
                m_DryChannelBuffer[i] += leftSample;
                m_DryChannelBuffer[i + 1] += rightSample;
            }
        }

        currentSampleIndex++;
    }

    m_Reverb.setEffectMix(ReverbMix);

    // TODO: Find a better way of feeding the data in
    for (int i = 0; i < StreamLength; i += 2)
    {
        const f32 left = static_cast<f32>(m_Reverb.tick(m_ReverbChannelBuffer[i], m_ReverbChannelBuffer[i + 1], 0));
        const f32 right = static_cast<f32>(m_Reverb.lastOut(1));
        m_ReverbChannelBuffer[i] = left;
        m_ReverbChannelBuffer[i + 1] = right;
    }
   
    for (int i = 0; i < StreamLength; i += 2)
    {
        const f32 left = m_DryChannelBuffer[i] + m_ReverbChannelBuffer[i];
        const f32 right = m_DryChannelBuffer[i + 1] + m_ReverbChannelBuffer[i + 1];
        SDL_MixAudioFormat((u8 *)(AudioStream + i), (const u8*)&left, AUDIO_F32, sizeof(f32), SDL_MIX_MAXVOLUME);
        SDL_MixAudioFormat((u8 *)(AudioStream + i + 1), (const u8*)&right, AUDIO_F32, sizeof(f32), SDL_MIX_MAXVOLUME);
    }

    CleanVoices();
}


void AliveAudio::Play(u8* stream, u32 len)
{
    if (m_DryChannelBuffer.size() != len)
    {
        // Maybe it's ok to have some crackles when the buffer size changes.
        // (This allocates memory, which you should never do in audio thread.)
        m_DryChannelBuffer.resize(len);
        m_ReverbChannelBuffer.resize(len);
    }


    AliveRenderAudio(reinterpret_cast<f32*>(stream), len / sizeof(f32));
}

void AliveAudio::PlayOneShot(int program, int note, f32 volume, f32 pitch)
{
    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);
    for (auto& tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
    {
        if (note >= tone->Min && note <= tone->Max)
        {
            AliveAudioVoice * voice = new AliveAudioVoice();
            voice->i_Note = note;
            voice->f_Velocity = volume;
            voice->m_Tone = tone.get();
            voice->f_Pitch = pitch;
            voice->m_DebugDisableResampling = DebugDisableVoiceResampling;
            m_Voices.push_back(voice);
        }
    }
}

void AliveAudio::PlayOneShot(std::string soundID)
{
    jsonxx::Array soundList = m_Config.get<jsonxx::Array>("sounds");

    for (size_t i = 0; i < soundList.size(); i++)
    {
        jsonxx::Object sndObj = soundList.get<jsonxx::Object>(static_cast<unsigned int>(i));
        if (sndObj.get<jsonxx::String>("id") == soundID)
        {
            f32 randA = 0;
            f32 randB = 0;

            if (sndObj.has<jsonxx::Array>("pitchrand"))
            {
                randA = (f32)sndObj.get<jsonxx::Array>("pitchrand").get<jsonxx::Number>(0);
                randB = (f32)sndObj.get<jsonxx::Array>("pitchrand").get<jsonxx::Number>(1);
            }

            PlayOneShot((int)sndObj.get<jsonxx::Number>("prog"), (int)sndObj.get<jsonxx::Number>("note"), 1.0f, RandFloat(randA, randB));
        }
    }
}

void AliveAudio::NoteOn(int program, int note, char velocity, f32 /*pitch*/, int trackID, f64 trackDelay)
{
    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);
    for (auto& tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
    {
        if (note >= tone->Min && note <= tone->Max)
        {
            AliveAudioVoice * voice = new AliveAudioVoice();
            voice->i_Note = note;
            voice->m_Tone = tone.get();
            voice->i_Program = program;
            voice->f_Velocity = velocity / 127.0f;
            voice->i_TrackID = trackID;
            voice->f_TrackDelay = trackDelay;
            voice->m_DebugDisableResampling = DebugDisableVoiceResampling;
            m_Voices.push_back(voice);
        }
    }
}

void AliveAudio::NoteOn(int program, int note, char velocity, int trackID, f64 trackDelay)
{
    NoteOn(program, note, velocity, 0, trackID, trackDelay);
}

void AliveAudio::NoteOff(int program, int note, int trackID)
{
    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);
    for (auto& voice : m_Voices)
    {
        if (voice->i_Note == note && voice->i_Program == program && voice->i_TrackID == trackID)
        {
            voice->b_NoteOn = false;
        }
    }
}

void AliveAudio::NoteOffDelay(int program, int note, int trackID, f32 trackDelay)
{
    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);
    for (auto& voice : m_Voices)
    {
        if (voice->i_Note == note && voice->i_Program == program && voice->i_TrackID == trackID && voice->f_TrackDelay < trackDelay && voice->f_NoteOffDelay <= 0)
        {
            voice->m_UsesNoteOffDelay = true;
            voice->f_NoteOffDelay = trackDelay;
        }
    }
}

void AliveAudio::DebugPlayFirstToneSample(int program, int tone)
{
    PlayOneShot(program, (m_CurrentSoundbank->m_Programs[program]->m_Tones[tone]->Min + m_CurrentSoundbank->m_Programs[program]->m_Tones[tone]->Max) / 2, 1);
}

void AliveAudio::ClearAllVoices(bool forceKill)
{
    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);

    std::vector<AliveAudioVoice *> deadVoices;

    for (auto& voice : m_Voices)
    {
        if (forceKill)
        {
            deadVoices.push_back(voice);
        }
        else
        {
            voice->b_NoteOn = false; // Send a note off to all of the notes though.
            if (voice->f_SampleOffset == 0) // Let the voices that are CURRENTLY playing play.
            {
                deadVoices.push_back(voice);
            }
        }
    }

    for (auto& obj : deadVoices)
    {
        delete obj;

        m_Voices.erase(std::remove(m_Voices.begin(), m_Voices.end(), obj), m_Voices.end());
    }

}

void AliveAudio::ClearAllTrackVoices(int trackID, bool forceKill)
{
    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);
    std::vector<AliveAudioVoice *> deadVoices;

    for (auto& voice : m_Voices)
    {
        if (forceKill)
        {
            if (voice->i_TrackID == trackID) // Kill the voices no matter what. Cuts of any sounds = Ugly sound
            {
                deadVoices.push_back(voice);
            }
        }
        else
        {
            voice->b_NoteOn = false; // Send a note off to all of the notes though.
            if (voice->i_TrackID == trackID && voice->f_SampleOffset == 0) // Let the voices that are CURRENTLY playing play.
            {
                deadVoices.push_back(voice);
            }
        }
    }

    for (auto& obj : deadVoices)
    {
        delete obj;

        m_Voices.erase(std::remove(m_Voices.begin(), m_Voices.end(), obj), m_Voices.end());
    }

}

void AliveAudio::SetSoundbank(std::unique_ptr<AliveAudioSoundbank> soundbank)
{
    ClearAllVoices(true);
    m_CurrentSoundbank = std::move(soundbank);
}

void AliveAudio::LoadAllFromLvl(Oddlib::LvlArchive& archive, std::string vabID, std::string seqFile)
{
    m_LoadedSeqData.clear();
    SetSoundbank(std::make_unique<AliveAudioSoundbank>(archive, vabID, *this));
    auto file = archive.FileByName(seqFile);
    if (file)
    {
        for (size_t i = 0; i < file->ChunkCount(); i++)
        {
            m_LoadedSeqData.push_back(file->ChunkByIndex(static_cast<u32>(i))->ReadData());
        }
    }
}
