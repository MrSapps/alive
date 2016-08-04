#include "oddlib/audio/AliveAudio.h"

#include <algorithm>

AliveAudioSoundbank::AliveAudioSoundbank(Oddlib::LvlArchive& archive, std::string vabID, AliveAudio& aliveAudio)
{
    Oddlib::LvlArchive::File* vhFile = archive.FileByName(vabID + ".VH");
    Oddlib::LvlArchive::File* vbFile = archive.FileByName(vabID + ".VB");

    auto vhStream = vhFile->ChunkByIndex(0)->Stream();
    auto vbStream = vbFile->ChunkByIndex(0)->Stream();

    Vab vab;
    vab.ReadVh(*vhStream);
    vab.ReadVb(*vbStream);

    InitFromVab(vab, aliveAudio);
}

AliveAudioSoundbank::AliveAudioSoundbank(std::string lvlPath, std::string vabID, AliveAudio& aliveAudio)
{
    Oddlib::LvlArchive archive(lvlPath);
    Oddlib::LvlArchive::File* vhFile = archive.FileByName(vabID + ".VH");
    Oddlib::LvlArchive::File* vbFile = archive.FileByName(vabID + ".VB");

    auto vhStream = vhFile->ChunkByIndex(0)->Stream();
    auto vbStream = vbFile->ChunkByIndex(0)->Stream();

    Vab vab;
    vab.ReadVh(*vhStream);
    vab.ReadVb(*vbStream);

    InitFromVab(vab, aliveAudio);
}

// Convert PSX volume envelope info to conventional ADSR. Not exact, because PSX format is richer.
static VolumeEnvelope PSXEnvelopeToADSR(uint16_t low, uint16_t high)
{
    // Following the spec of nocash emu: http://problemkaputt.de/psx-spx.htm#spuvolumeandadsrgenerator

    /*
    ____lower 16bit(at 1F801C08h + N * 10h)___________________________________
    15    Attack Mode(0 = Linear, 1 = Exponential)
    - Attack Direction(Fixed, always Increase) (until Level 7FFFh)
    14 - 10 Attack Shift(0..1Fh = Fast..Slow)
    9 - 8   Attack Step(0..3 = "+7,+6,+5,+4")
    - Decay Mode(Fixed, always Exponential)
    - Decay Direction(Fixed, always Decrease) (until Sustain Level)
    7 - 4   Decay Shift(0..0Fh = Fast..Slow)
    - Decay Step(Fixed, always "-8")
    3 - 0   Sustain Level(0..0Fh); Level = (N + 1) * 800h
    ____upper 16bit(at 1F801C0Ah + N * 10h)___________________________________
    31    Sustain Mode(0 = Linear, 1 = Exponential)
    30    Sustain Direction(0 = Increase, 1 = Decrease) (until Key OFF flag)
    29    Not used ? (should be zero)
    28 - 24 Sustain Shift(0..1Fh = Fast..Slow)
    23 - 22 Sustain Step(0..3 = "+7,+6,+5,+4" or "-8,-7,-6,-5") (inc / dec)
    21    Release Mode(0 = Linear, 1 = Exponential)
    - Release Direction(Fixed, always Decrease) (until Level 0000h)
    20 - 16 Release Shift(0..1Fh = Fast..Slow)
    - Release Step(Fixed, always "-8")
    */

    int attackMode = (low & 0x8000) >> 15;
    int attackShift = (low & 0x7C00) >> 10;
    int attackStep = ((low & 0x300) >> 8) + 4;
    int decayShift = (low & 0xF0) >> 4;
    int sustainLevel = ((low & 0xF) + 1) * 0x800;

    // TODO: Switch from plain ADSR to something which supports these options
    //int sustainMode = (high & 0x8000) >> 15;
    //int sustainDirection = (high & 0x4000) >> 14;
    //int sustainShift = (high & 0x1F00) >> 8;
    //int sustainStep = (high & 0xC0) >> 6;
    int releaseMode = (high & 0x20) >> 5;
    int releaseShift = (high & 0x1F);

    /*
    AdsrCycles = 1 SHL Max(0, ShiftValue - 11)
    AdsrStep = StepValue SHL Max(0, 11 - ShiftValue)
    IF exponential AND increase AND AdsrLevel>6000h THEN AdsrCycles = AdsrCycles * 4
    IF exponential AND decrease THEN AdsrStep = AdsrStep*AdsrLevel / 8000h
    Wait(AdsrCycles); cycles counted at 44.1kHz clock
    AdsrLevel = AdsrLevel + AdsrStep; saturated to 0.. + 7FFFh
    */

    VolumeEnvelope env = { 0 };
    const int maxAmplitude = 0x8000;
    const double expMinAmplitude = 0.1; // Gotta have some threshold when approximating exp with linear curve

    { // Attack
        int64_t durationInSamples = 0;
        int level = 0;
        while (level < maxAmplitude)
        {
            int cycles = 1 << std::max(0, attackShift - 11);
            int step = attackStep << std::max(0, 11 - attackShift);
            if (attackMode == 1 && level > 0x6000) // "Exponential curve"
            {
                // HACK: Stop here to match the linear attack curve better.
                // Makes the attack a bit too fast, but that's maybe better than too slow.
                break;
                // can be reenstated if hack is fixed
                //cycles = cycles * 4;
            }

            durationInSamples += cycles;
            level += step;
        }
        env.AttackTime = durationInSamples / 44100.0;
    }

    { // Sustain
        env.SustainLevel = 1.0*sustainLevel / maxAmplitude;
    }

    { // Decay
        int step = 1 << std::max(0, decayShift - 11);
        int shift = 8 << std::max(0, 11 - decayShift);
        double timeStep = step/44100.0;
        double amplitudeShift = 1.0*shift / maxAmplitude;

        double target = std::max(expMinAmplitude, env.SustainLevel);
        env.DecayTime = -log(target) / (amplitudeShift / timeStep);
    }

    { // Release
        int step = 1 << std::max(0, releaseShift - 11);
        int shift = 8 << std::max(0, 11 - releaseShift);
        double timeStep = step/44100.0;
        double amplitudeShift = 1.0*shift / maxAmplitude;

        // Exponential release is calculated at playback
        env.ExpRelease = (releaseMode == 1);
        env.LinearReleaseTime = 1.0 / (amplitudeShift / timeStep);
    }

    return env;
}

void AliveAudioSoundbank::InitFromVab(Vab& mVab, AliveAudio& /*aliveAudio*/)
{
    for (size_t i = 0; i < mVab.mVagOffsets.size(); i++)
    {
        auto sample = std::make_unique<AliveAudioSample>();
        Uint32 size = static_cast<Uint32>(mVab.mVagOffsets[i].iSampleData.size() / 2);
        sample->m_SampleBuffer.resize(size);
        sample->mSampleSize = size;
        memcpy(sample->m_SampleBuffer.data(), mVab.mVagOffsets[i].iSampleData.data(), mVab.mVagOffsets[i].iSampleData.size());

        m_Samples.emplace_back(std::move(sample));
    }


    /*
    for (size_t i = 0; i < mVab.iOffs.size(); i++)
    {
        auto sample = std::make_unique<AliveAudioSample>();
        if (mVab.iAoVags.size() > 0)
        {
            sample->mSampleSize = mVab.iAoVags[i]->iSize / sizeof(Uint16);
            sample->m_SampleBuffer.resize(mVab.iAoVags[i]->iSize);
            memcpy(sample->m_SampleBuffer.data(), mVab.iAoVags[i]->iSampleData.data(), sample->m_SampleBuffer.size() * sizeof(Uint16));
        }
        else
        {
            sample->mSampleSize = mVab.iOffs[i]->iLengthOrDuration / sizeof(Uint16);
            sample->m_SampleBuffer.resize(sample->mSampleSize);
            memcpy(sample->m_SampleBuffer.data(), aliveAudio.m_SoundsDat.data() + mVab.iOffs[i]->iFileOffset, sample->m_SampleBuffer.size() * sizeof(Uint16));

        }
        m_Samples.emplace_back(std::move(sample));
    }

    for (size_t i = 0; i < mVab.iAoVags.size(); i++)
    {
        auto sample = std::make_unique<AliveAudioSample>();
        sample->mSampleSize = mVab.iAoVags[i]->iSize / sizeof(Uint16);
        sample->m_SampleBuffer.resize(mVab.iAoVags[i]->iSize / sizeof(Uint16));
        memcpy(sample->m_SampleBuffer.data(), mVab.iAoVags[i]->iSampleData.data(), mVab.iAoVags[i]->iSize);
        m_Samples.emplace_back(std::move(sample));
    }
    */

    for (int i = 0; i < 128; i++)
    {
        auto program = std::make_unique<AliveAudioProgram>();
        for (int t = 0; t < mVab.mProgs[i].iNumTones; t++)
        {
            auto tone = std::make_unique<AliveAudioTone>();

            if (mVab.mProgs[i].iTones[t]->iVag == 0) // Some Tones have vag 0? Essentially null?
            {
                continue;
            }

            tone->f_Volume = mVab.mProgs[i].iTones[t]->iVol / 127.0f;
            tone->c_Center = mVab.mProgs[i].iTones[t]->iCenter;
            tone->c_Shift = mVab.mProgs[i].iTones[t]->iShift;
            tone->f_Pan = (mVab.mProgs[i].iTones[t]->iPan / 64.0f) - 1.0f;
            tone->Min = mVab.mProgs[i].iTones[t]->iMin;
            tone->Max = mVab.mProgs[i].iTones[t]->iMax;
            tone->Pitch = mVab.mProgs[i].iTones[t]->iShift / 100.0f;
            tone->Reverbate = (mVab.mProgs[i].iMode == 4);
            tone->m_Sample = m_Samples[mVab.mProgs[i].iTones[t]->iVag - 1].get();
         
#if 1 // Use nocash emu based ADSR calc
            VolumeEnvelope env = PSXEnvelopeToADSR(  mVab.mProgs[i].iTones[t]->iAdsr1,
                                                     mVab.mProgs[i].iTones[t]->iAdsr2);
            tone->Env = env;

            if (env.AttackTime > 0.5) // This works until the loop database is added.
            {
                tone->Loop = true;
            }
#else
            unsigned short ADSR1 = mVab.mProgs[i].iTones[t]->iAdsr1;
            unsigned short ADSR2 = mVab.mProgs[i].iTones[t]->iAdsr2;
            REAL_ADSR realADSR = {};
            PSXConvADSR(&realADSR, ADSR1, ADSR2, false);

            tone->AttackTime = realADSR.attack_time;
            tone->DecayTime = realADSR.decay_time;
            tone->ReleaseTime = realADSR.release_time;
            tone->SustainTime = realADSR.sustain_time;
            tone->SustainLevel = realADSR.sustain_level;

            if (realADSR.attack_time > 1) // This works until the loop database is added.
            {
                tone->Loop = true;
            }
#endif

            /*if (i == 27 || i == 81)
            tone->Loop = true;*/

            program->m_Tones.emplace_back(std::move(tone));
        }

        m_Programs.emplace_back(std::move(program));
    }
}
