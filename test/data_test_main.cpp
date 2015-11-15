#include "SDL.h"
#include "oddlib/stream.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/bits_factory.hpp"
#include "oddlib/ao_bits_pc.hpp"
#include "oddlib/ae_bits_pc.hpp"
#include "oddlib/psx_bits.hpp"
#include "oddlib/anim.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include "stdthread.h"
#include "msvc_sdl_link.hpp"
#include "gamedata.hpp"

class LvlFileReducer
{
public:
    LvlFileReducer(const LvlFileReducer&) = delete;
    LvlFileReducer& operator = (const LvlFileReducer&) = delete;
    LvlFileReducer(const std::string& resourcePath, const std::vector<std::string>& lvlFiles)
        : mLvlFiles(lvlFiles)
    {
        mFs.Init();

        // Clear out the ones loaded from resource paths json
        mFs.ResourcePaths().ClearAllResourcePaths();

        mGameData.Init(mFs);

        // Add the only one we're interested in
        mFs.ResourcePaths().AddResourcePath(resourcePath, 1);

        for (const auto& lvl : mLvlFiles)
        {
            auto stream = mFs.ResourcePaths().Open(lvl);
            if (stream)
            {
                Reduce(std::make_unique<Oddlib::LvlArchive>(std::move(stream)));
            }
            else
            {
                LOG_WARNING("LVL not found: " << lvl);
            }
        }
    }

    const std::vector<std::pair<Oddlib::LvlArchive::FileChunk*, std::string>>& Chunks() const
    {
        return mChunks;
    }

private:
    void Reduce(std::unique_ptr<Oddlib::LvlArchive> lvl)
    {
        bool chunkTaken = false;
        for (auto i = 0u; i < lvl->FileCount(); i++)
        {
            auto file = lvl->FileByIndex(i);
            for (auto j = 0u; j < file->ChunkCount(); j++)
            {
                auto chunk = file->ChunkByIndex(j);
                if (!ChunkExists(*chunk))
                {
                    AddChunk(chunk, file->FileName());
                    chunkTaken = true;
                }
            }
        }
        if (chunkTaken)
        {
            AddLvl(std::move(lvl));
        }
    }

    bool ChunkExists(Oddlib::LvlArchive::FileChunk& chunkToCheck)
    {
        for (auto& chunk : mChunks)
        {
            if (*chunk.first == chunkToCheck)
            {
                return true;
            }
        }
        return false;
    }

    void AddLvl(std::unique_ptr<Oddlib::LvlArchive> lvl)
    {
        mLvls.emplace_back(std::move(lvl));
    }

    void AddChunk(Oddlib::LvlArchive::FileChunk* chunk, const std::string& fileName)
    {
        mChunks.push_back(std::make_pair(chunk, fileName));
    }

    const std::vector<std::string>& mLvlFiles;
    GameData mGameData;
    FileSystem mFs;

    std::vector<std::pair<Oddlib::LvlArchive::FileChunk*, std::string>> mChunks;
    std::vector<std::unique_ptr<Oddlib::LvlArchive>> mLvls;
};

class DataTest
{
public:
    enum eDataType
    {
        eAoPc,
        eAoPcDemo,
        eAoPsx,
        eAoPsxDemo,
        eAePc,
        eAePcDemo,
        eAePsx,
        eAePsxDemo
    };

    DataTest(eDataType eType, const std::string& resourcePath, const std::vector<std::string>& lvls)
        : mType(eType), mReducer(resourcePath, lvls)
    {
        ReadAllAnimations();
        //ReadFg1s();
        //ReadFonts();
        //ReadAllPaths();
        //ReadAllCameras();
        // TODO: Handle sounds/fmvs
    }

    void ForChunksOfType(Uint32 type, std::function<void(Oddlib::LvlArchive::FileChunk&)> cb)
    {
        for (auto chunkPair : mReducer.Chunks())
        {
            auto chunk = chunkPair.first;
            auto fileName = chunkPair.second;
            if (chunk->Type() == type)
            {
                // For AE PSX variants these files need parsing checking/fixing as they seem
                // to be some slightly changed Anim format
                bool bBroken =
                    (fileName == "SLIG.BND" && chunk->Id() == 360) ||
                    (fileName == "DUST.BAN" && chunk->Id() == 303) ||
                    (fileName == "METAL.BAN" && chunk->Id() == 365) ||
                    (fileName == "VAPOR.BAN" && chunk->Id() == 305) ||
                    (fileName == "DEADFLR.BAN" && chunk->Id() == 349) ||
                    (fileName == "DEBRIS00.BAN" && chunk->Id() == 1105) ||
                    (fileName == "DOVBASIC.BAN" && chunk->Id() == 60) ||
                    (fileName == "DRPROCK.BAN" && chunk->Id() == 357) ||
                    (fileName == "DRPSPRK.BAN" && chunk->Id() == 357) ||
                    (fileName == "EVILFART.BAN" && chunk->Id() == 6017) ||
                    (fileName == "OMMFLARE.BAN" && chunk->Id() == 312) ||
                    (fileName == "SHELL.BAN" && chunk->Id() == 360) ||
                    (fileName == "LANDMINE.BAN" && chunk->Id() == 1036) ||
                    (fileName == "SLURG.BAN" && chunk->Id() == 306) ||
                    (fileName == "SQBSMK.BAN" && chunk->Id() == 354) ||
                    (fileName == "STICK.BAN" && chunk->Id() == 358) ||
                    (fileName == "TBOMB.BAN" && chunk->Id() == 1037) ||
                    (fileName == "WELLLEAF.BAN" && chunk->Id() == 341) ||
                    (fileName == "BOMB.BND" && chunk->Id() == 1005) ||
                    (fileName == "MINE.BND" && chunk->Id() == 1036) ||
                    (fileName == "UXB.BND" && chunk->Id() == 1037) ||
                    (fileName == "EXPLODE.BND" && chunk->Id() == 1105) ||
                    (fileName == "TRAPDOOR.BAN" && chunk->Id() == 1004) ||
                    (fileName == "SLAM.BAN" && chunk->Id() == 2020) ||
                    (fileName == "SLAMVLTS.BAN" && chunk->Id() == 2020) ||
                    (fileName == "BOMB.BAN" && chunk->Id() == 1005);

                if (bBroken && (mType != eAePsx && mType != eAePsxDemo))
                {
                    bBroken = false;
                }

                if (!bBroken)
                {
                    cb(*chunk);
                }
            }
        }
    }

    void ReadFg1s()
    {
        ForChunksOfType(Oddlib::MakeType('F', 'G', '1', ' '), [&](Oddlib::LvlArchive::FileChunk&)
        {
            // TODO: FG1 parsing
        });
    }

    void ReadFonts()
    {
        ForChunksOfType(Oddlib::MakeType('F', 'o', 'n', 't'), [&](Oddlib::LvlArchive::FileChunk&)
        {
            // TODO: Font parsing
        });
    }

    void ReadAllPaths()
    {
        ForChunksOfType(Oddlib::MakeType('P', 'a', 't', 'h'), [&](Oddlib::LvlArchive::FileChunk&)
        {
            // TODO: Load the game data json for the required hard coded data to load the path
            /*
            Oddlib::Path path(*chunk.Stream(),
                entry->mCollisionDataOffset,
                entry->mObjectIndexTableOffset,
                entry->mObjectDataOffset,
                entry->mMapXSize,
                entry->mMapYSize);*/
        });
    }

    void ReadAllCameras()
    {
        ForChunksOfType(Oddlib::MakeType('B', 'i', 't', 's'), [&](Oddlib::LvlArchive::FileChunk& chunk)
        {
            auto bits = Oddlib::MakeBits(*chunk.Stream());
            Oddlib::IBits* ptr = nullptr;
            switch (mType)
            {
            case eAoPc:
                ptr = dynamic_cast<Oddlib::AoBitsPc*>(bits.get());
                break;

            case eAePc:
                ptr = dynamic_cast<Oddlib::AeBitsPc*>(bits.get());
                break;

            case eAePsxDemo:
            case eAePsx:
            {
                auto tmp = dynamic_cast<Oddlib::PsxBits*>(bits.get());
                if (tmp && tmp->IncludeLength())
                {
                    ptr = tmp;
                }
            }
                break;

            case eAoPsxDemo:
            case eAoPsx:
            {
                auto tmp = dynamic_cast<Oddlib::PsxBits*>(bits.get());
                if (tmp && !tmp->IncludeLength())
                {
                    ptr = tmp;
                }
            }
                break;

            case eAoPcDemo:
                ptr = dynamic_cast<Oddlib::AoBitsPc*>(bits.get());
                break;

            case eAePcDemo:
                ptr = dynamic_cast<Oddlib::AoBitsPc*>(bits.get());
                if (!ptr)
                {
                    // Very strange, AE PC demo contains "half" cameras of random old PSX
                    // cameras, even half of the anti piracy screen is in here
                    auto tmp = dynamic_cast<Oddlib::PsxBits*>(bits.get());
                    if (tmp && tmp->IncludeLength())
                    {
                        ptr = tmp;
                    }
                }
                break;

            default:
                abort();
            }


            if (!ptr)
            {
                LOG_ERROR("Wrong camera type constructed");
                abort();
            }
            else
            {
                ptr->Save();
            }

        });
    }

    void ReadAllAnimations()
    {
        ForChunksOfType(Oddlib::MakeType('A', 'n', 'i', 'm'), [&](Oddlib::LvlArchive::FileChunk& chunk)
        {
            Oddlib::AnimSerializer anim(*chunk.Stream());

        });
    }

private:
    eDataType mType;
    LvlFileReducer mReducer;
};

int main(int /*argc*/, char** /*argv*/)
{
    const std::vector<std::string> aoLvls =
    {
        "s1.lvl",
        "r6.lvl",
        "r2.lvl",
        "r1.lvl",
        "l1.lvl",
        "f4.lvl",
        "f2.lvl",
        "f1.lvl",
        "e2.lvl",
        "e1.lvl",
        "d7.lvl",
        "d2.lvl",
        "c1.lvl"
    };

    const std::vector<std::string> aoDemoLvls =
    {
        "c1.lvl",
        "r1.lvl",
        "s1.lvl"
    };

    const std::vector<std::string> aoDemoPsxLvls =
    {
        "ABESODSE\\C1.LVL",
        "ABESODSE\\E1.LVL",
        "ABESODSE\\R1.LVL",
        "ABESODSE\\S1.LVL"
    };

    const std::vector<std::string> aeLvls =
    {
        "mi.lvl",
        "ba.lvl",
        "bm.lvl",
        "br.lvl",
        "bw.lvl",
        "cr.lvl",
        "fd.lvl",
        "ne.lvl",
        "pv.lvl",
        "st.lvl",
        "sv.lvl"
    };

    const std::vector<std::string> aeDemoLvls =
    {
        "cr.lvl",
        "mi.lvl",
        "st.lvl"
    };

    const std::vector<std::string> aeDemoPsxLvls =
    {
        "ABE2\\CR.LVL",
        "ABE2\\MI.LVL",
        "ABE2\\ST.LVL"
    };

    const std::map<DataTest::eDataType, const std::vector<std::string>*> DataTypeLvlMap =
    {
        { DataTest::eAoPc, &aoLvls },
        { DataTest::eAoPsx, &aoLvls },
        { DataTest::eAePc, &aeLvls },
        { DataTest::eAePsx, &aeLvls },
        { DataTest::eAoPcDemo, &aoDemoLvls },
        { DataTest::eAoPsxDemo, &aoDemoPsxLvls },
        { DataTest::eAePcDemo, &aeDemoLvls},
        { DataTest::eAePsxDemo, &aeDemoPsxLvls }
    };

    const std::map<DataTest::eDataType, std::string> datas =
    {
        { DataTest::eAePc,      "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Exoddus" },
        /*
        { DataTest::eAePcDemo,  "C:\\Users\\paul\\Desktop\\alive\\all_data\\exoddemo" },
        { DataTest::eAePsx,     "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin" },
        { DataTest::eAePsx,     "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 2) [SLES-11480].bin" },
        { DataTest::eAePsxDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Euro Demo 38 (E) (Track 1) [SCED-01148].bin" },
        { DataTest::eAoPc,      "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Oddysee" },
        { DataTest::eAoPcDemo,  "C:\\Users\\paul\\Desktop\\alive\\all_data\\abeodd" },
        { DataTest::eAoPsx,     "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (E) [SLES-00664].bin" },
        { DataTest::eAoPsxDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (Demo) (E) [SLED-00725].bin" },
        */
    };

    for (const auto& data : datas)
    {
        const auto it = DataTypeLvlMap.find(data.first);
        if (it == std::end(DataTypeLvlMap))
        {
            // Defined struct is wrong
            abort();
        }
        DataTest dataDecoder(data.first, data.second, *it->second);
    }

    return 0;
}
