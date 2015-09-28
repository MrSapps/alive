#include "sound.hpp"
#include "oddlib/audio/SequencePlayer.h"
#include "imgui/imgui.h"
#include "core/audiobuffer.hpp"
#include "logger.hpp"
#include "filesystem.hpp"
#include <memory>

std::unique_ptr<SequencePlayer> player;
bool firstChange = true;
int targetSong = -1;
bool loopSong = false;

void BarLoop()
{
    printf("Bar Loop!");

    if (targetSong != -1)
    {
        if (player->LoadSequenceData(AliveAudio::m_LoadedSeqData[targetSong]) == 0)
        {
            player->PlaySequence();
        }
        targetSong = -1;
    }
}


void ChangeTheme(void * /*clientData*/, FileSystem& fs)
{
    try
    {
        //player->StopSequence();

        jsonxx::Object theme = AliveAudio::m_Config.get<jsonxx::Array>("themes").get<jsonxx::Object>(14);

        const std::string lvlFileName = theme.get<jsonxx::String>("lvl", "null") + ".LVL";
        Oddlib::LvlArchive archive(fs.ResourceExists(lvlFileName)->Open(lvlFileName));
        AliveAudio::LoadAllFromLvl(archive, theme.get<jsonxx::String>("vab", "null"), theme.get<jsonxx::String>("seq", "null"));

        //TwRemoveAllVars(m_GUIFileList);
        for (Uint32 i = 0; i < archive.FileCount(); i++)
        {
            //char labelTest[100];
            //        sprintf(labelTest, "group='Files' label='%s'", archive.mFiles[i].get()->FileName().c_str());
            //TwAddButton(m_GUIFileList, nullptr, nullptr, nullptr, labelTest);
        }

        // TwRemoveAllVars(m_GUITones);
        for (int e = 0; e < 128; e++)
        {
            for (size_t i = 0; i < AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones.size(); i++)
            {
                // char labelTest[100];
                //sprintf(labelTest, "group='Program %i' label='%i - Min:%i Max:%i'", e, i, AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones[i]->Min, AliveAudio::m_CurrentSoundbank->m_Programs[e]->m_Tones[i]->Max);
                // TwAddButton(m_GUITones, nullptr, PlaySound, (void*)new int[2]{ e, i }, labelTest);
            }
        }

        // TwRemoveAllVars(m_GUISequences);
        for (size_t i = 0; i < AliveAudio::m_LoadedSeqData.size(); i++)
        {
            //char labelTest[100];
            //sprintf(labelTest, "group='Seq Files' label='Play Seq %i'", i);

            // TwAddButton(m_GUISequences, nullptr, PlaySong, (char*)i, labelTest);
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

}

void Sound::Update()
{

}

void Sound::Render(int w, int h)
{
    static bool bSet = false;
    if (!bSet)
    {
        ImGui::SetNextWindowPos(ImVec2(320, 250));
        bSet = true;
    }

    ImGui::Begin("Sound", nullptr, ImVec2(250, 280), 1.0f, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Music test", ImVec2(ImGui::GetWindowWidth(), 20)))
    {
        int id = 26; // death sound
        player.reset(new SequencePlayer());
        player->m_QuarterCallback = BarLoop;
        ChangeTheme(0, mFs);
        if (firstChange || player->m_PlayerState == ALIVE_SEQUENCER_FINISHED || player->m_PlayerState == ALIVE_SEQUENCER_STOPPED)
        {
            if (id < AliveAudio::m_LoadedSeqData.size() && player->LoadSequenceData(AliveAudio::m_LoadedSeqData[id]) == 0)
            {
                player->PlaySequence();
                firstChange = false;
            }
        }
        else
        {
            targetSong = id;
        }
    }

    ImGui::End();
}
