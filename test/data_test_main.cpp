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
    LvlFileReducer(const std::string& resourcePath, const std::vector<std::string>& lvlFiles, std::vector<std::string> fileFilter = std::vector<std::string>())
        : mLvlFiles(lvlFiles), mFileFilter(fileFilter)
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

            bool bUseFile = true;
            if (!mFileFilter.empty())
            {
                bUseFile = false;
                for (const auto& includedFile : mFileFilter)
                {
                    if (includedFile == file->FileName())
                    {
                        bUseFile = true;
                        break;
                    }
                }
            }

            if (bUseFile)
            {
                for (auto j = 0u; j < file->ChunkCount(); j++)
                {
                    auto chunk = file->ChunkByIndex(j);
                  //  if (chunk->Id() == 360)
                    {
                        if (!ChunkExists(*chunk))
                        {
                            AddChunk(chunk, file->FileName());
                            chunkTaken = true;
                        }
                    }
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
    std::vector<std::string> mFileFilter;

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

    DataTest(eDataType eType, const std::string& resourcePath, const std::vector<std::string>& lvls, const std::vector<std::string>& fileFilter)
        : mType(eType), mReducer(resourcePath, lvls, fileFilter)
    {
        ReadAllAnimations();
        //ReadFg1s();
        //ReadFonts();
        //ReadAllPaths();
        //ReadAllCameras();
        // TODO: Handle sounds/fmvs
    }

    void ForChunksOfType(Uint32 type, std::function<void(const std::string&, Oddlib::LvlArchive::FileChunk&)> cb)
    {
        for (auto chunkPair : mReducer.Chunks())
        {
            auto chunk = chunkPair.first;
            auto fileName = chunkPair.second;
            if (chunk->Type() == type)
            {
                /*
                // For AE PSX variants these files need parsing checking/fixing as they seem
                // to be some slightly changed Anim format
                bool bBroken =
                    (fileName == "SLIG.BND" && chunk->Id() == 360) ||           // [1C 00 00 00] 30 05 [04] [06] [31 00 00 00] // 40 = Abs offset to frame?

                    //[1]	0x[03][03][0000]
                    //[5]	0x[05][03][0008]
                    //[3]	0x[04][02][0010]
                    //[6]	0x[05][03][0018]
                    //[4]	0x[04][02][0020]
                    //[2]	0x[03][03][0028]

                    (fileName == "DUST.BAN" && chunk->Id() == 303) ||           // [1C 00 00 00] 78 4D [04] [06] [36 07 00 00] // 40
                    (fileName == "METAL.BAN" && chunk->Id() == 365) ||          // [1C 00 00 00] 78 3A [04] [06] [37 04 00 00] // 40
                    (fileName == "VAPOR.BAN" && chunk->Id() == 305) ||          // [1C 00 00 00] 78 73 [04] [06] [AB 08 00 00] // 40
                    (fileName == "DEBRIS00.BAN" && chunk->Id() == 1105) ||      // [1C 00 00 00] 78 5F [04] [06] [07 0A 00 00] // 40
                    //         78 5F = 120,95
                    //[0]	0x[18,22][00,50] 
                    //[1]	0x[1b,22][00,28]
                    //[2]	0x[1c,27][42,00]
                    //[3]	0x[1d,29][42,28]
                    //[4]	0x[1e,28][00,00]
                    //[5]	0x[21,22][1e,00]
                    //[6]	0x[23,20][1e,48]
                    //[7]	0x[24,1f][1e,28]

                    //[0]	0x[18,22][00,50] 50,00,22,18 | 80, 0, 34 (64), 24
                    //[1]	0x[1b,22][00,28] 28,00,22,1b | 40, 0, 34 (64), 27
                    //[2]	0x[1c,27][42,00] 00,42,27,1c | 0,  66, 39 (78), 28
                    //[3]	0x[1d,29][42,28] 28,42,29,1d | 40, 66, 41 (82), 29
                    //[4]	0x[1e,28][00,00] 00,00,28,1e | 0, 0, 40 (80), 30
                    //[5]	0x[21,22][1e,00] 00,1e,22,21 |       34 (64), 33
                    //[6]	0x[23,20][1e,48] 48,1e,20,23 |       32 (64),  35
                    //[7]	0x[24,1f][1e,28] 28,1e,1f,24 |        31 (62)  36

                    //[2] 64x24 // 0
                    //[1] 64x27 // 1
                    //[6] 72x28 // 2
                    //[7] 72x29 // 3
                    //[0] 72x30 // 4
                    //[3] 64x33 // 5
                    //[5] 56x35 // 6
                    //[4] 56x36 // 7

                    // Seems to be xoff, yoff, w,h


                    (fileName == "DRPROCK.BAN" && chunk->Id() == 357) ||        // [1C 00 00 00] 18 04 [04] [06] [1F 00 00 00] // 40
                    (fileName == "DRPSPRK.BAN" && chunk->Id() == 357) ||        // [1C 00 00 00] 18 04 [04] [06] [1C 00 00 00] // 40
                    (fileName == "EVILFART.BAN" && chunk->Id() == 6017) ||      // [1C 00 00 00] 68 44 [04] [06] [F6 05 00 00] // 40
                    (fileName == "SHELL.BAN" && chunk->Id() == 360) ||          // [1C 00 00 00] 30 05 [04] [06] [31 00 00 00] // 40
                    (fileName == "SQBSMK.BAN" && chunk->Id() == 354) ||         // [1C 00 00 00] 78 73 [04] [06] [AB 08 00 00] // 40
                    (fileName == "STICK.BAN" && chunk->Id() == 358) ||          // [1C 00 00 00] 68 2C [04] [06] [2D 03 00 00] // 40
                    (fileName == "WELLLEAF.BAN" && chunk->Id() == 341) ||       // [1C 00 00 00] 40 0B [04] [06] [A0 00 00 00] // 40
                    (fileName == "EXPLODE.BND" && chunk->Id() == 1105) ||       // [1C 00 00 00] 78 5F [04] [06] [07 0A 00 00] // 40

                    (fileName == "DEADFLR.BAN" && chunk->Id() == 349) ||        // [1C 00 00 00] 7C CC [08] [07] [3C 16 00 00] // A0
                    (fileName == "DOVBASIC.BAN" && chunk->Id() == 60) ||        // [1C 00 00 00] 80 43 [08] [07] [0C 0A 00 00] // A0
                    (fileName == "OMMFLARE.BAN" && chunk->Id() == 312) ||       // [1C 00 00 00] 64 15 [08] [07] [56 03 00 00] // A0
                    (fileName == "LANDMINE.BAN" && chunk->Id() == 1036) ||      // [1C 00 00 00] 30 0C [08] [07] [31 01 00 00] // A0
                    (fileName == "SLURG.BAN" && chunk->Id() == 306) ||          // [1C 00 00 00] 80 27 [08] [07] [74 07 00 00] // A0
                    (fileName == "TBOMB.BAN" && chunk->Id() == 1037) ||         // [1C 00 00 00] 80 49 [08] [07] [BC 12 00 00] // A0
                    (fileName == "BOMB.BND" && chunk->Id() == 1005) ||          // [1C 00 00 00] 28 11 [08] [07] [4E 01 00 00] // A0
                    (fileName == "MINE.BND" && chunk->Id() == 1036) ||          // [1C 00 00 00] 30 0C [08] [07] [31 01 00 00] // A0
                    (fileName == "UXB.BND" && chunk->Id() == 1037) ||           // [1C 00 00 00] 80 49 [08] [07] [BC 12 00 00] // A0
                    (fileName == "TRAPDOOR.BAN" && chunk->Id() == 1004) ||      // [1C 00 00 00] 7C 46 [08] [07] [7A 0B 00 00] // A0
                    (fileName == "SLAM.BAN" && chunk->Id() == 2020) ||          // [1C 00 00 00] 28 44 [08] [07] [9F 06 00 00] // A0
                    (fileName == "SLAMVLTS.BAN" && chunk->Id() == 2020) ||      // [1C 00 00 00] 24 44 [08] [07] [E0 05 00 00] // A0
                    (fileName == "BOMB.BAN" && chunk->Id() == 1005);            // [1C 00 00 00] 28 11 [08] [07] [4E 01 00 00] // A0

                if (bBroken && (mType != eAePsx && mType != eAePsxDemo))
                {
                    bBroken = false;
                }
                */

                //if (!bBroken)
                {
                    cb(fileName, *chunk);
                }
            }
        }
    }

    void ReadFg1s()
    {
        ForChunksOfType(Oddlib::MakeType('F', 'G', '1', ' '), [&](const std::string&, Oddlib::LvlArchive::FileChunk&)
        {
            // TODO: FG1 parsing
        });
    }

    void ReadFonts()
    {
        ForChunksOfType(Oddlib::MakeType('F', 'o', 'n', 't'), [&](const std::string&, Oddlib::LvlArchive::FileChunk&)
        {
            // TODO: Font parsing
        });
    }

    void ReadAllPaths()
    {
        ForChunksOfType(Oddlib::MakeType('P', 'a', 't', 'h'), [&](const std::string&, Oddlib::LvlArchive::FileChunk&)
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
        ForChunksOfType(Oddlib::MakeType('B', 'i', 't', 's'), [&](const std::string&, Oddlib::LvlArchive::FileChunk& chunk)
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
        ForChunksOfType(Oddlib::MakeType('A', 'n', 'i', 'm'), [&](const std::string& fileName, Oddlib::LvlArchive::FileChunk& chunk)
        {
            Oddlib::AnimSerializer anim(fileName, chunk.Id(), *chunk.Stream());

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
        
       // { DataTest::eAePc,      "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Exoddus" },
        

        /*
        { DataTest::eAePcDemo,  "C:\\Users\\paul\\Desktop\\alive\\all_data\\exoddemo" },
        */
        { DataTest::eAePsx,     "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin" },
       // { DataTest::eAePsx,     "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 2) [SLES-11480].bin" },
        //{ DataTest::eAePsxDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Euro Demo 38 (E) (Track 1) [SCED-01148].bin" },
       
        /*
        { DataTest::eAoPc,      "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Oddysee" },
        { DataTest::eAoPcDemo,  "C:\\Users\\paul\\Desktop\\alive\\all_data\\abeodd" },
        { DataTest::eAoPsx,     "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (E) [SLES-00664].bin" },
        { DataTest::eAoPsxDemo, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (Demo) (E) [SLED-00725].bin" },
        */
    };

    std::vector<std::string> fileFilter;
    //fileFilter.push_back("DUST.BAN");
    //fileFilter.push_back("METAL.BAN");
    //fileFilter.push_back("VAPOR.BAN");
    fileFilter.push_back("DEBRIS00.BAN");
    //fileFilter.push_back("DRPROCK.BAN");
    //fileFilter.push_back("DRPSPRK.BAN");
   // fileFilter.push_back("EVILFART.BAN");
   // fileFilter.push_back("SHELL.BAN");
   // fileFilter.push_back("SQBSMK.BAN");
   // fileFilter.push_back("WELLLEAF.BAN");
    //fileFilter.push_back("EXPLODE.BND");


    // FALLROCK.BAN corrupted frames?
    // ABEHOIST.BAN and some other ABE sprites have blue pixel patches appearing - maybe decompression issue or palt byte swapping?
    // AE broken frames - could it be 1 big image and each offset is a rect within it?

    for (const auto& data : datas)
    {
        const auto it = DataTypeLvlMap.find(data.first);
        if (it == std::end(DataTypeLvlMap))
        {
            // Defined struct is wrong
            abort();
        }
        DataTest dataDecoder(data.first, data.second, *it->second, fileFilter);
    }

    return 0;
}
