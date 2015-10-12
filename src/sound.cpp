#include "sound.hpp"
#include "oddlib/audio/SequencePlayer.h"
#include "imgui/imgui.h"
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
        Oddlib::LvlArchive archive(fs.ResourceExists(lvlFileName)->Open(lvlFileName));
        mAliveAudio.LoadAllFromLvl(archive, parts[1], parts[2]); // vab, bsq
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

void Sound::Render(int /*w*/, int /*h*/)
{
    static bool bSet = false;
    if (!bSet)
    {
        try
        {
            // Currently requires sounds.dat to be available from the get-go, so make
            // this failure non-fatal
            mAliveAudio.AliveInitAudio(mFs);
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR("Audio init failure: " << ex.what());
        }

        ImGui::SetNextWindowPos(ImVec2(320, 250));
        bSet = true;
        auto themes = mAliveAudio.m_Config.get<jsonxx::Array>("themes");
        for (auto i = 0u; i < themes.size(); i++)
        {
            auto theme = themes.get<jsonxx::Object>(i);

            const std::string lvlFileName = theme.get<jsonxx::String>("lvl", "null") + ".LVL";
            auto res = mFs.ResourceExists(lvlFileName);
            if (res)
            {
                Oddlib::LvlArchive archive(res->Open(lvlFileName));
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


    ImGui::Begin("Sound", nullptr, ImVec2(250, 280), 1.0f, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    static int selectedIndex = 0; 
    for (size_t i = 0; i < mThemes.size(); i++)
    {
        if (ImGui::Selectable(mThemes[i].c_str(), static_cast<int>(i) == selectedIndex))
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

    ImGui::End();

    { ImGui::Begin("Audio output settings");
        ImGui::Checkbox("Use antialiasing (not implemented)", &mAliveAudio.AntiAliasFilteringEnabled);

        { ImGui::BeginGroup();
            if (ImGui::RadioButton("No interpolation", mAliveAudio.Interpolation == AudioInterpolation_none))
                mAliveAudio.Interpolation = AudioInterpolation_none;

            if (ImGui::RadioButton("Linear interpolation", mAliveAudio.Interpolation == AudioInterpolation_linear))
                mAliveAudio.Interpolation = AudioInterpolation_linear;

            if (ImGui::RadioButton("Cubic interpolation", mAliveAudio.Interpolation == AudioInterpolation_cubic))
                mAliveAudio.Interpolation = AudioInterpolation_cubic;

            if (ImGui::RadioButton("Hermite interpolation", mAliveAudio.Interpolation == AudioInterpolation_hermite))
                mAliveAudio.Interpolation = AudioInterpolation_hermite;

        ImGui::EndGroup(); }

        ImGui::Checkbox("Force reverb", &mAliveAudio.ForceReverb);
        ImGui::SliderFloat("Reverb mix", &mAliveAudio.ReverbMix, 0.0f, 1.0f);

        ImGui::Checkbox("Disable resampling (= no freq changes)", &mAliveAudio.DebugDisableVoiceResampling);
    ImGui::End(); }
}
