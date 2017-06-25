#include "sound.hpp"
#include "oddlib/audio/SequencePlayer.h"
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

/*static*/ void Sound::RegisterScriptBindings()
{
    Sqrat::Class<Sound, Sqrat::NoConstructor<Sound>> c(Sqrat::DefaultVM::Get(), "Sound");
    c.Func("PlaySoundEffect", &Sound::PlaySoundEffect);
    Sqrat::RootTable().Bind("Sound", c);
}

Sound::Sound(IAudioController& audioController, ResourceLocator& locator)
    : mAudioController(audioController), mLocator(locator), mScriptInstance("gSound", this)
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

void Sound::AudioSettingsUi()
{
    if (ImGui::Begin("Audio output settings"))
    {
        ImGui::Checkbox("Use antialiasing (not implemented)", &mAliveAudio.mAntiAliasFilteringEnabled);

        if (ImGui::RadioButton("No interpolation", mAliveAudio.Interpolation == AudioInterpolation_none))
        {
            mAliveAudio.Interpolation = AudioInterpolation_none;
        }

        if (ImGui::RadioButton("Linear interpolation", mAliveAudio.Interpolation == AudioInterpolation_linear))
        {
            mAliveAudio.Interpolation = AudioInterpolation_linear;
        }

        if (ImGui::RadioButton("Cubic interpolation", mAliveAudio.Interpolation == AudioInterpolation_cubic))
        {
            mAliveAudio.Interpolation = AudioInterpolation_cubic;
        }

        if (ImGui::RadioButton("Hermite interpolation", mAliveAudio.Interpolation == AudioInterpolation_hermite))
        {
            mAliveAudio.Interpolation = AudioInterpolation_hermite;
        }

        ImGui::Checkbox("Music browser", &mMusicBrowser);
        ImGui::Checkbox("Sound effect browser", &mSoundEffectBrowser);

        ImGui::Checkbox("Force reverb", &mAliveAudio.ForceReverb);
        ImGui::SliderFloat("Reverb mix", &mAliveAudio.ReverbMix, 0.0f, 1.0f);

        ImGui::Checkbox("Disable resampling (= no freq changes)", &mAliveAudio.DebugDisableVoiceResampling);
    }
    ImGui::End();
}

void Sound::MusicBrowserUi()
{
    if (ImGui::Begin("Music"))
    {
        for (const auto& musicInfo : mLocator.mResMapper.mMusicMaps)
        {
            if (ImGui::Button(musicInfo.first.c_str()))
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
    }
    ImGui::End();
}

void Sound::SoundEffectBrowserUi()
{
    if (ImGui::Begin("Sound effects"))
    {
        for (const auto& soundEffectResourceName : mLocator.mResMapper.mSoundEffectMaps)
        {
            if (ImGui::Button(soundEffectResourceName.first.c_str()))
            {
                PlaySoundEffect(soundEffectResourceName.first.c_str());
            }
        }
    } 
    ImGui::End();
}

void Sound::DebugUi()
{
    AudioSettingsUi();
    if (mMusicBrowser)
    {
        MusicBrowserUi();
    }
    if (mSoundEffectBrowser)
    {
        SoundEffectBrowserUi();
    }
}

void Sound::Render()
{

}
