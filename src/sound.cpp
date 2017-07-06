#include "sound.hpp"
#include "oddlib/audio/SequencePlayer.h"
#include "core/audiobuffer.hpp"
#include "logger.hpp"
#include "resourcemapper.hpp"

static float RandFloat(float a, float b)
{
    return ((b - a)*((float)rand() / RAND_MAX)) + a;
}

void Sound::PlaySoundScript(const char* soundName)
{
    auto ret = PlaySound(soundName, nullptr, true, true);
    if (ret)
    {
        mSeqPlayers.push_back(std::move(ret));
    }
}

std::unique_ptr<SequencePlayer> Sound::PlaySound(const char* soundName, const char* explicitSoundBankName, bool useMusicRec, bool useSfxRec)
{
    // TODO: Cache all sfx up front

    std::unique_ptr<ISound> pSound = mLocator.LocateSound(soundName, explicitSoundBankName, useMusicRec, useSfxRec);
    if (pSound)
    {
        LOG_INFO("Play sound: " << soundName);

        // TODO: Might want another player instance or a better way of dividing sound fx/vs seq music - also needs higher
        // abstraction since music/fx could be wave/ogg/mp3 etc


        // TODO: Only do this once and resample PSX FMV audio to match it
        //mAudioController.SetAudioSpec(1024*8, kAliveAudioSampleRate);

        // TODO: Allow more than one so things can play in parallel
        std::lock_guard<std::recursive_mutex> lock(mSeqPlayersMutex);

        auto player = std::make_unique<SequencePlayer>(soundName, *pSound->mVab);
        if (pSound->mSeqData)
        {
            player->LoadSequenceStream(*pSound->mSeqData);
            player->PlaySequence();
        }
        else
        {
            player->NoteOnSingleShot(pSound->mProgram, pSound->mNote, 127, 0.0f, RandFloat(static_cast<f32>(pSound->mMinPitch), static_cast<f32>(pSound->mMaxPitch)));
        }

        return player;
    }
    else
    {
        LOG_ERROR("Sound: " << soundName << " not found");
    }
    return nullptr;
}

void Sound::SetTheme(const char* themeName)
{
    std::lock_guard<std::recursive_mutex> lock(mSeqPlayersMutex);

    mAmbiance = nullptr;
    mMusicTrack = nullptr;
    mActiveTheme = mLocator.LocateSoundTheme(themeName);
    if (!mActiveTheme)
    {
        LOG_ERROR("Music theme " << themeName << " was not found");
    }
}

void Sound::Preload()
{
    if (mActiveTheme)
    {
        // TODO: Preload music
    }

    // TODO: Preload sound effects
}

void Sound::HandleEvent(const char* eventName)
{
    // TODO: Need quarter beat transition in some cases
    EnsureAmbiance();

    auto ret = PlayThemeEntry(eventName);
    if (ret)
    {
        mMusicTrack = std::move(ret);
    }
}

std::unique_ptr<SequencePlayer> Sound::PlayThemeEntry(const char* entryName)
{
    if (mActiveTheme)
    {
        mActiveThemeEntry.SetMusicThemeEntry(mActiveTheme->FindEntry(entryName));

        const MusicThemeEntry* entry = mActiveThemeEntry.Entry();
        if (entry)
        {
            return PlaySound(entry->mMusicName.c_str(), nullptr, true, true);
        }
    }
    return nullptr;
}

void Sound::EnsureAmbiance()
{
    if (!mAmbiance)
    {
        mAmbiance = PlayThemeEntry("AMBIANCE");
    }
}

bool Sound::Play(f32* stream, u32 len)
{
    std::lock_guard<std::recursive_mutex> lock(mSeqPlayersMutex);

    if (mAmbiance)
    {
        mAmbiance->Play(stream, len);
    }

    if (mMusicTrack)
    {
        mMusicTrack->Play(stream, len);
    }

    for (auto& player : mSeqPlayers)
    {
        player->Play(stream, len);
    }
    return false;
}

/*static*/ void Sound::RegisterScriptBindings()
{
    Sqrat::Class<Sound, Sqrat::NoConstructor<Sound>> c(Sqrat::DefaultVM::Get(), "Sound");
    c.Func("PlaySoundEffect", &Sound::PlaySoundScript);
    Sqrat::RootTable().Bind("Sound", c);
}

Sound::Sound(IAudioController& audioController, ResourceLocator& locator)
    : mAudioController(audioController), mLocator(locator), mScriptInstance("gSound", this)
{
    mAudioController.AddPlayer(this);
}

Sound::~Sound()
{
    // Ensure the audio call back stops at this point else will be
    // calling back into freed objects
    SDL_AudioQuit();

    mAudioController.RemovePlayer(this);
}

void Sound::Update()
{
    if (Debugging().mBrowserUi.soundBrowserOpen)
    {
        {
            std::lock_guard<std::recursive_mutex> lock(mSeqPlayersMutex);
            if (!mSeqPlayers.empty())
            {
                mSeqPlayers[0]->DebugUi();
            }

            if (ImGui::Begin("Active SEQs"))
            {
                if (mAmbiance)
                {
                   ImGui::Text("Ambiance: %s", mAmbiance->Name().c_str());
                }

                if (mMusicTrack)
                {
                    ImGui::Text("Music: %s", mMusicTrack->Name().c_str());
                }

                int i = 0;
                for (auto& player : mSeqPlayers)
                {
                    i++;
                    if (ImGui::Button((std::to_string(i) + player->Name()).c_str()))
                    {
                        // TODO: Kill whatever this SEQ is
                    }
                }
            }
            ImGui::End();
        }

        SoundBrowserUi();
    }

    std::lock_guard<std::recursive_mutex> lock(mSeqPlayersMutex);
    for (auto it = mSeqPlayers.begin(); it != mSeqPlayers.end();)
    {
        if ((*it)->AtEnd())
        {
            it = mSeqPlayers.erase(it);
        }
        else
        {
            it++;
        }
    }

    if (mAmbiance)
    {
        mAmbiance->Update();
        if (mAmbiance->AtEnd())
        {
            mAmbiance->Restart();
        }
    }

    if (mMusicTrack)
    {
        mMusicTrack->Update();
        if (mMusicTrack->AtEnd())
        {
            if (mActiveThemeEntry.ToNextEntry())
            {
                mMusicTrack = PlaySound(mActiveThemeEntry.Entry()->mMusicName.c_str(), nullptr, true, true);
            }
            else
            {
                mMusicTrack = nullptr;
            }
        }
    }

    for (auto& player : mSeqPlayers)
    {
        player->Update();
    }
}

// TODO: Remove this when all tracks have been tested
// Only show one button to play tracks that have been tested/confirmed working
static std::set<std::string> gTestedSounds =
{
    "BARRAMB",
    "GUN",
    "MUDOHM",
    "OHM",
    "Abe_StopIt",
    "SSCRATCH", // TODO: psx diff pitch
    "SLIGBOMB",
    "SLIGBOM2",
    "OOPS",
    "PATROL",
    "SLEEPING",
    "WHEEL",
    "MYSTERY1",
    "MYSTERY2",
    "NEGATIV1",
    "NEGATIV3",
    "BAEND_3",
    "BAEND_4",
    "BAEND_5",
    "BAEND_6",
    "BA_1_1",
    "BA_2_1",
    "BA_3_1",
    "BA_4_1",
    "BA_5_1",
    "BA_6_1",
    "PARALLYA",
    "PIGEONS",
    "FLEECHES",
    "GRINDER",
    "CHIPPER",
    "BREWAMB",
    "BREND_1",
    "BREND_2",
    "BREND_3",
    "BREND_4",
    "BREND_5",
    "BREND_6",
    "BR_1_1",
    "BR_2_1",
    "BR_3_1",
    "BR_4_1",
    "BR_5_1",
    "BR_6_1",
    "BR_8_1",
    "BR_9_1",
    "BR_10_1",
    "BONEAMB",
    "BW_1_1",
    "BW_2_1",
    "BW_3_1",
    "BW_4_1",
    "BW_5_1",
    "BW_6_1",
    "BW_8_1",
    "BW_9_1",
    "BW_10_1",
    "BWEND_3",
    "BWEND_4",
    "BWEND_5",
    "BWEND_6",
    "BWEND_8",
    "BWEND_9",
    "BWEND_10",
    "FEECOAMB",
    "AE_FE_1_1",
    "AE_FE_2_1",
    "AE_FE_3_1",
    "AE_FE_4_1",
    "AO_FE_2_1",
    "AO_FE_4_1",
    "AO_FE_5_1",
    "AE_FE_5_1",
    "AE_FE_6_1",
    "AE_FE_8_1",
    "AE_FE_9_1",
    "AE_FE_10_1",
    "FEEND_3",
    "FEEND_4",
    "FEEND_5",
    "FEEND_6",
    "MI_1_1",
    "MI_2_1",
    "MI_3_1",
    "MI_4_1",
    "MI_5_1",
    "MI_6_1",
    "MI_8_1",
    "MI_9_1",
    "MI_10_1",
    "NE_1_1",
    "NE_2_1",
    "NE_3_1",
    "NE_4_1",
    "NE_5_1",
    "NE_6_1",
    "NE_7_1",
    "NE_8_1",
    "NE_9_1",
    "PARAMB", // PE vs PV as 1 sound diff @ intro


    "NECRAMB",
    "POSITIV1", // TODO: Fix PC vs PSX pitch
    "POSITIV9", // TODO: Fix PC vs PSX pitch
    "EBELL2",
    "PV_1_1",
    "PV_2_1", // Pc vs PSX slight change
    "PV_3_1",
    "PV_4_1", // Pc vs PSX slight change
    "PV_5_1",
    "PV_6_1", // Pc vs PSX slight change
    "PV_7_1", // Pc vs PSX slight change
    "PV_8_1", // Pc vs PSX slight change
    "PV_9_1", // Pc vs PSX slight change
    "PVEND_3",
    "PVEND_4",
    "PVEND_5",
    "PVEND_6",
    "D1_0_1",
    "D1_0_2",
    "D1_0_3",
    "D1_1_1",
    "D1_1_2",
    "D1_1_3",
    "D1_1_4",
    "D1_1_5",
    "D1_2_1",
    "D1_2_2",
    "D1_2_3",
    "D1_2_4",
    "D1_2_5",
    "D1_3_1", // not used?
    "D1_4_1",
    "D1_5_1",
    "D1_6_1",
    "D2AMB",
    "AO_PSX_DEMO_ALL_4_1",
    "AO_PSX_DEMO_ALL_5_1",
    "AO_PSX_DEMO_ALL_5_2",
    "AO_PSX_DEMO_ALL_5_3",
    "AO_PSX_DEMO_ALL_7_1",
    "AO_PSX_DEMO_ALL_8_1",
    "ALL_4_1",
    "ALL_5_1",
    "ALL_5_2",
    "ALL_5_3",
    "ALL_7_1",
    "ALL_8_1",
    "OPT_0_1",
    "OPT_0_2",
    "OPT_0_3",
    "OPT_0_4",
    "OPT_0_5",
    "OPT_1_1",
    "OPT_1_2",
    "OPT_1_3",
    "OPT_1_4",
    "OPT_1_5",
    "F1AMB",
    "BATSQUEK",
    "D1AMB",
    "ESCRATCH", // Psx vs pc diff pitch??
    "ONCHAIN",
    "SLOSLEEP",
    "AO_PSX_DEMO_ABEMOUNT",
    "PANTING",
    "SCRAMB",
    "SV_1_1",
    "SV_2_1",
    "SV_3_1",
    "SV_4_1",
    "SV_5_1",
    "SV_6_1",
    "SV_7_1",
    "SV_8_1",
    "SV_9_1",
    "SVEND_3",
    "SVEND_4",
    "SVEND_5",
    "SVEND_6",
    "SVEND_7",
    "SVEND_8",
    "SVEND_9", // Seems like hardly any sound - probably not used?
    "PVEND_7",
    "PVEND_8",
    "PVEND_9",
    "D2_0_1", // Never used ?
    "D2_0_2", // Never used ?
    "D2_1_1",
    "D2_1_2",
    "D2_2_1",
    "D2_2_2",
    "D2_4_1",
    "D2_5_1",
    "D2_6_1",
    "DE_2_1",
    "DE_4_1",
    "DE_5_1",
    "E1AMB",
    "E2AMB",
    "F1_0_1",
    "F1_0_2",
    "F1_0_3",
    "F1_1_1",
    "F1_1_2",
    "F1_1_3",
    "F1_2_1",
    "F1_2_2",
    "F1_2_3",
    "F1_2_4",
    "F1_3_1",
    "F1_4_1",
    "F1_5_1",
    "F1_6_1",
    "F2AMB",
    "AO_PSX_DEMO_E1AMB",
    "MLAMB",
    "ML_0_1",
    "ML_0_2",
    "ML_0_3",
    "ML_0_4",
    "ML_0_5",
    "ML_1_1",
    "ML_1_2",
    "ML_1_3",
    "ML_1_4",
    "ML_1_5",
    "ML_2_1",
    "ML_2_2",
    "ML_2_3",
    "ML_2_4",
    "ML_2_5",
    "ML_3_1",
    "ML_4_1",
    "ML_5_1",
    "ML_6_1",
    "F2_2_1",
    "F2_4_1",
    "F2_5_1",
    "F2_6_1",
    "RF_0_1",
    "RF_0_2",
    "RF_0_3",
    "RF_0_4",
    "RF_1_1",
    "RF_1_2",
    "RF_1_3",
    "RF_2_1",
    "RF_2_2",
    "RF_2_3",
    "RF_2_4",
    "RF_4_1",
    "RF_5_1",
    "RF_6_1",
    "RFAMB",
    "AO_PSX_DEMO_RF_5_1",
    "AO_PSX_DEMO_RF_6_1",
    "AO_PSX_DEMO_ALL_2_1",
    "AO_PSX_DEMO_ALL_3_1",
    "AO_PSX_DEMO_ALL_6_1",
    "AO_PSX_DEMO_ALL_6_2",
    "E1_4_1",
    "E1_5_1",
    "E1_6_1",
    "AO_PSX_DEMO_E1_0_1",
    "AO_PSX_DEMO_E1_0_2",
    "AO_PSX_DEMO_E1_0_3",
    "AO_PSX_DEMO_E1_0_4",
    "AO_PSX_DEMO_E1_0_5",
    "AO_PSX_DEMO_E1_1_1",
    "AO_PSX_DEMO_E1_1_2",
    "AO_PSX_DEMO_E1_1_3",
    "AO_PSX_DEMO_E1_1_4",
    "AO_PSX_DEMO_E1_1_5",
    "E1_0_1",
    "E1_0_2",
    "E1_0_3",
    "E1_0_4",
    "E1_0_5",
    "E1_1_1",
    "E1_1_2",
    "E1_1_3",
    "E1_1_4",
    "E1_1_5",
    "LE_LO_1",
    "LE_LO_2",
    "LE_LO_3",
    "LE_LO_4",
    "F2_0_1",
    "F2_1_1",
    "LE_SH_1",
    "LE_SH_2",
    "LE_SH_3",
    "LE_SH_4",
    "MINESAMB",
    "BASICTRK",
    "PARAPANT",
    "AE_OPTAMB",
    "AO_OPTAMB",
    "AE_OP_2_1",
    "AE_OP_2_2",
    "AE_OP_2_3",
    "AE_OP_3_1",
    "AE_OP_3_2",
    "AE_OP_4_1",
    "AE_PC_DEMO_OP_2_2",
    "AE_PC_DEMO_OP_2_3",
    "AE_PC_DEMO_OP_3_1",
    "AE_PC_DEMO_OP_3_2",
    "AE_PC_DEMO_OP_2_4",
    "AE_PC_DEMO_OP_2_5",
    "AE_PC_DEMO_OP_3_3",
    "AE_PC_DEMO_OP_3_4",
    "AE_PC_DEMO_OP_3_5",
    "Abe_Whistle1",
    "Abe_Whistle2",

    "Slig_Hi",
    "Slig_HereBoy",
    "Slig_GetEm",
    "Slig_Stay",
    "Slig_Bs",
    "Slig_LookOut",
    "Slig_SmoBs",
    "Slig_Laugh",
    "Slig_Freeze",
    "Slig_What",
    "Slig_Help",
    "Slig_Buhlur",
    "Slig_Gotcha",
    "Slig_Ow",
    "Slig_Urgh",

    "Glukkon_Commere",
    "Glukkon_AllYa",
    "Glukkon_DoIt",
    "Glukkon_Help",
    "Glukkon_Hey",
    "Glukkon_StayHere",
    "Glukkon_Laugh",
    "Glukkon_Hurg",
    "Glukkon_KillEm",
    "Glukkon_Step",
    "Glukkon_What",
};


static const char* kMusicEvents[] =
{
    "AMBIANCE",
    "BASE_LINE",
    "CRITTER_ATTACK",
    "CRITTER_PATROL",
    "SLIG_ATTACK",
    "SLIG_PATROL",
    "SLIG_POSSESSED",
};


void Sound::SoundBrowserUi()
{
    ImGui::Begin("Sound list");

    static const SoundResource* selected = nullptr;

    // left
    ImGui::BeginChild("left pane", ImVec2(200, 0), true);
    {
        for (const SoundResource& soundInfo : mLocator.mResMapper.mSoundResources.mSounds)
        {
            if (ImGui::Selectable(soundInfo.mResourceName.c_str()))
            {
                selected = &soundInfo;
            }
        }
        ImGui::EndChild();
    }
    ImGui::SameLine();

    // right
    ImGui::BeginGroup();
    {
        ImGui::BeginChild("item view");
        {
            if (selected)
            {
                ImGui::TextWrapped("Resource name: %s", selected->mResourceName.c_str());
                ImGui::Separator();
                ImGui::TextWrapped("Comment: %s", selected->mComment.empty() ? "(none)" : selected->mComment.c_str());
                ImGui::Separator();
                ImGui::TextWrapped("Is sound effect: %s", selected->mIsSoundEffect ? "true" : "false");

                const bool bHasMusic = !selected->mMusic.mSoundBanks.empty();
                const bool bHasSample = !selected->mSoundEffect.mSoundBanks.empty();
                if (bHasMusic)
                {
                    if (ImGui::CollapsingHeader("SEQs"))
                    {
                        for (const std::string& sb : selected->mMusic.mSoundBanks)
                        {
                            if (ImGui::Selectable(sb.c_str()))
                            {
                                mSeqPlayers.push_back(PlaySound(selected->mResourceName.c_str(), sb.c_str(), true, false));
                            }
                        }
                    }
                }

                if (bHasSample)
                {
                    if (ImGui::CollapsingHeader("Samples"))
                    {
                        for (const SoundEffectResourceLocation& sbLoc : selected->mSoundEffect.mSoundBanks)
                        {
                            for (const std::string& sb : sbLoc.mSoundBanks)
                            {
                                if (ImGui::Selectable(sb.c_str()))
                                {
                                    mSeqPlayers.push_back(PlaySound(selected->mResourceName.c_str(), sb.c_str(), false, true));
                                }
                            }
                        }
                    }
                }

                if (ImGui::Button("Play"))
                {
                    PlaySoundScript(selected->mResourceName.c_str());
                }
            }
            else
            {
                ImGui::TextWrapped("Click an item to display its info");
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    ImGui::End();

    ImGui::Begin("Sound themes");

    for (const MusicTheme& theme : mLocator.mResMapper.mSoundResources.mThemes)
    {
        if (ImGui::RadioButton(theme.mName.c_str(), mActiveTheme && theme.mName == mActiveTheme->mName))
        {
            SetTheme(theme.mName.c_str());
            Preload();
            HandleEvent("BASE_LINE");
        }
    }

    for (const char* eventName : kMusicEvents)
    {
        if (ImGui::Button(eventName))
        {
            HandleEvent(eventName);
        }
    }

    ImGui::End();

}

void Sound::Render(int /*w*/, int /*h*/)
{

}
