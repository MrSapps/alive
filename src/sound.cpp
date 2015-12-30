#include "sound.hpp"
#include "oddlib/audio/SequencePlayer.h"
#include "gui.h"
#include "core/audiobuffer.hpp"
#include "logger.hpp"
#include "filesystem.hpp"

void Sound::BarLoop()
{
    printf("Bar Loop!");

    if (mTargetSong != -1)
    {
        if (mSeqPlayer->LoadSequenceData(mAliveAudio.m_LoadedSeqData[mTargetSong]) == 0)
        {
            mSeqPlayer->PlaySequence();
        }
        mTargetSong = -1;
    }
}


void Sound::ChangeTheme(FileSystem& fs, const std::deque<std::string>& parts)
{
    try
    {
        const std::string lvlFileName = parts[0];
        auto stream = fs.ResourcePaths().Open(lvlFileName);
        if (stream)
        {
            Oddlib::LvlArchive archive(std::move(stream));
            mAliveAudio.LoadAllFromLvl(archive, parts[1], parts[2]); // vab, bsq
        }
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("Audio error: " << ex.what());
    }
}

Sound::Sound(GameData& gameData, IAudioController& audioController, FileSystem& fs)
    : mGameData(gameData), mAudioController(audioController), mFs(fs)
{
    mAudioController.AddPlayer(&mAliveAudio);
}

Sound::~Sound()
{
    mAudioController.RemovePlayer(&mAliveAudio);
}

void Sound::Update()
{
    if (mSeqPlayer)
    {
        mSeqPlayer->m_PlayerThreadFunction();
    }
}

void Sound::Render(GuiContext *gui, int /*w*/, int /*h*/)
{
    static bool bSet = false;
    if (!bSet)
    {
        try
        {
            bSet = true;

            // Currently requires sounds.dat to be available from the get-go, so make
            // this failure non-fatal
            mAliveAudio.AliveInitAudio(mFs);

            auto themes = mAliveAudio.m_Config.get<jsonxx::Array>("themes");
            for (auto i = 0u; i < themes.size(); i++)
            {
                auto theme = themes.get<jsonxx::Object>(i);

                const std::string lvlFileName = theme.get<jsonxx::String>("lvl", "null") + ".lvl";
                auto stream = mFs.ResourcePaths().Open(lvlFileName);
                if (stream)
                {
                    Oddlib::LvlArchive archive(std::move(stream));
                    auto vabName = theme.get<jsonxx::String>("vab", "null");
                    auto bsqName = theme.get<jsonxx::String>("seq", "null");

                    Oddlib::LvlArchive::File* file = archive.FileByName(bsqName);
                    for (auto j = 0u; j < file->ChunkCount(); j++)
                    {
                        mThemes.push_back(lvlFileName + "!" + vabName + "!" + bsqName + "!" + std::to_string(j));
                    }
                }
            }
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR("Audio init failure: " << ex.what());
        }
    }

    gui_begin_window(gui, "Sound");

    static int selectedIndex = 0; 
    for (size_t i = 0; i < mThemes.size(); i++)
    {
        if (gui_selectable(gui, mThemes[i].c_str(), static_cast<int>(i) == selectedIndex))
        {
            selectedIndex = static_cast<int>(i);
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(mThemes.size()) && !mThemes.empty())
            {
                mAudioController.SetAudioSpec(1024, AliveAudioSampleRate);
                mSeqPlayer = std::make_unique<SequencePlayer>(mAliveAudio);
                mSeqPlayer->m_QuarterCallback = [&]() { BarLoop(); };
                const auto parts = string_util::split(mThemes[selectedIndex], '!');
                int seqId = std::atoi(parts[3].c_str());
                ChangeTheme(mFs, parts);
                if (mSeqPlayer->m_PlayerState == ALIVE_SEQUENCER_FINISHED || mSeqPlayer->m_PlayerState == ALIVE_SEQUENCER_STOPPED)
                {
                    if (seqId < static_cast<int>(mAliveAudio.m_LoadedSeqData.size()) && mSeqPlayer->LoadSequenceData(mAliveAudio.m_LoadedSeqData[seqId]) == 0)
                    {
                        mSeqPlayer->PlaySequence();
                    }
                }
                else
                {
                    mTargetSong = seqId;
                }
            }
        }
    }
    gui_end_window(gui);

    { gui_begin_window(gui, "Audio output settings");
        gui_checkbox(gui, "Use antialiasing (not implemented)", &mAliveAudio.AntiAliasFilteringEnabled);

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
    gui_end_window(gui);  }
}
