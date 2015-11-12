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

class DataTest
{
public:
    enum eDataType
    {
        eAoPc,
        eAoPsx,
        eAePc,
        eAePsx,
        eAoPcDemo,
        eAoPsxDemo,
        eAePcDemo,
        eAePsxDemo
    };

    DataTest(eDataType eType, const std::string& resourcePath, const std::vector<std::string>& lvls)
        : mType(eType)
    {
        mFs.Init();
        
        // Clear out the ones loaded from resource paths json
        mFs.ResourcePaths().ClearAllResourcePaths();

        mGameData.Init(mFs);
        
        // Add the only one we're interested in
        mFs.ResourcePaths().AddResourcePath(resourcePath, 1);

        for (const auto& lvl : lvls)
        {
            auto stream = mFs.ResourcePaths().Open(lvl);
            if (stream)
            {
                Oddlib::LvlArchive archive(std::move(stream));
                ReadAllAnimations(archive);
                //ReadFg1s(archive);
                //ReadFonts(archive);
                //ReadAllPaths(archive);
                //ReadAllCameras(archive);
    
                // TODO: Handle sounds/fmvs
            }
            else
            {
                LOG_WARNING("LVL not found: " << lvl);
            }
        }
    }

    void ForChunksOfType(Oddlib::LvlArchive& archive, Uint32 type, std::function<void(Oddlib::LvlArchive::FileChunk&)> cb)
    {
        for (auto i = 0u; i < archive.FileCount(); i++)
        {
            auto file = archive.FileByIndex(i);
            for (auto j = 0u; j < file->ChunkCount(); j++)
            {
                auto chunk = file->ChunkByIndex(j);
                if (chunk->Type() == type)
                {
                    // For AE PSX varients these files need parsing checking/fixing as they seem
                    // to be some slightly changed Anim format
                    bool bBroken =
                    (file->FileName() == "SLIG.BND" && chunk->Id() == 360) ||
                    (file->FileName() == "DUST.BAN" && chunk->Id() == 303) ||
                    (file->FileName() == "METAL.BAN" && chunk->Id() == 365) ||
                    (file->FileName() == "VAPOR.BAN" && chunk->Id() == 305) ||
                    (file->FileName() == "DEADFLR.BAN" && chunk->Id() == 349) ||
                    (file->FileName() == "DEBRIS00.BAN" && chunk->Id() == 1105) ||
                    (file->FileName() == "DOVBASIC.BAN" && chunk->Id() == 60) ||
                    (file->FileName() == "DRPROCK.BAN" && chunk->Id() == 357) || 
                    (file->FileName() == "DRPSPRK.BAN" && chunk->Id() == 357) ||
                    (file->FileName() == "EVILFART.BAN" && chunk->Id() == 6017) ||
                    (file->FileName() == "OMMFLARE.BAN" && chunk->Id() == 312) ||
                    (file->FileName() == "SHELL.BAN" && chunk->Id() == 360) ||
                    (file->FileName() == "LANDMINE.BAN" && chunk->Id() == 1036) ||
                    (file->FileName() == "SLURG.BAN" && chunk->Id() == 306) ||
                    (file->FileName() == "SQBSMK.BAN" && chunk->Id() == 354) ||
                    (file->FileName() == "STICK.BAN" && chunk->Id() == 358) ||
                    (file->FileName() == "TBOMB.BAN" && chunk->Id() == 1037) ||
                    (file->FileName() == "WELLLEAF.BAN" && chunk->Id() == 341) ||
                    (file->FileName() == "BOMB.BND" && chunk->Id() == 1005) ||
                    (file->FileName() == "MINE.BND" && chunk->Id() == 1036) ||
                    (file->FileName() == "UXB.BND" && chunk->Id() == 1037) ||
                    (file->FileName() == "EXPLODE.BND" && chunk->Id() == 1105) ||
                    (file->FileName() == "TRAPDOOR.BAN" && chunk->Id() == 1004) ||
                    (file->FileName() == "SLAM.BAN" && chunk->Id() == 2020) ||
                    (file->FileName() == "SLAMVLTS.BAN" && chunk->Id() == 2020) ||
                    (file->FileName() == "BOMB.BAN" && chunk->Id() == 1005);

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
    }

    void ReadFg1s(Oddlib::LvlArchive& archive)
    {
        ForChunksOfType(archive, Oddlib::MakeType('F', 'G', '1', ' '), [&](Oddlib::LvlArchive::FileChunk&)
        {
            // TODO: FG1 parsing
        });
    }

    void ReadFonts(Oddlib::LvlArchive& archive)
    {
        ForChunksOfType(archive, Oddlib::MakeType('F', 'o', 'n', 't'), [&](Oddlib::LvlArchive::FileChunk&)
        {
            // TODO: Font parsing
        });
    }

    void ReadAllPaths(Oddlib::LvlArchive& archive)
    {
        ForChunksOfType(archive, Oddlib::MakeType('P', 'a', 't', 'h'), [&](Oddlib::LvlArchive::FileChunk&)
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

    void ReadAllCameras(Oddlib::LvlArchive& archive)
    {
        ForChunksOfType(archive, Oddlib::MakeType('B', 'i', 't', 's'), [&](Oddlib::LvlArchive::FileChunk& chunk)
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

    void ReadAllAnimations(Oddlib::LvlArchive& archive)
    {
        ForChunksOfType(archive, Oddlib::MakeType('A', 'n', 'i', 'm'), [&](Oddlib::LvlArchive::FileChunk& chunk)
        {
            Oddlib::AnimSerializer anim(*chunk.Stream());

        });
    }

private:
    eDataType mType;
    GameData mGameData;
    FileSystem mFs;
};

int main(int /*argc*/, char** /*argv*/)
{
    // TODO: Set path/type via command line and setup automated test with valgrind enabled

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
    DataTest aePsxCd1(DataTest::eAePsx, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin", aeLvls);
    DataTest aePsxCd2(DataTest::eAePsx, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 2) [SLES-11480].bin", aeLvls);
    DataTest aePc(DataTest::eAePc, "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Exoddus", aeLvls);

    // AO PC/PSX
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
    DataTest aoPc(DataTest::eAoPc, "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Oddysee", aoLvls);
    DataTest aoPsx(DataTest::eAoPsx, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (E) [SLES-00664].bin", aoLvls);

    const std::vector<std::string> aoDemoLvls =
    {
        "c1.lvl",
        "r1.lvl",
        "s1.lvl"
    };
    DataTest aoDemoPc(DataTest::eAoPcDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\abeodd", aoDemoLvls);

    const std::vector<std::string> aeDemoLvls =
    {
        "cr.lvl",
        "mi.lvl",
        "st.lvl"
    };
    DataTest aeDemoPc(DataTest::eAePcDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\exoddemo", aeDemoLvls);

    const std::vector<std::string> aoDemoPsxLvls =
    {
        "ABESODSE\\C1.LVL",
        "ABESODSE\\E1.LVL",
        "ABESODSE\\R1.LVL",
        "ABESODSE\\S1.LVL"
    };

    DataTest aoDemoPsx(DataTest::eAoPsxDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (Demo) (E) [SLED-00725].bin", aoDemoPsxLvls);

    const std::vector<std::string> aeDemoPsxLvls =
    {
        "ABE2\\CR.LVL",
        "ABE2\\MI.LVL",
        "ABE2\\ST.LVL"
    };

    DataTest aeDemoPsx(DataTest::eAePsxDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Euro Demo 38 (E) (Track 1) [SCED-01148].bin", aeDemoPsxLvls);

    return 0;
}
