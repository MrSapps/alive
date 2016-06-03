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
#include <cassert>
#include "jsonxx/jsonxx.h"

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
                Reduce(std::make_unique<Oddlib::LvlArchive>(std::move(stream)), lvl);
            }
            else
            {
                //LOG_WARNING("LVL not found: " << lvl);
               // abort(); // All LVLs in a set must exist
            }
        }
    }

    const std::vector<std::pair<Oddlib::LvlArchive::FileChunk*, std::pair<std::string, bool>>>& Chunks() const
    {
        return mChunks;
    }

    const std::set<std::string>& LvlFileContent(const std::string& name) const
    {
        return mLvlFileContent.find(name)->second;
    }

    bool ALvlWasPsx() const
    {
        return mIsPsx;
    }

private:
    void Reduce(std::unique_ptr<Oddlib::LvlArchive> lvl, const std::string& lvlName)
    {
        
        bool foundCam = false;
        for (auto i = 0u; i < lvl->FileCount(); i++)
        {
            auto file = lvl->FileByIndex(i);
            if (string_util::ends_with(file->FileName(), ".CAM", true))
            {
                auto stream = file->ChunkByType(Oddlib::MakeType('B', 'i', 't', 's'))->Stream();
                mIsPsx = Oddlib::IsPsxCamera(*stream);
                foundCam = true;
                break;
            }

        }

        if (!foundCam)
        {
            abort();
        }
        

        bool chunkTaken = false;
        for (auto i = 0u; i < lvl->FileCount(); i++)
        {
            auto file = lvl->FileByIndex(i);
            mLvlFileContent[lvlName].insert(file->FileName());

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
                            AddChunk(chunk, file->FileName(), mIsPsx);
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

    void AddChunk(Oddlib::LvlArchive::FileChunk* chunk, const std::string& fileName, bool isPsx)
    {
        mChunks.push_back(std::make_pair(chunk, std::make_pair(fileName, isPsx)));
    }

    const std::vector<std::string>& mLvlFiles;
    std::vector<std::string> mFileFilter;

    GameData mGameData;
    FileSystem mFs;

    std::vector<std::pair<Oddlib::LvlArchive::FileChunk*, std::pair<std::string, bool>>> mChunks;
    std::vector<std::unique_ptr<Oddlib::LvlArchive>> mLvls;

    std::map<std::string, std::set<std::string>> mLvlFileContent;
    bool mIsPsx = false;
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
        eAePsxCd1,
        eAePsxCd2,
        eAePsxDemo
    };

    static const char* ToString(eDataType type)
    {
        switch (type)
        {
        case eAoPc:
            return "AoPc";

        case eAoPcDemo:
            return "AoPcDemo";

        case eAoPsx:
            return "AoPsx";

        case eAoPsxDemo:
            return "AoPsxDemo";

        case eAePc:
            return "AePc";

        case eAePcDemo:
            return "AePcDemo";

        case eAePsxCd1:
            return "AePsxCd1";

        case eAePsxCd2:
            return "AePsxCd2";

        case eAePsxDemo:
            return "AePsxDemo";
        }
        return "unknown";
    }

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

    void ForChunksOfType(Uint32 type, std::function<void(const std::string&, Oddlib::LvlArchive::FileChunk&, bool)> cb)
    {
        for (auto chunkPair : mReducer.Chunks())
        {
            auto chunk = chunkPair.first;
            auto fileName = chunkPair.second.first;
            bool isPsx = chunkPair.second.second;
            if (chunk->Type() == type)
            {
                cb(fileName, *chunk, isPsx);
            }
        }
    }

    void ReadFg1s()
    {
        ForChunksOfType(Oddlib::MakeType('F', 'G', '1', ' '), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
        {
            // TODO: FG1 parsing
        });
    }

    void ReadFonts()
    {
        ForChunksOfType(Oddlib::MakeType('F', 'o', 'n', 't'), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
        {
            // TODO: Font parsing
        });
    }

    void ReadAllPaths()
    {
        ForChunksOfType(Oddlib::MakeType('P', 'a', 't', 'h'), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
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
        ForChunksOfType(Oddlib::MakeType('B', 'i', 't', 's'), [&](const std::string&, Oddlib::LvlArchive::FileChunk& chunk, bool )
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
            case eAePsxCd1:
            case eAePsxCd2:
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
        ForChunksOfType(Oddlib::MakeType('A', 'n', 'i', 'm'), [&](const std::string& fileName, Oddlib::LvlArchive::FileChunk& chunk, bool isPsx)
        {
            if (chunk.Id() == 0x1770)
            {
                // ELECWALL.BAN
             //   abort();
            }
            Oddlib::DebugDumpAnimationFrames(fileName, chunk.Id(), *chunk.Stream(), isPsx, ToString(mType));
        });
    }

private:
    eDataType mType;
    LvlFileReducer mReducer;
};



int main(int /*argc*/, char** /*argv*/)
{
    const std::vector<std::string> aoPcLvls =
    {
        "c1.lvl",
        "d1.lvl",
        "d2.lvl",
        "d7.lvl",
        "e1.lvl",
        "e2.lvl",
        "f1.lvl",
        "f2.lvl",
        "f4.lvl",
        "l1.lvl",
        "r1.lvl",
        "r2.lvl",
        "r6.lvl",
        "s1.lvl"
    };

    const std::vector<std::string> aoPsxLvls =
    {
        "e1.lvl",
        "f1.lvl",
        "f2.lvl",
        "f4.lvl",
        "l1.lvl",
        "r1.lvl",
        "s1.lvl",
        "p2\\c1.lvl",
        "p2\\d1.lvl",
        "p2\\d2.lvl",
        "p2\\d7.lvl",
        "p2\\e2.lvl",
        "p2\\r2.lvl",
        "p2\\r6.lvl"
    };

    const std::vector<std::string> aoPcDemoLvls =
    {
        "c1.lvl",
        "r1.lvl",
        "s1.lvl"
    };

    const std::vector<std::string> aoPsxDemoLvls =
    {
        "ABESODSE\\C1.LVL",
        "ABESODSE\\E1.LVL",
        "ABESODSE\\R1.LVL",
        "ABESODSE\\S1.LVL"
    };

    const std::vector<std::string> aePsxCd1Lvls =
    {
        "ba\\ba.lvl",
        "br\\br.lvl",
        "bw\\bw.lvl",
        "cr\\cr.lvl",
        "fd\\fd.lvl",
        "mi\\mi.lvl",
        "ne\\ne.lvl",
        "pv\\pv.lvl",
        "st\\st.lvl",
        "sv\\sv.lvl"
    };

    const std::vector<std::string> aePsxCd2Lvls =
    {
        "ba\\ba.lvl",
        "bm\\bm.lvl",
        "br\\br.lvl",
        "bw\\bw.lvl",
        "cr\\cr.lvl",
        "fd\\fd.lvl",
        "mi\\mi.lvl",
        "ne\\ne.lvl",
        "pv\\pv.lvl",
        "st\\st.lvl",
        "sv\\sv.lvl"
    };

    const std::vector<std::string> aePcLvls =
    {
        "ba.lvl",
        "bm.lvl",
        "br.lvl",
        "bw.lvl",
        "cr.lvl",
        "fd.lvl",
        "mi.lvl",
        "ne.lvl",
        "pv.lvl",
        "st.lvl",
        "sv.lvl"
    };

    const std::vector<std::string> aePcDemoLvls =
    {
        "cr.lvl",
        "mi.lvl",
        "st.lvl"
    };

    const std::vector<std::string> aePsxDemoLvls =
    {
        "ABE2\\CR.LVL",
        "ABE2\\MI.LVL",
        "ABE2\\ST.LVL"
    };

    const std::map<DataTest::eDataType, const std::vector<std::string>*> DataTypeLvlMap =
    {
        { DataTest::eAoPc, &aoPcLvls },
        { DataTest::eAoPsx, &aoPsxLvls },
        { DataTest::eAePc, &aePcLvls },
        { DataTest::eAePsxCd1, &aePsxCd1Lvls },
        { DataTest::eAePsxCd2, &aePsxCd2Lvls },
        { DataTest::eAoPcDemo, &aoPcDemoLvls },
        { DataTest::eAoPsxDemo, &aoPsxDemoLvls },
        { DataTest::eAePcDemo, &aePcDemoLvls },
        { DataTest::eAePsxDemo, &aePsxDemoLvls }
    };

    const std::string dataPath = "F:\\Data\\alive\\all_data\\";

    const std::vector<std::pair<DataTest::eDataType, std::string>> datas =
    {
        { DataTest::eAePc, dataPath + "Oddworld Abes Exoddus" },
        { DataTest::eAePcDemo, dataPath + "exoddemo" },
        { DataTest::eAePsxDemo, dataPath + "Euro Demo 38 (E) (Track 1) [SCED-01148].bin" },
        { DataTest::eAePsxCd1, dataPath + "Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin" },
        { DataTest::eAePsxCd2, dataPath + "Oddworld - Abe's Exoddus (E) (Disc 2) [SLES-11480].bin" },
        
        { DataTest::eAoPc, dataPath + "Oddworld Abes Oddysee" },
        { DataTest::eAoPcDemo, dataPath + "abeodd" },
        { DataTest::eAoPsxDemo, dataPath + "Oddworld - Abe's Oddysee (Demo) (E) [SLED-00725].bin" },
        { DataTest::eAoPsx, dataPath + "Oddworld - Abe's Oddysee (E) [SLES-00664].bin" },
    };


    // Used to create the "base" json db's. E.g the list of LVL's each data set has, unique collection of anim id's
    // and what BAN/BND's they live in. And which LVL's each BAN/BND lives in.
    class Db
    {
    public:
        Db() = default;

        void Merge(DataTest::eDataType eType, const std::string& resourcePath, const std::vector<std::string>& lvls)
        {
            LvlFileReducer reducer(resourcePath, lvls);

            for (const auto& lvl : lvls)
            {
                AddLvlMapping(eType, lvl, reducer.ALvlWasPsx(), reducer.LvlFileContent(lvl));
            }

            for (auto& chunkPair : reducer.Chunks())
            {
                Oddlib::LvlArchive::FileChunk* chunk = chunkPair.first;
                if (chunk->Type() == Oddlib::MakeType('A', 'n', 'i', 'm'))
                {
                    AddRes(chunk->Id(), chunkPair.second.first);

                    Oddlib::AnimSerializer as(*chunk->Stream(), reducer.ALvlWasPsx());
                    AddNumAnimationsMapping(chunk->Id(), static_cast<Uint32>(as.Animations().size()));
                }
            }
        }

        void ToJson()
        {
            jsonxx::Array resources;

            for (const auto& animData : mAnimResIds)
            {
                const auto& animFiles = animData.second;
                const auto id = animData.first;
                for (const auto& animFile : animFiles)
                {
                    jsonxx::Object animObj;
                    animObj << "id" << std::to_string(id);
                    animObj << "file" << animFile;

                    auto numAnims = mNumberOfAnimsMap[animData.first];
                    //animObj << "numAnims" << numAnims;

                    jsonxx::Array anims;

                    for (auto i = 0u; i < numAnims; i++)
                    {
                        jsonxx::Object anim;

                        // Generated globally unique name
                        anim 
                            << "name" 
                            << animFile + "_" + std::to_string(id) + "_" + std::to_string(i + 1);

                        // Guessed blending mode
                        anim << "blend_mode" << "1";

                        // TODO: Semi trans flag
                        // TODO: pallet res id?

                        anims << anim;

                        // TODO: Index array for each data set

                    }
                    animObj << "anims" << anims;

                    resources << animObj;
                }
            }


            // Map of which LVL's live in what data set
            for (const auto& dataSetPair : mLvlToDataSetMap)
            {
                jsonxx::Object dataSet;
                const std::string strName = DataTest::ToString(dataSetPair.first);
                dataSet << "data_set_name" << strName;
                dataSet << "is_psx" << std::string(dataSetPair.second.begin()->second.mIsPsx ? "true" : "false");

                jsonxx::Array lvlsArray;

            
                for (const auto& lvlData : dataSetPair.second)
                {
                    jsonxx::Object lvlObj;
                    lvlObj << "name" << lvlData.first;

                    jsonxx::Array lvlContent;
                    const std::set<std::string>& content = lvlData.second.mFiles;
                    for (const auto& lvlFile : content)
                    {
                        lvlContent << lvlFile;
                    }
                    lvlObj << "files" << lvlContent;
                    lvlsArray << lvlObj;
                }
                dataSet << "lvls" << lvlsArray;
            

                resources << dataSet;
            }

            std::ofstream jsonFile("test.json");
            if (!jsonFile.is_open())
            {
                abort();
            }
            jsonFile << resources.json().c_str() << std::endl;
        }

        private:

        void AddNumAnimationsMapping(Uint32 resId, Uint32 numAnims)
        {
            // TODO: Must store num anims for each dataset for saving indexes
            // in ToJson

            auto it = mNumberOfAnimsMap.find(resId);
            if (it == std::end(mNumberOfAnimsMap))
            {
                mNumberOfAnimsMap[resId] = numAnims;
            }
            else
            {
                // TODO: If not equal then should probably just take the biggest value
                if (resId == 314) return; // BLOOD.BAN and SPARKS.BAN have same id but diff number of anims
                if (resId == 2020) return; // SLAM.BAN and SLAMVLTS.BAN

                if (resId == 10) return; // ABEBSIC.BAN 47 VS 51 of AePcDemo
                if (resId == 55) return; // ABEBSIC1.BAN 11 VS 10 of AePcDemo
                if (resId == 43) return; // ABEEDGE.BAN 43 VS 10 of AePcDemo
                if (resId == 28) return; // ABEKNOKZ.BAN 2 VS 3 of AePcDemo
                if (resId == 518) return; // MUDTORT.BAN 3 VS 2
                if (resId == 604) return;
                if (resId == 605) return;
                if (resId == 45) return; // ABEDOOR.BAN 4 vs 2 ao demo
                if (resId == 26) return; // 4 VS 5
                if (resId == 203) return; // 1 VS 2
                if (resId == 511) return; // 3 VS 6
                if (resId == 130) return; // 15 vs 10
                if (resId == 576) return; // 2 vs 3
                if (resId == 1000) return; //1 vs 2
                if (resId == 800) return; // 24 vs 5
                if (resId == 200) return; // 3 vs 14
                if (resId == 700) return; // 18 vs 12
                if (resId == 711) return; // 2 vs 1
                if (resId == 701) return; // 3 vs 2
                if (resId == 704) return; // 2 vs 1
                if (resId == 113) return; // 2 vs 1
                if (resId == 600) return; // 22 vs 13
                if (resId == 2013) return; // 2 vs 1
                if (resId == 27) return; // 2 vs 3
                if (resId == 16) return;
                if (resId == 415) return;
                if (resId == 100) return;
                if (resId == 6000) return;
                if (resId == 2012) return;
                if (resId == 2019) return;
                if (resId == 2018) return;
                if (resId == 2007) return;
                if (resId == 337) return;
                if (resId == 2026) return;
                if (resId == 508) return;
                if (resId == 8002) return;

                assert(it->second == numAnims);
            }
        }

        void AddRes(Uint32 resId, const std::string& resFileName)
        {
            auto it = mAnimResIds.find(resId);
            if (it == std::end(mAnimResIds))
            {
                mAnimResIds[resId] = { resFileName };
            }
            else
            {
                it->second.insert(resFileName);
            }

        }

        void AddLvlMapping(DataTest::eDataType eType, const std::string& lvlName, bool isPsx, const std::set<std::string>& files)
        {
            auto it = mLvlToDataSetMap[eType].find(lvlName);
            if (it == std::end(mLvlToDataSetMap[eType]))
            {
                mLvlToDataSetMap[eType][lvlName] = LvlData{ isPsx, files };
            }
        }

    private:
        // Map of Anim res ids to files that contain them
        std::map<Uint32, std::set<std::string>> mAnimResIds; // E.g 25 -> [ABEBLOW.BAN, XYZ.BAN]

        // ResId to number of anims in that ResId
        std::map<Uint32, Uint32> mNumberOfAnimsMap; // E.g 25 -> 3, because it has flying head, arm and leg anims

        // Map of which LVL's live in what data set
        struct LvlData
        {
            bool mIsPsx;
            std::set<std::string> mFiles;
        };

        std::map<DataTest::eDataType, std::map<std::string, LvlData>> mLvlToDataSetMap; // E.g AoPc, AoPsx, AoPcDemo, AoPsxDemo -> R1.LVL
    };

    Db db;

    for (const auto& data : datas)
    {
        const auto it = DataTypeLvlMap.find(data.first);
        if (it == std::end(DataTypeLvlMap))
        {
            // Defined struct is wrong
            abort();
        }
        db.Merge(data.first, data.second, *it->second);
    }

    db.ToJson();

    return 0;
}
