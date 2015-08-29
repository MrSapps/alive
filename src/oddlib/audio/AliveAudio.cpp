#include "oddlib/audio/AliveAudio.h"
#include "filesystem.hpp"

biquad * AliveAudio::AliveAudioEQBiQuad = nullptr;
std::vector<unsigned char> AliveAudio::m_SoundsDat;
AliveAudioSoundbank * AliveAudio::m_CurrentSoundbank = nullptr;
jsonxx::Object AliveAudio::m_Config;
std::vector<AliveAudioVoice *> AliveAudio::m_Voices;
std::mutex AliveAudio::voiceListMutex;
std::mutex AliveAudio::EQMutex;
std::vector<std::vector<Uint8>> AliveAudio::m_LoadedSeqData;
bool AliveAudio::Interpolation = true;
bool AliveAudio::EQEnabled = true;
bool AliveAudio::voiceListLocked = false;
long long AliveAudio::currentSampleIndex = 0;

static void LoadJsonConfig(std::string filePath, FileSystem& fs)
{
    std::string jsonData = fs.Open(filePath)->LoadAllToString();
	jsonxx::Object obj;
	obj.parse(jsonData);

	AliveAudio::m_Config.import(obj);
}

void AliveInitAudio(FileSystem& fs)
{
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

	/* Open the audio device */
	if (SDL_OpenAudio(&waveSpec, NULL) < 0)
    {
		fprintf(stderr, "Failed to initialize audio: %s\n", SDL_GetError());
		exit(-1);
	}

	LoadJsonConfig("data/themes.json", fs);
	LoadJsonConfig("data/sfx_list.json", fs);

    auto soundsDatSteam = fs.OpenResource("sounds.dat");
    AliveAudio::m_SoundsDat = std::vector<unsigned char>(soundsDatSteam->Size() + 1); // Plus one, just in case interpolating tries to go that one byte further!

    soundsDatSteam->ReadBytes(AliveAudio::m_SoundsDat.data(), soundsDatSteam->Size());

	SDL_PauseAudio(0);
}

void CleanVoices()
{
	AliveAudio::voiceListMutex.lock();
	std::vector<AliveAudioVoice *> deadVoices;

	for (auto voice : AliveAudio::m_Voices)
	{
		if (voice->b_Dead)
		{
			deadVoices.push_back(voice);
		}
	}

	for (auto obj : deadVoices)
	{
		delete obj;

		AliveAudio::m_Voices.erase(std::remove(AliveAudio::m_Voices.begin(), AliveAudio::m_Voices.end(), obj), AliveAudio::m_Voices.end());
	}
	AliveAudio::voiceListMutex.unlock();
}

void AliveRenderAudio(float * AudioStream, int StreamLength)
{
	static float tick = 0;
	static int note = 0;

	AliveAudioSoundbank * currentSoundbank = AliveAudio::m_CurrentSoundbank;

	AliveAudio::voiceListMutex.lock();
	int voiceCount = AliveAudio::m_Voices.size();
	AliveAudioVoice ** rawPointer = AliveAudio::m_Voices.data(); // Real nice speed boost here.

	for (int i = 0; i < StreamLength; i += 2)
	{
		for (int v = 0; v < voiceCount; v++)
		{
			AliveAudioVoice * voice = rawPointer[v]; // Raw pointer skips all that vector bottleneck crap

			voice->f_TrackDelay--;

			if (voice->m_UsesNoteOffDelay)
				voice->f_NoteOffDelay--;

			if (voice->m_UsesNoteOffDelay && voice->f_NoteOffDelay <= 0 && voice->b_NoteOn == true)
			{
				voice->b_NoteOn = false;
				//printf("off");
			}

			if (voice->b_Dead || voice->f_TrackDelay > 0)
				continue;

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

			float s = voice->GetSample();

			float leftSample = s * leftPan;
			float rightSample = s * rightPan;

			SDL_MixAudioFormat((Uint8 *)(AudioStream + i), (const Uint8*)&leftSample, AUDIO_F32, sizeof(float), 37); // Left Channel
			SDL_MixAudioFormat((Uint8 *)(AudioStream + i + 1), (const Uint8*)&rightSample, AUDIO_F32, sizeof(float), 37); // Right Channel
		}

		AliveAudio::currentSampleIndex++;
	}
	AliveAudio::voiceListMutex.unlock();


	CleanVoices();
}

typedef struct _StereoSample
{
	float L;
	float R;
} StereoSample;

void AliveAudioSDLCallback(void *udata, Uint8 *stream, int len)
{
	memset(stream, 0, len);
	AliveRenderAudio((float *)stream, len / sizeof(float));

	if (AliveAudio::EQEnabled)
		AliveEQEffect((float *)stream, len / sizeof(float));
}

void AliveAudio::PlayOneShot(int program, int note, float volume, float pitch)
{
	voiceListMutex.lock();
	for (auto tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
	{
		if (note >= tone->Min && note <= tone->Max)
		{
			AliveAudioVoice * voice = new AliveAudioVoice();
			voice->i_Note = note;
			voice->f_Velocity = volume;
			voice->m_Tone = tone;
			voice->f_Pitch = pitch;
			m_Voices.push_back(voice);
		}
	}
	voiceListMutex.unlock();
}

void AliveAudio::PlayOneShot(std::string soundID)
{
	jsonxx::Array soundList = AliveAudio::m_Config.get<jsonxx::Array>("sounds");

	for (int i = 0; i < soundList.size(); i++)
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

			PlayOneShot((int)sndObj.get<jsonxx::Number>("prog"), (int)sndObj.get<jsonxx::Number>("note"), 1.0f, AliveAudioHelper::RandFloat(randA, randB));
		}
	}
}

void AliveAudio::NoteOn(int program, int note, char velocity, float pitch, int trackID, float trackDelay)
{
	if (!voiceListLocked)
		voiceListMutex.lock();
	for (auto tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
	{
		if (note >= tone->Min && note <= tone->Max)
		{
			AliveAudioVoice * voice = new AliveAudioVoice();
			voice->i_Note = note;
			voice->m_Tone = tone;
			voice->i_Program = program;
			voice->f_Velocity = velocity / 127.0f;
			voice->i_TrackID = trackID;
			voice->f_TrackDelay = trackDelay;
			m_Voices.push_back(voice);
		}
	}
	if (!voiceListLocked)
		voiceListMutex.unlock();
}

void AliveAudio::NoteOn(int program, int note, char velocity, int trackID, float trackDelay)
{
	NoteOn(program, note, velocity, 0, trackID, trackDelay);
}

void AliveAudio::NoteOff(int program, int note, int trackID)
{
	if (!voiceListLocked)
		voiceListMutex.lock();
	for (auto voice : m_Voices)
	{
		if (voice->i_Note == note && voice->i_Program == program && voice->i_TrackID == trackID)
		{
			voice->b_NoteOn = false;
		}
	}
	if (!voiceListLocked)
		voiceListMutex.unlock();
}

void AliveAudio::NoteOffDelay(int program, int note, int trackID, float trackDelay)
{
	if (!voiceListLocked)
		voiceListMutex.lock();
	for (auto voice : m_Voices)
	{
		if (voice->i_Note == note && voice->i_Program == program && voice->i_TrackID == trackID && voice->f_TrackDelay < trackDelay && voice->f_NoteOffDelay <= 0)
		{
			voice->m_UsesNoteOffDelay = true;
			voice->f_NoteOffDelay = trackDelay;
		}
	}
	if (!voiceListLocked)
		voiceListMutex.unlock();
}

void AliveAudio::DebugPlayFirstToneSample(int program, int tone)
{
	PlayOneShot(program, (m_CurrentSoundbank->m_Programs[program]->m_Tones[tone]->Min + m_CurrentSoundbank->m_Programs[program]->m_Tones[tone]->Max) / 2, 1);
}

void AliveAudio::LockNotes()
{
	if (!voiceListLocked)
	{
		voiceListLocked = true;
		voiceListMutex.lock();
	}
	//else
	//throw "Voice list locked. Can't lock again.";
}

void AliveAudio::UnlockNotes()
{
	if (voiceListLocked)
	{
		voiceListLocked = false;
		voiceListMutex.unlock();
	}
	//else
	//throw "Voice list unlocked. Can't unlock again.";
}

void AliveAudio::ClearAllVoices(bool forceKill)
{
	LockNotes();

	std::vector<AliveAudioVoice *> deadVoices;

	for (auto voice : AliveAudio::m_Voices)
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

	for (auto obj : deadVoices)
	{
		delete obj;

		AliveAudio::m_Voices.erase(std::remove(AliveAudio::m_Voices.begin(), AliveAudio::m_Voices.end(), obj), AliveAudio::m_Voices.end());
	}

	UnlockNotes();
}

void AliveAudio::ClearAllTrackVoices(int trackID, bool forceKill)
{
	LockNotes();

	std::vector<AliveAudioVoice *> deadVoices;

	for (auto voice : AliveAudio::m_Voices)
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

	for (auto obj : deadVoices)
	{
		delete obj;

		AliveAudio::m_Voices.erase(std::remove(AliveAudio::m_Voices.begin(), AliveAudio::m_Voices.end(), obj), AliveAudio::m_Voices.end());
	}

	UnlockNotes();
}

void AliveAudio::LoadSoundbank(char * fileName)
{
	ClearAllVoices(true);

	if (AliveAudio::m_CurrentSoundbank != nullptr)
		delete AliveAudio::m_CurrentSoundbank;

	AliveAudioSoundbank * soundBank = new AliveAudioSoundbank(fileName);
	AliveAudio::m_CurrentSoundbank = soundBank;
}

void AliveAudio::SetSoundbank(AliveAudioSoundbank * soundbank)
{
	ClearAllVoices(true);

	if (AliveAudio::m_CurrentSoundbank != nullptr)
		delete AliveAudio::m_CurrentSoundbank;

	AliveAudio::m_CurrentSoundbank = soundbank;
}

void AliveAudio::LoadAllFromLvl(std::string lvlPath, std::string vabID, std::string seqFile, FileSystem& fs)
{
    Oddlib::LvlArchive archive(fs.OpenResource(lvlPath));
	LoadAllFromLvl(archive, vabID, seqFile);
}

void AliveAudio::LoadAllFromLvl(Oddlib::LvlArchive& archive, std::string vabID, std::string seqFile)
{
	m_LoadedSeqData.clear();
	SetSoundbank(new AliveAudioSoundbank(archive, vabID));
	for (int i = 0; i < archive.FileByName(seqFile)->ChunkCount(); i++)
	{
		m_LoadedSeqData.push_back(archive.FileByName(seqFile)->ChunkByIndex(i)->ReadData());
	}
}