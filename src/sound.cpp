#include "sound.hpp"
#include "oddlib/audio/SequencePlayer.h"
#include "gui.h"
#include "core/audiobuffer.hpp"
#include "logger.hpp"
#include "resourcemapper.hpp"

void Sound::BarLoop()
{
    printf("Bar Loop!");

    /*
    if (mTargetSong != -1)
    {
        if (mSeqPlayer->LoadSequenceData(mAliveAudio.m_LoadedSeqData[mTargetSong]) == 0)
        {
            mSeqPlayer->PlaySequence();
        }
        mTargetSong = -1;
    }*/
}

Sound::Sound(IAudioController& audioController, ResourceLocator& locator)
    : mAudioController(audioController), mLocator(locator)
{
    mAudioController.AddPlayer(&mAliveAudio);
}

Sound::~Sound()
{
    // Ensure the audio call back stops at this point else will be
    // calling back into freed objects
    SDL_AudioQuit();

    mAudioController.RemovePlayer(&mAliveAudio);
}

void Sound::Update()
{
    if (mSeqPlayer)
    {
        mSeqPlayer->PlayerThreadFunction();
    }
}

void Sound::Render(GuiContext *gui, int /*w*/, int /*h*/)
{
    gui_begin_window(gui, "Sound");

    for (const auto& musicInfo : mLocator.mResMapper.mMusicMaps)
    {
        if (gui_button(gui, musicInfo.first.c_str()))
        {
            std::unique_ptr<IMusic> music = mLocator.LocateMusic(musicInfo.first.c_str());
            if (music)
            {
                mAudioController.SetAudioSpec(1024, AliveAudioSampleRate);
                if (!mSeqPlayer)
                {
                    mSeqPlayer = std::make_unique<SequencePlayer>(mAliveAudio);
                }

                auto soundBank = std::make_unique<AliveAudioSoundbank>(*music->mVab, mAliveAudio);
                mAliveAudio.SetSoundbank(std::move(soundBank));
                mSeqPlayer->LoadSequenceStream(*music->mSeqData);
                mSeqPlayer->PlaySequence();
            }
        }
    }

    gui_end_window(gui);

    gui_begin_window(gui, "Audio output settings");
    gui_checkbox(gui, "Use antialiasing (not implemented)", &mAliveAudio.mAntiAliasFilteringEnabled);

    if (gui_radiobutton(gui, "No interpolation", mAliveAudio.Interpolation == AudioInterpolation_none))
        mAliveAudio.Interpolation = AudioInterpolation_none;

    if (gui_radiobutton(gui, "Linear interpolation", mAliveAudio.Interpolation == AudioInterpolation_linear))
        mAliveAudio.Interpolation = AudioInterpolation_linear;

    if (gui_radiobutton(gui, "Cubic interpolation", mAliveAudio.Interpolation == AudioInterpolation_cubic))
        mAliveAudio.Interpolation = AudioInterpolation_cubic;

    if (gui_radiobutton(gui, "Hermite interpolation", mAliveAudio.Interpolation == AudioInterpolation_hermite))
        mAliveAudio.Interpolation = AudioInterpolation_hermite;

    gui_checkbox(gui, "Force reverb", &mAliveAudio.ForceReverb);
    gui_slider(gui, "Reverb mix", &mAliveAudio.ReverbMix, 0.0f, 1.0f);

    gui_checkbox(gui, "Disable resampling (= no freq changes)", &mAliveAudio.DebugDisableVoiceResampling);
    gui_end_window(gui);
}
