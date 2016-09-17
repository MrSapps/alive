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

static float RandFloat(float a, float b)
{
    return ((b - a)*((float)rand() / RAND_MAX)) + a;
}

void Sound::PlaySoundEffect(const char* effectName)
{
    auto it = mSfxCache.find(effectName);
    if (it != std::end(mSfxCache))
    {
        auto& fx = it->second;
        mAliveAudio.NoteOn(fx->mProgram, fx->mNote, 127, 0.0f, RandFloat(fx->mMinPitch, fx->mMaxPitch));
        return;
    }

    auto soundEffect = mLocator.LocateSoundEffect(effectName);
    if (soundEffect)
    {
        LOG_INFO("Play sound effect: " << effectName);

        // TODO: Might want another player instance or a better way of dividing sound fx/vs seq music - also needs higher
        // abstraction since music/fx could be wave/ogg/mp3 etc

        mAudioController.SetAudioSpec(1024, AliveAudioSampleRate);

        auto soundBank = std::make_unique<AliveAudioSoundbank>(*soundEffect->mVab, mAliveAudio);
        mAliveAudio.SetSoundbank(std::move(soundBank));

        // TODO: Set pitch
        mAliveAudio.NoteOn(soundEffect->mProgram, soundEffect->mNote, 127, 0.0f, RandFloat(soundEffect->mMinPitch, soundEffect->mMaxPitch));

        mSfxCache[effectName] = std::move(soundEffect);
    }
    else
    {
        LOG_ERROR("Sound effect:" << effectName << " not found");
    }
}

Sound::Sound(IAudioController& audioController, ResourceLocator& locator, sol::state& luaState)
    : mAudioController(audioController), mLocator(locator)
{
    mAudioController.AddPlayer(&mAliveAudio);

    luaState.set_function("PlaySoundEffect", &Sound::PlaySoundEffect, this);
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

void Sound::AudioSettingsUi(GuiContext* gui)
{
    gui_begin_window(gui, "Audio output settings");
    gui_checkbox(gui, "Use antialiasing (not implemented)", &mAliveAudio.mAntiAliasFilteringEnabled);

    if (gui_radiobutton(gui, "No interpolation", mAliveAudio.Interpolation == AudioInterpolation_none))
    {
        mAliveAudio.Interpolation = AudioInterpolation_none;
    }

    if (gui_radiobutton(gui, "Linear interpolation", mAliveAudio.Interpolation == AudioInterpolation_linear))
    {
        mAliveAudio.Interpolation = AudioInterpolation_linear;
    }

    if (gui_radiobutton(gui, "Cubic interpolation", mAliveAudio.Interpolation == AudioInterpolation_cubic))
    {
        mAliveAudio.Interpolation = AudioInterpolation_cubic;
    }

    if (gui_radiobutton(gui, "Hermite interpolation", mAliveAudio.Interpolation == AudioInterpolation_hermite))
    {
        mAliveAudio.Interpolation = AudioInterpolation_hermite;
    }

    gui_checkbox(gui, "Music browser", &mMusicBrowser);
    gui_checkbox(gui, "Sound effect browser", &mSoundEffectBrowser);

    gui_checkbox(gui, "Force reverb", &mAliveAudio.ForceReverb);
    gui_slider(gui, "Reverb mix", &mAliveAudio.ReverbMix, 0.0f, 1.0f);

    gui_checkbox(gui, "Disable resampling (= no freq changes)", &mAliveAudio.DebugDisableVoiceResampling);
    gui_end_window(gui);
}

void Sound::MusicBrowserUi(GuiContext* gui)
{
    gui_begin_window(gui, "Music");

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
}

void Sound::SoundEffectBrowserUi(GuiContext* gui)
{
    gui_begin_window(gui, "Sound effects");

    for (const auto& soundEffectResourceName : mLocator.mResMapper.mSoundEffectMaps)
    {
        if (gui_button(gui, soundEffectResourceName.first.c_str()))
        {
            PlaySoundEffect(soundEffectResourceName.first.c_str());
        }
    }
    gui_end_window(gui);
}

void Sound::Render(GuiContext *gui, int /*w*/, int /*h*/)
{
    AudioSettingsUi(gui);
    if (mMusicBrowser)
    {
        MusicBrowserUi(gui);
    }
    if (mSoundEffectBrowser)
    {
        SoundEffectBrowserUi(gui);
    }
}
