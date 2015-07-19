#pragma once

#include "Sample.h"
#include "Program.h"

class AliveAudioSoundbank
{
public:
	~AliveAudioSoundbank();
	AliveAudioSoundbank(std::string fileName);
	AliveAudioSoundbank(std::string lvlPath, std::string vabName);
	AliveAudioSoundbank(Oddlib::LvlArchive& lvlArchive, std::string vabName);

	void InitFromVab(Vab& vab);
	std::vector<AliveAudioSample *> m_Samples;
	std::vector<AliveAudioProgram *> m_Programs;
};