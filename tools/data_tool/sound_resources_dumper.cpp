#include "sound_resources_dumper.hpp"
#include "logger.hpp"
#include "../engine_hook/seq_name_algorithm.hpp"
#include "resourcemapper.hpp"

// AoPsxDemo
// BASE LINE
// RIDE ELUM
// SLIG PATROL
// SLIG ATTACK
// SLIG POSSESSED
// SLOG ANGRY
// SLOG ATTACK
// SLOG CLASH
// SLOG KILL

// AoPc
// BASE LINE
// RIDE ELUM
// SILENCE
// MYSTERY
// SLIG
// PATROL
// SLIG ATTACK
// SLIG POSSESSED
// SLOG ANGRY
// SLOG ATTACK
// SLOG CLASH
// SLOG KILL
// CRITTER PATROL
// CRITTER ATTACK
// NEGATIVE
// NEGATIVE2
// POSITIVE
// POSITIVE2

/*
// AoPsxDemo only
"ALL_2_1.SEQ",
"ALL_3_1.SEQ",
"ALL_6_1.SEQ",
"ALL_6_2.SEQ"

// AePcDemo only, completely unique names
"OP_2_4.SEQ",
"OP_2_5.SEQ",
"OP_3_3.SEQ",
"OP_3_4.SEQ",
"OP_3_5.SEQ",

// AePsxDemo only, Ao leftovers, but are they the same?
"ABEMOUNT.SEQ",
"BATSQUEK.SEQ",
"OPT_0_1.SEQ",
"OPT_0_2.SEQ",
"OPT_0_3.SEQ",
"OPT_0_4.SEQ",
"OPT_0_5.SEQ",
"OPT_1_1.SEQ",
"OPT_1_2.SEQ",
"OPT_1_3.SEQ",
"OPT_1_4.SEQ",
"OPT_1_5.SEQ",
"WHISTLE1.SEQ",
"WHISTLE2.SEQ"
*/

const char* kSeqAePc[] =
{
    #include "seq_rip/AePc.txt"
};

const char* kSeqAePcDemo[] =
{
    #include "seq_rip/AePcDemo.txt"
};

const char* kSeqAePsxCd1[] =
{
#include "seq_rip/AePsxCd1.txt"
};

const char* kSeqAePsxCd2[] =
{
#include "seq_rip/AePsxCd2.txt"
};

const char* kSeqAePsxDemo[] =
{
#include "seq_rip/AePsxDemo.txt"
};

const char* kSeqAoPc[] =
{
#include "seq_rip/AoPc.txt"
};

const char* kSeqAoPcDemo[] =
{
#include "seq_rip/AoPcDemo.txt"
};

const char* kSeqAoPsx[] =
{
#include "seq_rip/AoPsx.txt"
};

const char* kSeqAoPsxDemo[] =
{
#include "seq_rip/AoPsxDemo.txt"
};

#define array_size(x) sizeof(x) / sizeof(x[0])

std::map<eDataSetType, std::pair<const char**, size_t>> dataSetToSeqListMap =
{
    { eDataSetType::eAoPc, std::make_pair(kSeqAoPc, array_size(kSeqAoPc)) },
    { eDataSetType::eAoPcDemo, std::make_pair(kSeqAoPcDemo, array_size(kSeqAoPcDemo)) },
    { eDataSetType::eAoPsx, std::make_pair(kSeqAoPsx, array_size(kSeqAoPsx)) },
    { eDataSetType::eAoPsxDemo, std::make_pair(kSeqAoPsxDemo, array_size(kSeqAoPsxDemo)) },
    { eDataSetType::eAePc, std::make_pair(kSeqAePc, array_size(kSeqAePc)) },
    { eDataSetType::eAePcDemo, std::make_pair(kSeqAePcDemo, array_size(kSeqAePcDemo)) },
    { eDataSetType::eAePsxCd1, std::make_pair(kSeqAePsxCd1, array_size(kSeqAePsxCd1)) },
    { eDataSetType::eAePsxCd2, std::make_pair(kSeqAePsxCd2, array_size(kSeqAePsxCd2)) },
    { eDataSetType::eAePsxDemo, std::make_pair(kSeqAePsxDemo, array_size(kSeqAePsxDemo)) },
};

static std::map<eDataSetType, std::map<u32, std::string>> dataSetToSeqIdMap;


void SoundResourcesDumper::HandleLvl(eDataSetType eType, const std::string& /*lvlName*/, std::unique_ptr<Oddlib::LvlArchive> lvl)
{
    auto it = dataSetToSeqIdMap.find(eType);
    if (it == std::end(dataSetToSeqIdMap))
    {
        // Shouldn't be possible, die
        abort();
    }

    auto vhs = GetFilesOfType(*lvl, "VH");
    RemoveLoadingVhs(vhs);

    auto bsqs = GetFilesOfType(*lvl, "BSQ");
    for (auto& bsq : bsqs)
    {
        HandleBsq(eType, it->second, *bsq, vhs);
    }
}

template<class TOnPassword>
static bool DoBruteForce(char* str, int index, int maxDepth, TOnPassword cb)
{
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    static constexpr int kAlphabetSize = array_size(kAlphabet)-1;
    for (int i = 0; i < kAlphabetSize; ++i)
    {
        str[index] = kAlphabet[i];
        if (index == maxDepth - 1)
        {
            if (cb(str))
            {
                return true;
            }
        }
        else
        {
            if (DoBruteForce(str, index + 1, maxDepth, cb))
            {
                return true;
            }
        }
    }
    return false;
}

template<int maxLen, class TOnPassword>
static bool BruteForce(TOnPassword cb)
{
    char buf[maxLen + 1];
    for (int i = 1; i <= maxLen; ++i)
    {
        memset(buf, 0, maxLen + 1);
        if (DoBruteForce(buf, 0, i, cb))
        {
            return false;
        }
    }
    return false;
}

/*
Out of the tons of hash collisions one of these seems like the most likely original name:
MI_7_10
MI_7_11
MI_7_12
MI_7_13
MI_7_14
MI_7_15
MI_7_16
MI_7_17
MI_7_18
MI_7_19

"MI_10_1.SEQ",
"MI_1_1.SEQ",
"MI_2_1.SEQ",
"MI_3_1.SEQ",
"MI_4_1.SEQ",
"MI_5_1.SEQ",
"MI_6_1.SEQ",
"MI_8_1.SEQ",
"MI_9_1.SEQ",
*/
void BruteForceUnknownSeqName(std::ofstream& s)
{
    BruteForce<8>([&](const char* password)
    {
        if (SeqResourceNameHash(password) == 0x000b3a07)
        {
            s << password << std::endl;
            LOG_INFO("MATCH:" << password);
            return false;
        }
        return false;
    });
}

static void InitGSounds();

SoundResourcesDumper::SoundResourcesDumper(ResourceLocator& locator)
    : mLocator(locator)
{
    // std::ofstream matches("matches.txt", std::ios::out);
    // BruteForceUnknownSeqName(matches);
    CompileSeqIds();
    InitGSounds();
}

void SoundResourcesDumper::CompileSeqIds()
{
    for (auto& item : dataSetToSeqListMap)
    {
        std::map<u32, std::string> seqNames;
        const size_t arrayLen = item.second.second;
        for (size_t i = 0; i < arrayLen; i++)
        {
            seqNames[SeqResourceNameHash(item.second.first[i])] = item.second.first[i];
        }

        dataSetToSeqIdMap[item.first] = seqNames;
    }
}

std::string SoundResourcesDumper::MatchIdAnyWhere(u32 id)
{
    for (auto& item : dataSetToSeqIdMap)
    {
        auto it = item.second.find(id);
        if (it != std::end(item.second))
        {
            return it->second;
        }
    }
    return "";
}

const std::map<std::string, std::string> bsqToVhMap =
{
    { "BASEQ.BSQ", "BARRACKS.VH" },
    { "B2SEQ.BSQ", "BAENDER.VH" },

    { "BWSEQ.BSQ", "BONEWERK.VH" },
    { "B3SEQ.BSQ", "BWENDER.VH" },

    { "FDSEQ.BSQ", "FEECO.VH" },
    { "FESEQ.BSQ", "FEENDER.VH" },

    { "PVSEQ.BSQ", "PARVAULT.VH" },
    { "PESEQ.BSQ", "PVENDER.VH" },

    { "SVSEQ.BSQ", "SCRVAULT.VH" },
    { "SESEQ.BSQ", "SVENDER.VH" },

    // Strange case in Ae demo where both are in the same LVL
    { "STSEQ.BSQ", "OPTION.VH" },
    { "MISEQ.BSQ", "MINES.VH" },

};

// Seqs that use these sound banks with the same ids are NOT the same seq, so ensure
// they are kept unique instead of being de-duplicated.
const std::set<std::string> uniqueSoundBanks =
{
    "E1SNDFX",
    "E2SNDFX",
    "D2ENDER",
    "D2SNDFX",
    "F2ENDER",
    "F2SNDFX",
    "RFSNDFX", // These might be the same since the same file names are in R1 and R2 LVLs
    "RFENDER",
    "OPTSNDFX" // TODO: These might not be the same but still get de-duped due the the vab being the same name
};

void SoundResourcesDumper::HandleBsq(eDataSetType eType, const std::map<u32, std::string>& seqNames, const Oddlib::LvlArchive::File& bsq, std::vector<const Oddlib::LvlArchive::File*>& vhs)
{
    for (u32 i = 0; i < bsq.ChunkCount(); i++)
    {
        const Oddlib::LvlArchive::FileChunk* seq = bsq.ChunkByIndex(i);

        auto it = seqNames.find(seq->Id());
        if (it != std::end(seqNames))
        {
            TempMusicResource music;
            music.mResourceId = seq->Id();
            music.mResourceName = /*std::string(ToString(eType)) + "_" +*/ string_util::split(it->second, '.')[0];
         
            // Need to choose the correct/matching VAB out of vhs
            std::string vh = vhs[0]->FileName();
            if (vhs.size() > 1)
            {
                auto vhIt = bsqToVhMap.find(bsq.FileName());
                if (vhIt == std::end(bsqToVhMap))
                {
                    abort();
                }
                vh = vhIt->second;
            }

            const std::string soundBankBaseName = string_util::split(vh, '.')[0];
            std::string soundBankNameResourceName = std::string(ToString(eType)) + "_" + string_util::split(bsq.FileName(), '.')[0] + "_" + soundBankBaseName;

            /*            
            auto sbIt = uniqueSoundBanks.find(soundBankBaseName);
            if (sbIt != std::end(uniqueSoundBanks))
            {
                music.mResourceName += "_" + soundBankBaseName;
            }
            */

            SoundBankLocation location;
            location.mDataSetName = ToString(eType);
            location.mName = soundBankNameResourceName;
            location.mSeqFileName = bsq.FileName();
            location.mSoundBankName = string_util::split(vh, '.')[0];

            TempMusicResource* pExisting = Exists(mTempResources.mMusics, music);
            if (pExisting)
            {
                pExisting->mSoundBanks.insert(soundBankNameResourceName);
            }
            else
            {
                music.mSoundBanks.insert(soundBankNameResourceName);
                mTempResources.mMusics.push_back(music);
            }

            if (!Exists(mFinalResources.mSoundBanks, location))
            {
                mFinalResources.mSoundBanks.push_back(location);
            }
        }
        else
        {
            // An unknown and thus probably un-used/beta sequence?
            HandleUnknownSeq(eType, *seq);
        }
    }
}

void SoundResourcesDumper::HandleUnknownSeq(eDataSetType eType, const Oddlib::LvlArchive::FileChunk& seq)
{
    // Check if its completely unknown
    std::string match = MatchIdAnyWhere(seq.Id());
    if (match.empty())
    {
        if (seq.Id() == 0x000b3a07)
        {
            // An unknown SEQ in AePcDemo, this is the only completely unknown SEQ in all data sets
        }
        else
        {
            abort();
        }
    }
    else
    {
        // WHISTLE1/2 and EBELL still exist in some AE data sets
        LOG_INFO("Unused but known SEQ: " << match << " in " << ToString(eType));
    }
}

void SoundResourcesDumper::RemoveLoadingVhs(std::vector<const Oddlib::LvlArchive::File*>& vhs)
{
    for (auto it = vhs.begin(); it != vhs.end(); )
    {
        if ((*it)->FileName() == "SPEAKIN.VH"
            || (*it)->FileName() == "SPEAKOUT.VH"
            || (*it)->FileName() == "MONK.VH")
        {
            it = vhs.erase(it);
        }
        else
        {
            it++;
        }
    }
}

TempMusicResource* SoundResourcesDumper::Exists(std::vector<TempMusicResource>& all, const TempMusicResource& res)
{
    for (auto& r : all)
    {
        if (r.mResourceId == res.mResourceId &&
            r.mResourceName == res.mResourceName)
        {
            return &r;
        }
    }
    return nullptr;
}

bool SoundResourcesDumper::Exists(const std::vector<SoundBankLocation>& all, const SoundBankLocation& loc)
{
    for (auto& r : all)
    {
        if (r.mDataSetName == loc.mDataSetName &&
            r.mName == loc.mName &&
            r.mSeqFileName == loc.mSeqFileName &&
            r.mSoundBankName == loc.mSoundBankName)
        {
            return true;
        }
    }
    return false;
}

static std::map<std::string, std::set<std::string>> gBrokenSeqs
{
    {
        "MUDOHM",
        {
            "AePcDemo_MISEQ_MINES"
        }
    },

    { 
        "OHM",
        { 
            "AoPsxDemo_E1SEQ_E1SNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsxDemo_S1SEQ_OPTSNDFX"
        } 
    },

    {
        "STOPIT",
        {
            "AePsxDemo_STSEQ_OPTSNDFX"
        }
    },

    {
        "SSCRATCH",
        {
            "AePc_NESEQ_NECRUM",
            "AePsxCd1_NESEQ_NECRUM",
            "AePsxCd2_NESEQ_NECRUM",
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_F1SEQ_F1SNDFX",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_L1SEQ_MLSNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsx_NESEQ_NECRUM",
            "AoPsx_F1SEQ_F1SNDFX",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_L1SEQ_MLSNDFX",
            "AoPsx_D2SEQ_D2ENDER"
        }
    },

    {
        "SLIGBOMB",
        {
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_D2SEQ_D2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },

    {
        "SLIGBOM2",
        {
            "AoPsx_D2SEQ_D2ENDER",
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },

    {
        "PATROL",
        {
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_D2SEQ_D2ENDER",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPc_D1SEQ_D1SNDFX",
            "AoPc_D2SEQ_D2SNDFX",
            "AoPc_E1SEQ_E1SNDFX",
            "AoPc_F2SEQ_F2SNDFX",
            "AoPsxDemo_E1SEQ_E1SNDFX",
            "AoPsx_D1SEQ_D1SNDFX",
            "AoPsx_D2SEQ_D2SNDFX",
            "AoPsx_E1SEQ_E1SNDFX",
            "AoPsx_F2SEQ_F2SNDFX"
        }
    },

    {
        "NEGATIV1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },

    {
        "NEGATIV3",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },

    {
        "POSITIV1",
        {
            "AePc_BMSEQ_BRENDER",
            "AePc_BRSEQ_BREWERY",
            "AePsxCd1_BMSEQ_BRENDER",
            "AePsxCd2_BRSEQ_BREWERY",
            "AePsxCd1_BRSEQ_BREWERY",
            "AePsxCd2_BMSEQ_BRENDER"
        }
    },

    {
        "POSITIV9",
        {
            "AePc_BMSEQ_BRENDER",
            "AePc_BRSEQ_BREWERY",
            "AePsxCd1_BRSEQ_BREWERY",
            "AePsxCd2_BMSEQ_BRENDER",
            "AePsxCd2_BRSEQ_BREWERY"
        }
    },

    {
        "PIGEONS",
        {
            "AePcDemo_MISEQ_MINES",
            "AePsxDemo_MISEQ_MINES",
            "AoPcDemo_R1SEQ_RFSNDFX",
            "AoPc_R1SEQ_DFSNDFX",
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_D2SEQ_D2SNDFX",
            "AoPc_E1SEQ_E1SNDFX",
            "AoPc_E2SEQ_E2SNDFX",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_F2SEQ_F2SNDFX",
            "AoPc_R1SEQ_RFSNDFX",
            "AoPc_R2SEQ_RFSNDFX",
            "AoPc_R6SEQ_RFENDER",
            "AoPsxDemo_E1SEQ_E1SNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsx_D2SEQ_D2ENDER",
            "AoPsx_D2SEQ_D2SNDFX",
            "AoPsx_E1SEQ_E1SNDFX",
            "AoPsx_E2SEQ_E2SNDFX",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2SNDFX",
            "AoPsx_R1SEQ_RFSNDFX",
            "AoPsx_R2SEQ_RFSNDFX",
            "AoPsx_R6SEQ_RFENDER"
        }
    },

    {
        "PARAPANT",
        {
            "AoPcDemo_R1SEQ_RFSNDFX",
            "AoPc_L1SEQ_MLSNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsx_L1SEQ_MLSNDFX",
            "AoPsx_R1SEQ_RFSNDFX",
            "AoPsx_R2SEQ_RFSNDFX",
            "AoPc_R1SEQ_RFSNDFX",
            "AoPc_R2SEQ_RFSNDFX",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2SNDFX"
        }
    },

    {
        "BATSQUEK",
        {
            "AoPcDemo_R1SEQ_RFSNDFX",
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_L1SEQ_MLENDER",
            "AoPc_R1SEQ_RFSNDFX",
            "AoPc_R2SEQ_RFSNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsx_D2SEQ_D2ENDER",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_L1SEQ_MLENDER",
            "AoPsx_R1SEQ_RFSNDFX",
            "AoPsx_R2SEQ_RFSNDFX",
            "AoPc_L1SEQ_MLSNDFX",
            "AoPsx_L1SEQ_MLSNDFX",
        }
    },

    {
        "EBELL2",
        {
            "AoPcDemo_R1SEQ_RFSNDFX",
            "AoPc_R1SEQ_RFSNDFX",
            "AoPc_R2SEQ_RFSNDFX",
            "AoPc_R6SEQ_RFENDER",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsx_R1SEQ_RFSNDFX",
            "AoPsx_R2SEQ_RFSNDFX",
            "AoPsx_R6SEQ_RFENDER"
        }
    },

    {
        "ESCRATCH",
        {
            "AoPcDemo_R1SEQ_RFSNDFX",
            "AoPc_R1SEQ_RFSNDFX",
            "AoPc_R2SEQ_RFSNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsx_R1SEQ_RFSNDFX",
            "AoPsx_R2SEQ_RFSNDFX",
            "AoPsx_R2SEQ_RFSNDFX"
        }
    },

    {
        "SLEEPING",
        {
            "AoPc_D1SEQ_D1SNDFX",
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_D2SEQ_D2SNDFX",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_F2SEQ_F2SNDFX",
            "AoPsx_D1SEQ_D1SNDFX",
            "AoPsx_D2SEQ_D2ENDER",
            "AoPsx_D2SEQ_D2SNDFX",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2SNDFX"

        }
    },

    {
        "ONCHAIN",
        {
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_L1SEQ_MLSNDFX",
            "AoPsxDemo_E1SEQ_E1SNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsx_D2SEQ_D2ENDER",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_L1SEQ_MLSNDFX",
            "AoPsxDemo_E1SEQ_E1SNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
        }
    },
    {
        "OP_2_1",
        {
            "AePcDemo_STSEQ_OPTION"
        }
    },
    {
        "FE_2_1",
        {
            "AoPc_F2SEQ_F2SNDFX",
            "AoPsx_F2SEQ_F2SNDFX"
        }
    },
    {
        "FE_4_1",
        {
            "AoPc_F2SEQ_F2SNDFX",
            "AoPsx_F2SEQ_F2SNDFX"
        }
    },
    {
        "FE_5_1",
        {
            "AoPc_F2SEQ_F2SNDFX",
            "AoPsx_F2SEQ_F2SNDFX"
        }
    },
    {
        "MI_2_1",
        {
            "AePcDemo_MISEQ_MINES",
            "AePsxDemo_MISEQ_MINES"
        }
    },
    {
        "MI_3_1",
        {
            "AePcDemo_MISEQ_MINES",
            "AePsxDemo_MISEQ_MINES"
        }
    },
    {
        "MI_5_1",
        {
            "AePcDemo_MISEQ_MINES",
            "AePsxDemo_MISEQ_MINES"
        }
    },
    {
        "F1AMB",
        {
            "AoPcDemo_C1SEQ_OPTSNDFX",
            "AoPcDemo_S1SEQ_OPTSNDFX",
            "AoPc_C1SEQ_OPTSNDFX",
            "AoPc_S1SEQ_OPTSNDFX",
            "AoPsxDemo_C1SEQ_OPTSNDFX",
            "AoPsxDemo_S1SEQ_OPTSNDFX",
            "AoPsx_C1SEQ_OPTSNDFX",
            "AoPsx_S1SEQ_OPTSNDFX",
        }
    },
    {
        "SLOSLEEP",
        {
            "AoPcDemo_R1SEQ_RFSNDFX",
            "AoPc_D1SEQ_D1SNDFX",
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_E1SEQ_E1SNDFX",
            "AoPc_E2SEQ_E2SNDFX",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_L1SEQ_MLSNDFX",
            "AoPc_R1SEQ_RFSNDFX",
            "AoPc_R2SEQ_RFSNDFX",
            "AoPsx_D2SEQ_D2ENDER",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_L1SEQ_MLSNDFX",
        }
    },
    {
        "PANTING",
        {
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_L1SEQ_MLSNDFX",
            "AoPsxDemo_E1SEQ_E1SNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsx_D2SEQ_D2ENDER",
            "AoPsx_F2SEQ_F2ENDER",
            "AoPsx_L1SEQ_MLSNDFX",
        }
    },
    {
        "PVEND_7",
        {
            "AePsxCd2_PESEQ_PARVAULT",
        }
    },
    {
        "PVEND_8",
        {
            "AePsxCd2_PESEQ_PARVAULT",
        }
    },
    {
        "PVEND_9",
        {
            "AePsxCd2_PESEQ_PARVAULT",
        }
    },
    {
        "D2_2_1",
        {
            "AoPc_D2SEQ_D2SNDFX",
            "AoPsx_D2SEQ_D2SNDFX"
        }
    },
    {
        "D2_2_2",
        {
            "AoPc_D2SEQ_D2SNDFX",
            "AoPsx_D2SEQ_D2SNDFX"
        }
        
    },
    {
        "DE_2_1",
        {
            "AoPc_D2SEQ_D2SNDFX",
            "AoPsx_D2SEQ_D2SNDFX"
        }
    },
    {
        "DE_4_1",
        {
            "AoPc_D2SEQ_D2SNDFX",
            "AoPsx_D2SEQ_D2SNDFX"
        }
    },
    {
        "DE_5_1",
        {
            "AoPc_D2SEQ_D2SNDFX",
            "AoPsx_D2SEQ_D2SNDFX"
        }
    },
    {
        "LE_LO_1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "LE_LO_2",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "LE_LO_3",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "LE_LO_4",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },



    {
        "F2_2_1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "F2_4_1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "F2_5_1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "F2_6_1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "F2_1_1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "F2_0_1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "LE_SH_1",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "LE_SH_2",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "LE_SH_3",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "LE_SH_4",
        {
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        "MINESAMB",
        {
            "AePcDemo_STSEQ_OPTION",
            "AePsxDemo_STSEQ_OPTION",
            "AePsxDemo_STSEQ_OPTSNDFX"
        }
    },
    {
        "BASICTRK",
        {
            // Might be wrong, but hey ho they sound close enough
            "AoPc_F2SEQ_F2ENDER",
            "AoPsx_F2SEQ_F2ENDER"
        }
    },
    {
        // RFENDER - completely removes this
        "RE_2_1",
        {
            "AoPc_R6SEQ_RFENDER",
        }
    },
    {
        // RFENDER - completely removes this
        "RE_4_1",
        {
            "AoPc_R6SEQ_RFENDER",
        }
    },
    {
        // RFENDER - completely removes this
        "RE_5_1",
        {
            "AoPc_R6SEQ_RFENDER",
        }
    },
    {
        "WHISTLE1",
        {
            "AePsxDemo_MISEQ_MINES",
            "AePsxDemo_STSEQ_OPTION",
            "AoPcDemo_R1SEQ_RFSNDFX",
            "AoPcDemo_S1SEQ_OPTSNDFX",
            "AoPc_D1SEQ_D1SNDFX",
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_D2SEQ_D2SNDFX",
            "AoPc_E1SEQ_E1SNDFX",
            "AoPc_E2SEQ_E2SNDFX",
            "AoPc_F1SEQ_F1SNDFX",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_F2SEQ_F2SNDFX",
            "AoPc_L1SEQ_MLSNDFX",
            "AoPc_R1SEQ_RFSNDFX",
            "AoPc_R2SEQ_RFSNDFX",
            "AoPc_R6SEQ_RFENDER",
            "AoPc_S1SEQ_OPTSNDFX",
            "AoPsxDemo_E1SEQ_E1SNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsxDemo_S1SEQ_OPTSNDFX",
        }
    },
    {
        "WHISTLE2",
        {
            "AePsxDemo_MISEQ_MINES",
            "AePsxDemo_STSEQ_OPTION",
            "AoPcDemo_R1SEQ_RFSNDFX",
            "AoPcDemo_S1SEQ_OPTSNDFX",
            "AoPc_D1SEQ_D1SNDFX",
            "AoPc_D2SEQ_D2ENDER",
            "AoPc_D2SEQ_D2SNDFX",
            "AoPc_E1SEQ_E1SNDFX",
            "AoPc_E2SEQ_E2SNDFX",
            "AoPc_F1SEQ_F1SNDFX",
            "AoPc_F2SEQ_F2ENDER",
            "AoPc_F2SEQ_F2SNDFX",
            "AoPc_L1SEQ_MLSNDFX",
            "AoPc_R1SEQ_RFSNDFX",
            "AoPc_R2SEQ_RFSNDFX",
            "AoPc_R6SEQ_RFENDER",
            "AoPc_S1SEQ_OPTSNDFX",
            "AoPsxDemo_E1SEQ_E1SNDFX",
            "AoPsxDemo_R1SEQ_RFSNDFX",
            "AoPsxDemo_R2SEQ_RFSNDFX",
            "AoPsxDemo_S1SEQ_OPTSNDFX",
        }
    },
};

// Remove sound
void SoundResourcesDumper::RemoveBadSoundBanks()
{
    for (auto& toRemove : gBrokenSeqs)
    {
        for (auto& music : mTempResources.mMusics)
        {
            if (music.mResourceName == toRemove.first)
            {
                for (auto sbIt = music.mSoundBanks.begin(); sbIt != music.mSoundBanks.end();)
                {
                    bool removed = false;
                    for (auto& sbToRemove : toRemove.second)
                    {
                        if (*sbIt == sbToRemove)
                        {
                            sbIt = music.mSoundBanks.erase(sbIt);
                            removed = true;
                            break;
                        }
                    }
                    if (!removed)
                    {
                        sbIt++;
                    }
                }
            }
        }
    }
}

struct SplitSeqData
{
    std::string mNewName;
    std::set<std::string> mVabs;
};

static std::map<std::string, SplitSeqData> gSplitSeqs
{
    {
        "OPTAMB",
        {
            "AE_OPTAMB",
            {
                "AePcDemo_STSEQ_OPTION",
                "AePc_STSEQ_OPTION",
                "AePsxCd1_STSEQ_OPTION",
                "AePsxCd2_STSEQ_OPTION"
            }
        }
    },
    {
        "OP_2_2",
        {
            "AE_PC_DEMO_OP_2_2",
            {
                "AePcDemo_STSEQ_OPTION"
            }
        }
    },
    {
        "OP_2_3",
        {
            "AE_PC_DEMO_OP_2_3",
            {
                "AePcDemo_STSEQ_OPTION"
            }
        }
    },
    {
        "FE_2_1",
        {
            "AE_FE_2_1",
            {
                "AePc_FDSEQ_FEECO",
                "AePsxCd1_FDSEQ_FEECO",
                "AePsxCd2_FDSEQ_FEECO"
            }
        }
    },
    {
        "OP_3_1",
        {
            "AE_PC_DEMO_OP_3_1",
            {
                "AePcDemo_STSEQ_OPTION",
            }
        }
    },
    {
        "OP_3_2",
        {
            "AE_PC_DEMO_OP_3_2",
            {
                "AePcDemo_STSEQ_OPTION",
            }
        }
    },
    {
        "FE_4_1",
        {
            "AO_FE_4_1",
            {
                "AoPc_F2SEQ_F2ENDER",
                "AoPsx_F2SEQ_F2ENDER"
            }
        }
    },
    {
        "FE_5_1",
        {
            "AO_FE_5_1",
            {
                "AoPc_F2SEQ_F2ENDER",
                "AoPsx_F2SEQ_F2ENDER"
            }
        }
    },
    {
        "ALL_4_1",
        {
            "AO_PSX_DEMO_ALL_4_1",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX"
            }
        }
    },
    {
        "ALL_5_1",
        {
            "AO_PSX_DEMO_ALL_5_1",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX"
            }
        }
    },
    {
        "ALL_5_2",
        {
            "AO_PSX_DEMO_ALL_5_2",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX"
            }
        }
    },
    {
        "ALL_5_3",
        {
            "AO_PSX_DEMO_ALL_5_3",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX"
            }
        }
    },
    {
        "ALL_7_1",
        {
            "AO_PSX_DEMO_ALL_7_1",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX"
            }
        }
    },
    {
        "ALL_8_1",
        {
            "AO_PSX_DEMO_ALL_8_1",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX"
            }
        }
    },
    {
        "E1AMB",
        {
            "AO_PSX_DEMO_E1AMB",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX"
            }
        }
    },
    {
        "RF_5_1",
        {
            "AO_PSX_DEMO_RF_5_1",
            {
                "AoPsxDemo_R1SEQ_RFSNDFX",
                "AoPsxDemo_R2SEQ_RFSNDFX"
            }
        }
    },
    {
        "RF_6_1",
        {
            "AO_PSX_DEMO_RF_6_1",
            {
                "AoPsxDemo_R1SEQ_RFSNDFX",
                "AoPsxDemo_R2SEQ_RFSNDFX"
            }
        }
    },
    {
        "E1_0_1",
        {
            "AO_PSX_DEMO_E1_0_1",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_0_2",
        {
            "AO_PSX_DEMO_E1_0_2",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_0_3",
        {
            "AO_PSX_DEMO_E1_0_3",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_0_4",
        {
            "AO_PSX_DEMO_E1_0_4",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_0_5",
        {
            "AO_PSX_DEMO_E1_0_5",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_1_1",
        {
            "AO_PSX_DEMO_E1_1_1",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_1_2",
        {
            "AO_PSX_DEMO_E1_1_2",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_1_3",
        {
            "AO_PSX_DEMO_E1_1_3",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_1_4",
        {
            "AO_PSX_DEMO_E1_1_4",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
    {
        "E1_1_5",
        {
            "AO_PSX_DEMO_E1_1_5",
            {
                "AoPsxDemo_E1SEQ_E1SNDFX",
            }
        }
    },
};

static std::map<std::string, std::string> gSeqRenames =
{
    { "OPTAMB", "AO_OPTAMB" },
    { "OP_2_1", "AE_OP_2_1" },
    { "OP_2_2", "AE_OP_2_2" },
    { "OP_2_3", "AE_OP_2_3" },
    { "OP_3_1", "AE_OP_3_1" },
    { "OP_3_2", "AE_OP_3_2" },
    { "OP_4_1", "AE_OP_4_1" },
    { "FE_1_1", "AE_FE_1_1" },
    { "FE_3_1", "AE_FE_3_1" },
    { "FE_2_1", "AO_FE_2_1" },
    { "FE_4_1", "AE_FE_4_1" },
    { "FE_5_1", "AE_FE_5_1" },
    { "FE_6_1", "AE_FE_6_1" },
    { "FE_8_1", "AE_FE_8_1" },
    { "FE_9_1", "AE_FE_9_1" },
    { "FE_10_1", "AE_FE_10_1" },
    { "ABEMOUNT", "AO_PSX_DEMO_ABEMOUNT" },
    { "OP_2_4", "AE_PC_DEMO_OP_2_4" },
    { "OP_2_5", "AE_PC_DEMO_OP_2_5" },
    { "OP_3_3", "AE_PC_DEMO_OP_3_3" },
    { "OP_3_4", "AE_PC_DEMO_OP_3_4" },
    { "OP_3_5", "AE_PC_DEMO_OP_3_5" },
    { "ALL_2_1", "AO_PSX_DEMO_ALL_2_1" },
    { "ALL_3_1", "AO_PSX_DEMO_ALL_3_1" },
    { "ALL_6_1", "AO_PSX_DEMO_ALL_6_1" },
    { "ALL_6_2", "AO_PSX_DEMO_ALL_6_2" },

};

void SoundResourcesDumper::SplitSEQs()
{
    for (auto& toSplit : gSplitSeqs)
    {
        for (auto musicIt = mTempResources.mMusics.begin(); musicIt != mTempResources.mMusics.end(); )
        {
            // Find the music record that matches the one we want to split
            bool erased = false;
            if (musicIt->mResourceName == toSplit.first)
            {
                // Create the split record
                TempMusicResource splitRecord;
                splitRecord.mResourceName = toSplit.second.mNewName;
                splitRecord.mResourceId = musicIt->mResourceId;

                TempMusicResource* existing = Exists(mTempResources.mMusics, splitRecord);

                // Iterate the vabs to split out into the new record
                for (auto& sbToSplit : toSplit.second.mVabs)
                {
                    // Remove from existing SEQ
                    musicIt->mSoundBanks.erase(sbToSplit);

                    // Added to split out SEQ
                    if (existing)
                    {
                        existing->mSoundBanks.insert(sbToSplit);
                    }
                    else
                    {
                        splitRecord.mSoundBanks.insert(sbToSplit);
                    }
                }

                if (!existing)
                {
                    erased = true;
                    mTempResources.mMusics.push_back(splitRecord);
                    musicIt = mTempResources.mMusics.begin();
                }

            }

            if (!erased)
            {
                musicIt++;
            }
        }
    }
}

void SoundResourcesDumper::RenameSEQs()
{
    for (auto& toRename : gSeqRenames)
    {
        for (auto musicIt = mTempResources.mMusics.begin(); musicIt != mTempResources.mMusics.end(); musicIt++)
        {
            if (musicIt->mResourceName == toRename.first)
            {
                musicIt->mResourceName = toRename.second;
            }
        }
    }
}


// Mostly ripped from the game by hooking the sound playing functions to
// find this table in the real game
static std::vector<TempSoundEffectResource> gSounds;

static void InitGSounds()
{
    gSounds.clear();

    // TODO: Add a comment field as it won't be obvious from just as a name what some sound effects actually are

    // Slig
    gSounds.emplace_back(TempSoundEffectResource("Slig_Hi", 127, 0, 0,
    {
        //{ 62, 60, { "Ao" } },
        { 25, 60, { "AePc" } }
    }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_HereBoy", 127, 0, 0, { { 25, 62,{ "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_GetEm", 127, 0, 0,{ { 25, 61, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Stay", 127, 0, 0, { { 25, 63, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Bs", 127, 0, 0, { { 25, 66, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_LookOut", 127, 0, 0,{ { 25, 37, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_SmoBs", 127, 0, 0, { { 25, 67, { "AePc"} } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Laugh", 127, 0, 0, { { 25, 38, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Freeze", 127, 0, 0, { { 25, 57, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_What", 127, 0, 0, { { 25, 58, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Help", 127, 0, 0, { { 25, 59, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Buhlur", 127, 0, 0, { { 25, 39, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Gotcha",  127, 0, 0, { { 25, 64, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Ow", 127, 0, 0, { { 25, 65, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Slig_Urgh", 127, 0, 0,{ { 25, 68, { "AePc" } } }));

    // Glukkon
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_Commere", 127, 0, 0,{ { 8, 61, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_AllYa", 127, 0, 0,{ { 8, 63, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_DoIt", 127, 0, 0,{ { 8, 64, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_Help", 127, 0, 0,{ { 8, 65, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_Hey", 127, 0, 0,{ { 8, 66, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_StayHere", 127, 0, 0,{ { 8, 67, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_Laugh", 127, 0, 0,{ { 8, 69, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_Hurg", 127, 0, 0,{ { 8, 70, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_KillEm", 127, 0, 0,{ { 8, 71, { "AePc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_Step", 127, 0, 0,{ { 8, 36, { "AePc" } } })); // TODO: Pitch rand?
    gSounds.emplace_back(TempSoundEffectResource("Glukkon_What", 127, 0, 0,{ { 8, 62, { "AePc" } } }));
    
    // Whistles - must match SEQ name
    // TODO: Add SEQ to rename list so it can be Abe_Whistle1/2 etc
    gSounds.emplace_back(TempSoundEffectResource("WHISTLE1", 127, 0, 0, { { 58, 60,{ "AoPc" } } }));
    gSounds.emplace_back(TempSoundEffectResource("WHISTLE2", 127, 0, 0, { { 58, 61,{ "AoPc" } } }));

    // TODO: Add to new format as above with similar naming convention, also rename in abe.nut where any are currently used
    /*
    {
    "sound_effects": [
    {
        "data_set": "AoPc",
        "note": 53,
        "program" : 6,
        "resource_name": "ELUM_BELL",
        "sound_bank": "f1_F1SNDFX_AoPc"
    },
    {
        "data_set": "AoPc",
        "note": 36,
        "program" : 4,
        "resource_name": "OPTION_SELECT",
        "sound_bank": "s1_OPTSNDFX_AoPc"
    },
    {
        "data_set": "AePc",
        "note": 60,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_HELLO",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 62,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_ANGRY",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 61,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_FOLLOWME",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 63,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_WAIT",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 66,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_FART",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 67,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_GIGGLE",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 58,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_UHUH",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 56,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_HURT",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 57,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_LAUGH",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 59,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_OKAY",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 65,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_AHHHH",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 64,
        "program": 24,
        "resource_name": "GAMESPEAK_MUD_LAND",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 60,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_ALLYA",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 68,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_HI_ANGRY",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 69,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_HI_HAPPY",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 70,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_HI_SAD",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 72,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_NO_ANGRY",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 73,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_NO_SAD",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 58,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_PHOOF",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 64,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_SICK",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 66,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_WORK",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 62,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_SIGH",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 63,
        "program": 23,
        "resource_name": "GAMESPEAK_MUD_SORRY",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 60,
        "program": 4,
        "resource_name": "GAMESPEAK_SCRAB_SCREACH",
        "sound_bank": "sv_SCRVAULT_AePc"
    },
    {
        "data_set": "AePc",
        "note": 68,
        "program": 4,
        "resource_name": "GAMESPEAK_SCRAB_ROAR",
        "sound_bank": "sv_SCRVAULT_AePc"
    },
    {
        "data_set": "AePc",
        "note": 61,
        "program": 4,
        "resource_name": "ACTION_SCRAB_DEATH",
        "sound_bank": "sv_SCRVAULT_AePc"
    },
    {
        "data_set": "AePc",
        "note": 65,
        "program": 4,
        "resource_name": "ACTION_SCRAB_STOMP",
        "sound_bank": "sv_SCRVAULT_AePc"
    },
    {
        "data_set": "AePc",
        "note": 72,
        "program": 0,
        "resource_name": "FX_UXB_BEEP_GREEN",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 73,
        "program": 0,
        "resource_name": "FX_UXB_BEEP_RED",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 70,
        "program": 0,
        "resource_name": "FX_LED_SCREEN",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 57,
        "program": 1,
        "resource_name": "FX_POP",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 64,
        "program": 2,
        "resource_name": "FX_SHELL",
        "sound_bank": "mi_MINES_AePc",
        "pitch_range" : [ -2, 2 ]
    },
    {
        "data_set": "AePc",
        "note": 36,
        "program": 2,
        "resource_name": "FX_ZAP",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 37,
        "program": 2,
        "resource_name": "FX_ZAP_DISTANT",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 38,
        "program": 2,
        "resource_name": "FX_ZAP_LIGHT",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 48,
        "program": 3,
        "resource_name": "FX_MINE_PICK",
        "sound_bank": "mi_MINES_AePc",
        "pitch_range" : [ -1, 1 ]
    },
    {
        "data_set": "AePc",
        "note": 65,
        "program": 4,
        "resource_name": "FX_FLESH_LOW",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 58,
        "program": 4,
        "resource_name": "FX_FLESH_HIGH",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 58,
        "program": 5,
        "resource_name": "FX_EAT_FLESH1",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 59,
        "program": 5,
        "resource_name": "FX_EAT_FLESH2",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 64,
        "program": 7,
        "resource_name": "FX_BIRDS_FLY",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 60,
        "program": 7,
        "resource_name": "FX_MINECART_LOOP",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 61,
        "program": 7,
        "resource_name": "FX_MINECART_STOP",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 40,
        "program": 9,
        "resource_name": "FX_ROCK_COLLISION",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 61,
        "program": 9,
        "resource_name": "FX_ROCKBAG_SWING",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 57,
        "program": 9,
        "resource_name": "FX_WELL_ENTER",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 58,
        "program": 9,
        "resource_name": "FX_WELL_EXIT",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 62,
        "program": 9,
        "resource_name": "FX_SPAWN",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 61,
        "program": 10,
        "resource_name": "FX_LIFT_MOVE",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 60,
        "program": 10,
        "resource_name": "FX_LIFT_DOCK",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 48,
        "program": 10,
        "resource_name": "FX_PLATFORM_TOGGLE",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 65,
        "program": 10,
        "resource_name": "FX_PLATFORM_TOGGLE2",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 62,
        "program": 10,
        "resource_name": "FX_ROPE_PULL",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 63,
        "program": 10,
        "resource_name": "FX_LEVER",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 64,
        "program": 10,
        "resource_name": "FX_DOOR",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 59,
        "program": 10,
        "resource_name": "FX_BUTTON",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 66,
        "program": 10,
        "resource_name": "FX_GAS?",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 35,
        "program": 10,
        "resource_name": "FX_TRANSITION",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 40,
        "program": 10,
        "resource_name": "FX_MINER_IDLE",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 42,
        "program": 10,
        "resource_name": "FX_MINER_CUT",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 60,
        "program": 2,
        "resource_name": "ACTION_SLIG_SHOOT",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 37,
        "program": 9,
        "resource_name": "ACTION_THROW",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 60,
        "program": 9,
        "resource_name": "ACTION_PICKUP",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 58,
        "program": 1,
        "resource_name": "MOVEMENT_SLIG_TURN",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 59,
        "program": 1,
        "resource_name": "MOVEMENT_SLIG_STEP",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 48,
        "program": 1,
        "resource_name": "MOVEMENT_FLYSLIG_MOTOR_1",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 49,
        "program": 1,
        "resource_name": "MOVEMENT_FLYSLIG_MOTOR_2",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 50,
        "program": 1,
        "resource_name": "MOVEMENT_FLYSLIG_MOTOR_3",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 59,
        "program": 3,
        "resource_name": "MOVEMENT_MUD_HOP",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 58,
        "program": 3,
        "resource_name": "MOVEMENT_MUD_SLIDE",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 67,
        "program": 3,
        "resource_name": "MOVEMENT_MUD_SNEAK",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 72,
        "program": 3,
        "resource_name": "MOVEMENT_MUD_HITWALL",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 57,
        "program": 3,
        "resource_name": "MOVEMENT_MUD_ROLL",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 36,
        "program": 3,
        "resource_name": "MOVEMENT_MUD_STEP",
        "sound_bank": "mi_MINES_AePc",
        "pitch_range" : [ -1, 1 ]
    },
    {
        "data_set": "AePc",
        "note": 61,
        "program": 3,
        "resource_name": "MOVEMENT_MUD_LAND",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 60,
        "program": 3,
        "resource_name": "MOVEMENT_MUD_TURN",
        "sound_bank": "mi_MINES_AePc"
    },
    {
        "data_set": "AePc",
        "note": 66,
        "program": 4,
        "resource_name": "MOVEMENT_SCRAB_HOP",
        "sound_bank": "sv_SCRVAULT_AePc"
    },
    {
        "data_set": "AePc",
        "note": 64,
        "program": 4,
        "resource_name": "MOVEMENT_SCRAB_SPIN",
        "sound_bank": "sv_SCRVAULT_AePc"
    },
    {
        "data_set": "AePc",
        "note": 67,
        "program": 4,
        "resource_name": "MOVEMENT_SCRAB_STEP",
        "sound_bank": "sv_SCRVAULT_AePc",
        "pitch_range" : [ -2, 2 ]
    }
    ]
    },
    */

}


static std::map<std::string, std::string> gPrimarySfxSoundBanks =
{
    { "Glukkon_Commere", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_AllYa", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_DoIt", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_Help", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_Hey", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_StayHere", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_Laugh", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_Hurg", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_KillEm", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_Step", "AePc_B2SEQ_BAENDER" },
    { "Glukkon_What", "AePc_B2SEQ_BAENDER" },

};

static std::map<std::string, std::set<std::string>> gBrokenSfxSoundBanks =
{
    {
        "Slig_Hi",
        {
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_HereBoy",
        {
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_GetEm",
        {
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_Stay",
        {
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_Bs",
        {
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_LookOut",
        {
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_SmoBs",
        {
            "AePcDemo_MISEQ_MINES", // Has diff volume
            "AePcDemo_STSEQ_OPTION", // Ditto
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_Laugh",
        {
            "AePcDemo_STSEQ_OPTION",
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_Freeze",
        {
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_What",
        {
            "AePcDemo_STSEQ_OPTION",
            "AePc_STSEQ_OPTION",
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_Help",
        {
            "AePcDemo_STSEQ_OPTION",
            "AePc_STSEQ_OPTION",
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_Buhlur",
        {
            "AePcDemo_STSEQ_OPTION",
            "AePc_STSEQ_OPTION",
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_Gotcha",
        {
            "AePcDemo_STSEQ_OPTION",
            "AePc_B2SEQ_BAENDER",
            "AePc_B3SEQ_BWENDER",
            "AePc_STSEQ_OPTION",
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT"
        }
    },
    {
        "Slig_Ow",
        {
            "AePcDemo_STSEQ_OPTION",
            "AePc_B2SEQ_BAENDER",
            "AePc_B3SEQ_BWENDER",
            "AePc_STSEQ_OPTION",
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT",
            "AePc_SESEQ_SVENDER"
        }
    },
    {
        "Slig_Urgh",
        {
            "AePcDemo_STSEQ_OPTION",
            "AePc_B2SEQ_BAENDER",
            "AePc_B3SEQ_BWENDER",
            "AePc_STSEQ_OPTION",
            "AePc_PESEQ_PVENDER",
            "AePc_PVSEQ_PARVAULT",
            "AePc_SVSEQ_SCRVAULT",
            "AePc_SESEQ_SVENDER"
        }
    },
};

void SoundResourcesDumper::PopulateSoundEffectSoundBanks()
{
    std::set<std::string> allVabs;
    for (auto& res : mTempResources.mMusics)
    {
        for (auto& vab : res.mSoundBanks)
        {
            allVabs.insert(vab);
        }
    }


    for (TempSoundEffectResource& soundEffect : gSounds)
    {
        if (soundEffect.mSoundBanks.empty())
        {
            abort();
        }

        for (SoundEffectResourceLocation& loc : soundEffect.mSoundBanks)
        {
            // In the defined data above the initial sound banks are actually wild card matches for which
            // sound banks we really want to put in here. This is because there are tons of sound banks and in general
            // it appears that each sound effect is the same across a whole dataset.
            if (loc.mSoundBanks.size() != 1)
            {
                abort();
            }

            std::string startsWithMatcher = *loc.mSoundBanks.begin();
            loc.mSoundBanks.clear();

            for (const std::string& vab : allVabs)
            {
                if (string_util::starts_with(vab, startsWithMatcher, true))
                {
                    loc.mSoundBanks.insert(vab);
                }
            }
        }
    }
}

void SoundResourcesDumper::RemoveBrokenSoundEffectSoundBanks()
{
    for (TempSoundEffectResource& sfxRes : gSounds)
    {
        auto brokenSfxRec = gBrokenSfxSoundBanks.find(sfxRes.mResourceName);
        if (brokenSfxRec != std::end(gBrokenSfxSoundBanks))
        {
            for (SoundEffectResourceLocation& sfxLoc : sfxRes.mSoundBanks)
            {
                for (const std::string& brokenSoundBank : brokenSfxRec->second)
                {
                    sfxLoc.mSoundBanks.erase(brokenSoundBank);
                }
            }
        }
    }
}

// Note if any new resources where created this requires 2 passes as  they won't be in mLocator yet
void SoundResourcesDumper::RemoveSoundBanksThatDontMatchPrimarySample()
{
    // TODO: Impl to prevent 2 passes
    //mLocator.Reload();

    for (TempSoundEffectResource& sfxRes : gSounds)
    {
        auto primaryRec = gPrimarySfxSoundBanks.find(sfxRes.mResourceName);
        if (primaryRec != std::end(gPrimarySfxSoundBanks))
        {
            // Load sample data for sfxRes out of primarySoundBank
            const std::string primarySoundBank = primaryRec->second;
            std::unique_ptr<ISound> pFx = mLocator.LocateSound(sfxRes.mResourceName.c_str(), primarySoundBank.c_str(), false, true);
            if (pFx)
            {
                const s16 pSampleIndex = pFx->mVab->VagAt(pFx->mProgram, pFx->mNote)->iVag-1;
                Vab::SampleData pSampleData = pFx->mVab->mSamples[pSampleIndex];
                for (SoundEffectResourceLocation& sfxLoc : sfxRes.mSoundBanks)
                {
                    // Remove if sample data does not match primary
                    for (auto sbIt = std::begin(sfxLoc.mSoundBanks); sbIt != std::end(sfxLoc.mSoundBanks); )
                    {
                        std::unique_ptr<ISound> pCurrentFx = mLocator.LocateSound(sfxRes.mResourceName.c_str(), sbIt->c_str(), false, true);
                        if (pCurrentFx)
                        {
                            const VagAtr* pVag = pCurrentFx->mVab->VagAt(pFx->mProgram, pFx->mNote);
                            if (pVag)
                            {
                                const s16 sampleIndex = pVag->iVag-1;
                                Vab::SampleData sampleData = pCurrentFx->mVab->mSamples[sampleIndex];
                                if (sampleData != pSampleData)
                                {
                                    sbIt = sfxLoc.mSoundBanks.erase(sbIt);
                                    continue;
                                }
                            }
                            else
                            {
                                sbIt = sfxLoc.mSoundBanks.erase(sbIt);
                                continue;
                            }
                        }
                        else
                        {
                            sbIt = sfxLoc.mSoundBanks.erase(sbIt);
                            continue;
                        }
                        sbIt++;
                    }
                }
            }
        }
    }
}

void SoundResourcesDumper::RemoveSEQsWithNoSoundBanks()
{
    for (auto musicIt = mTempResources.mMusics.begin(); musicIt != mTempResources.mMusics.end(); )
    {
        bool erased = false;
        if (musicIt->mSoundBanks.empty())
        {
            musicIt = mTempResources.mMusics.erase(musicIt);
            erased = true;
        }

        if (!erased)
        {
            musicIt++;
        }
    }
}

static SoundResource* FindRecord(std::vector<SoundResource>& all, const std::string& name)
{
    for (auto& rec : all)
    {
        if (rec.mResourceName == name)
        {
            return &rec;
        }
    }
    return nullptr;
}

static void CopyRec(SoundResource& sfxRec, const TempSoundEffectResource& sfx)
{
    sfxRec.mResourceName = sfx.mResourceName;
    sfxRec.mSoundEffect.mMaxPitch = sfx.mMaxPitch;
    sfxRec.mSoundEffect.mMinPitch = sfx.mMinPitch;
    sfxRec.mSoundEffect.mVolume = sfx.mVolume;
    sfxRec.mSoundEffect.mSoundBanks = sfx.mSoundBanks;
}

void SoundResourcesDumper::MergeToFinalResources()
{
    // To start with just merge all of the musics in
    for (const TempMusicResource& music : mTempResources.mMusics)
    {
        SoundResource musicRec;
        musicRec.mResourceName = music.mResourceName;
        musicRec.mMusic.mResourceId = music.mResourceId;
        musicRec.mMusic.mSoundBanks = music.mSoundBanks;
        mFinalResources.mSounds.push_back(musicRec);
    }

    // Now either add new music records or add as the counter part to the SEQ
    for (const TempSoundEffectResource& sfx : gSounds)
    {
        SoundResource* pFound = FindRecord(mFinalResources.mSounds, sfx.mResourceName);
        if (pFound)
        {
            if (!pFound->mSoundEffect.mSoundBanks.empty())
            {
                // Attempting to overwrite a sound effect
                abort();
            }

            CopyRec(*pFound, sfx);
        }
        else
        {
            SoundResource sfxRec;
            CopyRec(sfxRec, sfx);
            mFinalResources.mSounds.push_back(sfxRec);
        }
    }
}

void SoundResourcesDumper::GenerateThemes()
{
    {
        // TODO: AE_FE_10_1
        MusicTheme theme;
        theme.mName = "Feeco";
        theme.mEntries["AMBIANCE"] = { { "FEECOAMB", 1} };
        theme.mEntries["BASE_LINE"] = { { "AE_FE_3_1", 1 }, { "AE_FE_2_1", 1 }, { "AE_FE_1_1", 1 } };
        theme.mEntries["SLIG_PATROL"] = { { "AE_FE_4_1", 1 } };
        theme.mEntries["SLIG_POSSESSED"] = { { "AE_FE_5_1", 1 } };
        theme.mEntries["SLIG_ATTACK"] = { { "AE_FE_6_1", 1 } };
        theme.mEntries["CRITTER_PATROL"] = { { "AE_FE_8_1", 1 } };
        theme.mEntries["CRITTER_ATTACK"] = { { "AE_FE_9_1", 1 } };
        mFinalResources.mThemes.push_back(theme);
    }

    {
        // TODO: MI_10_1
        MusicTheme theme;
        theme.mName = "Mines";
        theme.mEntries["AMBIANCE"] = { { "MINESAMB", 1 } };
        theme.mEntries["BASE_LINE"] = { { "MI_3_1", 1 },{ "MI_2_1", 1 },{ "MI_1_1", 1 } };
        theme.mEntries["SLIG_PATROL"] = { { "MI_4_1", 1 } };
        theme.mEntries["SLIG_POSSESSED"] = { { "MI_5_1", 1 } };
        theme.mEntries["SLIG_ATTACK"] = { { "MI_6_1", 1 } };
        theme.mEntries["CRITTER_PATROL"] = { { "MI_8_1", 1 } };
        theme.mEntries["CRITTER_ATTACK"] = { { "MI_9_1", 1 } };
        mFinalResources.mThemes.push_back(theme);
    }

    {
        // TODO: NE_7_1
        MusicTheme theme;
        theme.mName = "Necrum";
        theme.mEntries["AMBIANCE"] = { { "NECRAMB", 1 } };
        theme.mEntries["BASE_LINE"] = { { "NE_3_1", 1 },{ "NE_2_1", 1 },{ "NE_1_1", 1 } };
        theme.mEntries["SLIG_PATROL"] = { { "NE_4_1", 1 } };
        theme.mEntries["SLIG_POSSESSED"] = { { "NE_5_1", 1 } };
        theme.mEntries["SLIG_ATTACK"] = { { "NE_6_1", 1 } };
        theme.mEntries["CRITTER_PATROL"] = { { "NE_8_1", 1 } };
        theme.mEntries["CRITTER_ATTACK"] = { { "NE_9_1", 1 } };
        mFinalResources.mThemes.push_back(theme);
    }

    {
        // Paramite vault
    }

    {
        // Paramite vault ender
    }

    {
        // Scrab vault
    }

    {
        // Scrab vault ender
    }

    {
		// TOOD: BR_10_1
		MusicTheme theme;
        theme.mName = "Brewery";
        theme.mEntries["AMBIANCE"] = { { "BREWAMB", 1 } };
        theme.mEntries["BASE_LINE"] = { { "BR_3_1", 1 },{ "BR_2_1", 1 },{ "BR_1_1", 1 } };
        theme.mEntries["SLIG_PATROL"] = { { "BR_4_1", 1 } };
        theme.mEntries["SLIG_POSSESSED"] = { { "BR_5_1", 1 } };
        theme.mEntries["SLIG_ATTACK"] = { { "BR_6_1", 1 } };
        theme.mEntries["CRITTER_PATROL"] = { { "BR_8_1", 1 } };
        theme.mEntries["CRITTER_ATTACK"] = { { "BR_9_1", 1 } };
        mFinalResources.mThemes.push_back(theme);	
    }
	
    {
		MusicTheme theme;
        theme.mName = "Brewery ender";
        theme.mEntries["AMBIANCE"] = { { "BREWAMB", 1 } };
        theme.mEntries["BASE_LINE"] = { { "BREND_3", 1 },{ "BREND_2", 1 },{ "BREND_1", 1 } };
        theme.mEntries["SLIG_PATROL"] = { { "BREND_4", 1 } };
        theme.mEntries["SLIG_POSSESSED"] = { { "BREND_5", 1 } };
        theme.mEntries["SLIG_ATTACK"] = { { "BREND_6", 1 } };
        //theme.mEntries["CRITTER_PATROL"] = { { "BREND_8", 1 } };
        //theme.mEntries["CRITTER_ATTACK"] = { { "BREND_9", 1 } };
        mFinalResources.mThemes.push_back(theme);
    }
	
    {
		MusicTheme theme;
        theme.mName = "Barracks";
        theme.mEntries["AMBIANCE"] = { { "BARRAMB", 1 } };
        theme.mEntries["BASE_LINE"] = { { "BA_3_1", 1 },{ "BA_2_1", 1 },{ "BA_1_1", 1 } };
        theme.mEntries["SLIG_PATROL"] = { { "BA_4_1", 1 } };
        theme.mEntries["SLIG_POSSESSED"] = { { "BA_5_1", 1 } };
        theme.mEntries["SLIG_ATTACK"] = { { "BA_6_1", 1 } };
        //theme.mEntries["CRITTER_PATROL"] = { { "BA_8_1", 1 } };
        //theme.mEntries["CRITTER_ATTACK"] = { { "BA_9_1", 1 } };
        mFinalResources.mThemes.push_back(theme);	
		
    }

	{
		// Barracks ender
	}

    {
        // Bonewerkz
    }

    {
        // Bonewerkz ender
    }
}

void SoundResourcesDumper::OnFinished()
{
    // Remove sound banks that don't work with the given SEQ
    RemoveBadSoundBanks();

    // Split off sound banks to their own record if they are not the same. E.g some
    // SEQ names across AO/AE have the same name but are not the same track.
    SplitSEQs();

    // Simple rename of X to Y
    RenameSEQs();

    // Clean up any SEQs which can't be played because we've removed all sound banks
    RemoveSEQsWithNoSoundBanks();

    // We just put the data set name in the sound bank name of the hard coded list of sounds, this function adds all
    // sound banks that start with that name, e.g if its set to "Ae" then every sound bank that starts with "Ae" will be
    // added
    PopulateSoundEffectSoundBanks();

    // Like RemoveBadSoundBanks but for sound effects
    RemoveBrokenSoundEffectSoundBanks();
    
    // Better method of RemoveBrokenSoundEffectSoundBanks, we specify one sound bank of the data set that is correct
    // and then remove all others that don't have byte for byte matching sample data
    RemoveSoundBanksThatDontMatchPrimarySample();

    // Since we effectively have a list of music and sound effects we now need to merge them into one list
    // here it is expected that a SEQ and a SAMPLE may have the same name, but there shouldn't be more than one duplicate
    // for example
    // Ao WHISTLE1.SEQ and
    // Ae WHISTLE1 SAMPLE can be merged but there shouldn't then be
    // anything else that gets merged otherwise it will overwrite the previous
    MergeToFinalResources();

    GenerateThemes();

    for (auto& res : mFinalResources.mSounds)
    {
        LOG_INFO(res.mResourceName);
    }
    mFinalResources.Dump("..\\data\\sounds.json");
}
