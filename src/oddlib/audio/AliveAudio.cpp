#include "oddlib/audio/AliveAudio.h"
#include "filesystem.hpp"

static float RandFloat(float a, float b)
{
    return ((b - a)*((float)rand() / RAND_MAX)) + a;
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

    auto soundsDatSteam = fs.ResourcePaths().Open("sounds.dat");
    if (!soundsDatSteam)
    {
        throw Oddlib::Exception("sounds.dat not found");
    }

    m_SoundsDat = std::vector<unsigned char>(soundsDatSteam->Size() + 1); // Plus one, just in case interpolating tries to go that one byte further!

    soundsDatSteam->ReadBytes(m_SoundsDat.data(), soundsDatSteam->Size());

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

void AliveAudio::AliveRenderAudio(float * AudioStream, int StreamLength)
{
    static float tick = 0;
    static int note = 0;

    //AliveAudioSoundbank * currentSoundbank = AliveAudio::m_CurrentSoundbank;

    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);

    // Reset buffers
    for (int i = 0; i < StreamLength; ++i)
    {
        m_DryChannelBuffer[i] = 0;
        m_ReverbChannelBuffer[i] = 0;
    }

    const int voiceCount = m_Voices.size();
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
        for (int v = 0; v < voiceCount; v++)
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

            float centerPan = voice->m_Tone->f_Pan;
            float leftPan = 1.0f;
            float rightPan = 1.0f;

            if (centerPan > 0)
            {
                leftPan = 1.0f - abs(centerPan);
            }

            if (centerPan < 0)
            {
                rightPan = 1.0f - abs(centerPan);
            }

            // TODO FIX ME
            float  s = voice->GetSample(Interpolation, AntiAliasFilteringEnabled);
            float leftSample = (s * leftPan);
            float rightSample = (s * rightPan);

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
        const float left = static_cast<float>(m_Reverb.tick(m_ReverbChannelBuffer[i], m_ReverbChannelBuffer[i + 1], 0));
        const float right = static_cast<float>(m_Reverb.lastOut(1));
        m_ReverbChannelBuffer[i] = left;
        m_ReverbChannelBuffer[i + 1] = right;
    }
   
    for (int i = 0; i < StreamLength; i += 2)
    {
        const float left = m_DryChannelBuffer[i] + m_ReverbChannelBuffer[i];
        const float right = m_DryChannelBuffer[i + 1] + m_ReverbChannelBuffer[i + 1];
        SDL_MixAudioFormat((Uint8 *)(AudioStream + i), (const Uint8*)&left, AUDIO_F32, sizeof(float), SDL_MIX_MAXVOLUME);
        SDL_MixAudioFormat((Uint8 *)(AudioStream + i + 1), (const Uint8*)&right, AUDIO_F32, sizeof(float), SDL_MIX_MAXVOLUME);
    }

    CleanVoices();
}


void AliveAudio::Play(Uint8* stream, Uint32 len)
{
    if (m_DryChannelBuffer.size() != len)
    {
        // Maybe it's ok to have some crackles when the buffer size changes.
        // (This allocates memory, which you should never do in audio thread.)
        m_DryChannelBuffer.resize(len);
        m_ReverbChannelBuffer.resize(len);
    }


    AliveRenderAudio(reinterpret_cast<float*>(stream), len / sizeof(float));
}

void AliveAudio::PlayOneShot(int program, int note, float volume, float pitch)
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
        jsonxx::Object sndObj = soundList.get<jsonxx::Object>(i);
        if (sndObj.get<jsonxx::String>("id") == soundID)
        {
            float randA = 0;
            float randB = 0;

            if (sndObj.has<jsonxx::Array>("pitchrand"))
            {
                randA = (float)sndObj.get<jsonxx::Array>("pitchrand").get<jsonxx::Number>(0);
                randB = (float)sndObj.get<jsonxx::Array>("pitchrand").get<jsonxx::Number>(1);
            }

            PlayOneShot((int)sndObj.get<jsonxx::Number>("prog"), (int)sndObj.get<jsonxx::Number>("note"), 1.0f, RandFloat(randA, randB));
        }
    }
}

void AliveAudio::NoteOn(int program, int note, char velocity, float /*pitch*/, int trackID, double trackDelay)
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

void AliveAudio::NoteOn(int program, int note, char velocity, int trackID, double trackDelay)
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

void AliveAudio::NoteOffDelay(int program, int note, int trackID, float trackDelay)
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

void AliveAudio::LoadSoundbank(char * fileName)
{
    ClearAllVoices(true);

    if (m_CurrentSoundbank != nullptr)
    {
        delete m_CurrentSoundbank;
    }

    AliveAudioSoundbank * soundBank = new AliveAudioSoundbank(fileName, *this);
    m_CurrentSoundbank = soundBank;
}

void AliveAudio::SetSoundbank(AliveAudioSoundbank * soundbank)
{
    ClearAllVoices(true);

    if (m_CurrentSoundbank != nullptr)
    {
        delete m_CurrentSoundbank;
    }

    m_CurrentSoundbank = soundbank;
}

void AliveAudio::LoadAllFromLvl(std::string lvlPath, std::string vabID, std::string seqFile, FileSystem& fs)
{
    auto stream = fs.ResourcePaths().Open(lvlPath);
    if (stream)
    {
        Oddlib::LvlArchive archive(std::move(stream));
        LoadAllFromLvl(archive, vabID, seqFile);
    }
}

void AliveAudio::LoadAllFromLvl(Oddlib::LvlArchive& archive, std::string vabID, std::string seqFile)
{
    m_LoadedSeqData.clear();
    SetSoundbank(new AliveAudioSoundbank(archive, vabID, *this));
    for (size_t i = 0; i < archive.FileByName(seqFile)->ChunkCount(); i++)
    {
        m_LoadedSeqData.push_back(archive.FileByName(seqFile)->ChunkByIndex(i)->ReadData());
    }
}
