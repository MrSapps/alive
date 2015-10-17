#include "SDL.h"
#include "oddlib/stream.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/bits_factory.hpp"
#include "oddlib/ao_bits_pc.hpp"
#include "oddlib/ae_bits_pc.hpp"
#include "oddlib/anim.hpp"
#include "filesystem.hpp"
#include "logger.hpp"
#include <functional>
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
                ReadFg1s(archive);
                ReadFonts(archive);
                ReadAllPaths(archive);
                ReadAllCameras(archive);
                ReadAllAnimations(archive);

                // TODO: Handle sounds/fmvs
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
                    cb(*chunk);
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
                entry->mNumberOfCollisionItems,
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
            if (mType == eAoPc)
            {
                Oddlib::IBits* ptr = nullptr;
                switch (mType)
                {
                case eAoPc:
                    ptr = dynamic_cast<Oddlib::AoBitsPc*>(bits.get());
                    break;

                case eAePc:
                    ptr = dynamic_cast<Oddlib::AeBitsPc*>(bits.get());
                    break;

                default:
                    abort();
                }


                if (!ptr)
                {
                    LOG_ERROR("Wrong camera type constructed");
                    abort();
                }
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
    //DataTest aoPc(DataTest::eAoPc, "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Oddysee", aoLvls);
    DataTest aoPsx(DataTest::eAoPsx, "C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Oddysee (E) [SLES-00664].bin", aoLvls);

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
    //DataTest aePc(DataTest::eAePc, "C:\\Program Files (x86)\\Steam\\SteamApps\\common\\Oddworld Abes Exoddus", aeLvls);

    /* TODO: Check all other data
 
    // AE PC/PSX

    DataTest aePsxCd1("C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin");
    DataTest aePsxCd2("C:\\Users\\paul\\Desktop\\alive\\all_data\\Oddworld - Abe's Exoddus (E) (Disc 2) [SLES-11480].bin");

    // AO Demo PC/PSX

    // AE demo PC/PSX
    */

    return 0;
}
