#define _CRT_SECURE_NO_WARNINGS

#include "SDL.h"
#include "oddlib/stream.hpp"
#include "oddlib/lvlarchive.hpp"
#include "oddlib/bits_factory.hpp"
#include "oddlib/bits_ao_pc.hpp"
#include "oddlib/bits_ae_pc.hpp"
#include "oddlib/bits_psx.hpp"
#include "oddlib/anim.hpp"
#include "logger.hpp"
#include "stdthread.h"
#include "msvc_sdl_link.hpp"
#include <cassert>
#include "jsonxx/jsonxx.h"
#include "resourcemapper.hpp"
#include "phash.hpp"
#include "oddlib/audio/vab.hpp"
#include "gridmap.hpp"
#include "gamefilesystem.hpp"

void HackToReferencePrintEtc()
{
    fprintf(stderr, "%d", 1);
    // Add other unresolved functions
}

enum eDataSetType
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

bool IsPsx(eDataSetType type)
{
    switch (type)
    {
    case eAoPc:
    case eAoPcDemo:
    case eAePc:
    case eAePcDemo:
        return false;

    case eAoPsx:
    case eAoPsxDemo:
    case eAePsxCd1:
    case eAePsxCd2:
    case eAePsxDemo:
        return true;
    }
    abort();
}

bool IsAo(eDataSetType type)
{
    switch (type)
    {
    case eAoPc:
    case eAoPcDemo:
    case eAoPsx:
    case eAoPsxDemo:
        return true;

    case eAePsxCd1:
    case eAePsxCd2:
    case eAePsxDemo:
    case eAePc:
    case eAePcDemo:
        return false;
    }
    abort();
}

const char* ToString(eDataSetType type)
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
    abort();
}

struct LvlFileChunk
{
    eDataSetType mDataSet;
    std::string mLvlName;
    std::string mFileName;
    Oddlib::LvlArchive::FileChunk* mChunk;
    std::vector<u8> mData;
};

struct DeDuplicatedLvlChunk
{
    std::unique_ptr<LvlFileChunk> mChunk;
    std::unique_ptr<Oddlib::AnimationSet> mAnimSet;
    std::vector<std::unique_ptr<LvlFileChunk>> mDuplicates;
};

bool CompareFrames(const Oddlib::Animation::Frame& frame1, const Oddlib::Animation::Frame& frame2)
{
    const Uint64 h1 = pHash(frame1.mFrame);
    const Uint64 h2 = pHash(frame2.mFrame);
    const u32 distance = hamming_distance(h1, h2);

    if (distance <= 22)
    {
        return true;
    }
    else
    {
        /*
        const u32 w = frame1.mFrame->w + frame2.mFrame->w;
        const u32 h = std::max(frame1.mFrame->h, frame2.mFrame->h);

        SDL_SurfacePtr combined(SDL_CreateRGBSurface(0,
            w, h,
            frame1.mFrame->format->BitsPerPixel,
            frame1.mFrame->format->Rmask,
            frame1.mFrame->format->Gmask,
            frame1.mFrame->format->Bmask,
            frame1.mFrame->format->Amask));

        SDL_BlitSurface(frame1.mFrame, NULL, combined.get(), NULL);

        SDL_Rect dstRect = { frame1.mFrame->w, 0, frame2.mFrame->w, frame2.mFrame->h };
        SDL_BlitSurface(frame2.mFrame, NULL, combined.get(), &dstRect);

        SDLHelpers::SaveSurfaceAsPng(("distance_" + std::to_string(distance) + ".png").c_str(), combined.get());
        */
    }
  
    // TODO: figure out if the 2 frames are similar enough to be called the same
    return false;
}

struct LocationFileInfo
{
    bool operator < (const LocationFileInfo& other) const
    {
        return std::tie(mFileName, mChunkId, mAnimIndex) < std::tie(other.mFileName, other.mChunkId, other.mAnimIndex);
    }

    std::string mFileName;
    u32 mChunkId;
    u32 mAnimIndex;
};

struct DeDuplicatedAnimation
{
private:
    void HandleLvlChunk(std::map<eDataSetType, std::set<LocationFileInfo>>& vec, const LvlFileChunk& chunk, u32 animIdx)
    { 
        LocationFileInfo fileInfo = {};
        fileInfo.mChunkId = chunk.mChunk->Id();
        fileInfo.mFileName = chunk.mFileName;
        fileInfo.mAnimIndex = animIdx;
        vec[chunk.mDataSet].insert(fileInfo);
    }

    void HandleDeDuplicatedChunk(std::map<eDataSetType, std::set<LocationFileInfo>>& vec, const DeDuplicatedLvlChunk& chunk, u32 animIdx)
    {
        HandleLvlChunk(vec, *chunk.mChunk, animIdx);
        for (const std::unique_ptr<LvlFileChunk>& lvlChunk : chunk.mDuplicates)
        {
            HandleLvlChunk(vec, *lvlChunk, animIdx);
        }
    }

public:
    const Oddlib::Animation* mAnimation;
    u32 mAnimationIndex;
    DeDuplicatedLvlChunk* mContainingChunk;
    std::vector<std::pair<u32, DeDuplicatedLvlChunk*>> mDuplicates;

    std::map<eDataSetType, std::set<LocationFileInfo>> Locations()
    {
        std::map<eDataSetType, std::set<LocationFileInfo>> r;
        HandleDeDuplicatedChunk(r, *mContainingChunk, mAnimationIndex);
        for (const auto& chunkPair : mDuplicates)
        {
            HandleDeDuplicatedChunk(r, *chunkPair.second, chunkPair.first);
        }
        return r;
    }

    bool operator == (const DeDuplicatedAnimation& other)
    {
        if (mAnimation->NumFrames() == other.mAnimation->NumFrames() &&
            mAnimation->Fps() == other.mAnimation->Fps())
        {
            for (s32 i = 0; i < mAnimation->NumFrames(); i++)
            {
                const Oddlib::Animation::Frame& frame1 = mAnimation->GetFrame(i);
                const Oddlib::Animation::Frame& frame2 = other.mAnimation->GetFrame(i);
                // Compare if frame images are "similar"
                if (!CompareFrames(frame1, frame2))
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};

bool ChunksAreEqual(const LvlFileChunk& c1, const LvlFileChunk& c2)
{
    // Here just compare content and not type or id, since 2 chunks may have the same data
    // but have changed ID's across games
    return c1.mData == c2.mData;
}

class LvlFileChunkReducer
{
public:
    LvlFileChunkReducer(const LvlFileChunkReducer&) = delete;
    LvlFileChunkReducer& operator = (const LvlFileChunkReducer&) = delete;
    LvlFileChunkReducer() = default;

    void MergeReduceLvlChunks(IFileSystem& parentFs, const std::string& resourcePath, const std::vector<std::string>& lvlFiles, eDataSetType dataSet)
    {
        auto fs = IFileSystem::Factory(parentFs, resourcePath);
        if (!fs)
        {
            throw std::runtime_error("FS init failed");
        }

        for (const auto& lvl : lvlFiles)
        {
            LOG_INFO("Opening LVL: " << lvl);
            auto stream = fs->Open(lvl);
            if (stream)
            {
                MergeReduceLvlChunks(std::make_unique<Oddlib::LvlArchive>(std::move(stream)), lvl, dataSet);
            }
            else
            {
                LOG_WARNING("LVL not found: " << lvl);
                abort();
            }
        }
    }

    std::vector<std::unique_ptr<DeDuplicatedLvlChunk>>& UniqueChunks() { return mDeDuplicatedLvlFileChunks; }
    const std::map<eDataSetType, std::map<std::string, std::set<std::string>>>& LvlContent() const { return mLvlToDataSetMap; }
private:
    void MergeReduceLvlChunks(std::unique_ptr<Oddlib::LvlArchive> lvl, const std::string& lvlName, eDataSetType dataSet)
    {
        for (auto i = 0u; i < lvl->FileCount(); i++)
        {
            auto file = lvl->FileByIndex(i);
      
            AddLvlMapping(dataSet, lvlName, file->FileName());
            if (
                string_util::ends_with(file->FileName(), ".VH") ||
                string_util::ends_with(file->FileName(), ".VB") ||
                string_util::ends_with(file->FileName(), ".BSQ")
                )
            {
                for (auto j = 0u; j < file->ChunkCount(); j++)
                {
                    auto chunk = file->ChunkByIndex(j);

                    //if (chunk->Type() == Oddlib::MakeType('A', 'n', 'i', 'm')) // Only care about Anim resources at the moment
                    {
                        bool deDuplicatedChunkAlreadyExists = false;

                        // Read this chunk only once
                        auto chunkInfo = std::make_unique<LvlFileChunk>();
                        chunkInfo->mChunk = chunk;
                        chunkInfo->mDataSet = dataSet;
                        chunkInfo->mFileName = file->FileName();
                        chunkInfo->mLvlName = lvlName;
                        chunkInfo->mData = chunk->ReadData();

                        // Find if this chunk exists
                        for (std::unique_ptr<DeDuplicatedLvlChunk>& deDuplicatedChunk : mDeDuplicatedLvlFileChunks)
                        {
                            if (ChunksAreEqual(*deDuplicatedChunk->mChunk, *chunkInfo))
                            {
                                // Since it exists add the chunk as a duplicate
                                deDuplicatedChunkAlreadyExists = true;
                                deDuplicatedChunk->mDuplicates.push_back(std::move(chunkInfo));
                                deDuplicatedChunk->mDuplicates.back()->mData = std::vector<u8>(); // Don't keep many copies of the same buffer
                                break;
                            }
                        }

                        if (!deDuplicatedChunkAlreadyExists)
                        {
                            // Otherwise add it as a unique chunk
                            auto deDuplicatedChunk = std::make_unique<DeDuplicatedLvlChunk>();
                            deDuplicatedChunk->mChunk = std::move(chunkInfo);
                            mDeDuplicatedLvlFileChunks.push_back(std::move(deDuplicatedChunk));
                        }
                    }
                }
            }
        }

        AddLvl(std::move(lvl));
    }

    void AddLvl(std::unique_ptr<Oddlib::LvlArchive> lvl)
    {
        mLvls.emplace_back(std::move(lvl));
    }

    // Map of which LVL's live in what data set
    std::map<eDataSetType, std::map<std::string, std::set<std::string>>> mLvlToDataSetMap; // E.g AoPc, AoPsx, AoPcDemo, AoPsxDemo -> R1.LVL

    void AddLvlMapping(eDataSetType eType, const std::string& lvlName, const std::string& file)
    {
        mLvlToDataSetMap[eType][lvlName].insert(file);
    }

    // Open LVLS (required because chunks must have the parent lvl in scope)
    std::vector<std::unique_ptr<Oddlib::LvlArchive>> mLvls;

    // List of unique lvl chunks with links to its duplicates
    std::vector<std::unique_ptr<DeDuplicatedLvlChunk>> mDeDuplicatedLvlFileChunks;
};

/*
class DataTest
{
public:
    DataTest(IFileSystem& fs, eDataSetType eType, const std::string& resourcePath, const std::vector<std::string>& lvls, eDataSetType dataSet)
        : mType(eType), 
          mReducer(fs, resourcePath, lvls, dataSet)
    {
        //ReadAllAnimations();
        //ReadFg1s();
        //ReadFonts();
        //ReadAllPaths();
        //ReadAllCameras();
        // TODO: Handle sounds/fmvs
    }

 
    void ForChunksOfType(u32 type, std::function<void(const std::string&, Oddlib::LvlArchive::FileChunk&, bool)> cb)
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
        ForChunksOfType(Oddlib::MakeType("FG1 "), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
        {
            // TODO: FG1 parsing
        });
    }

    void ReadFonts()
    {
        ForChunksOfType(Oddlib::MakeType("Font"), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
        {
            // TODO: Font parsing
        });
    }
    */

    /*
    void ReadAllPaths()
    {
        ForChunksOfType(Oddlib::MakeType("Path"), [&](const std::string&, Oddlib::LvlArchive::FileChunk&, bool)
        {
            // TODO: Load the game data json for the required hard coded data to load the path
            Oddlib::Path path(*chunk.Stream(),
                entry->mCollisionDataOffset,
                entry->mObjectIndexTableOffset,
                entry->mObjectDataOffset,
                entry->mMapXSize,
                entry->mMapYSize);
        });
    }

    void ReadAllCameras()
    {
        ForChunksOfType(Oddlib::MakeType("Bits"), [&](const std::string&, Oddlib::LvlArchive::FileChunk& chunk, bool )
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
        ForChunksOfType(Oddlib::MakeType("Anim"), [&](const std::string& fileName, Oddlib::LvlArchive::FileChunk& chunk, bool isPsx)
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
    eDataSetType mType;
    LvlFileChunkReducer mReducer;
};
*/

static int MaxW(const Oddlib::Animation& anim)
{
    int ret = 0;
    for (s32 i = 0; i < anim.NumFrames(); i++)
    {
        if (anim.GetFrame(i).mFrame->w > ret)
        {
            ret = anim.GetFrame(i).mFrame->w;
        }
    }
    return ret;
}

static int MaxH(const Oddlib::Animation& anim)
{
    int ret = 0;
    for (s32 i = 0; i < anim.NumFrames(); i++)
    {
        if (anim.GetFrame(i).mFrame->h > ret)
        {
            ret = anim.GetFrame(i).mFrame->h;
        }
    }
    return ret;
}

#include <vorbis/vorbisenc.h>

//#define READ 1024
//signed char readbuffer[READ * 4]; /* out of the data segment, not the stack */

#include <oddlib/audio/Soundbank.h>
#include <oddlib/audio/SequencePlayer.h>

class OggDecoder
{
public:

};

class OggEncoder
{
public:

    void InitEncoder(ResourceLocator& locator)
    {
        vorbis_info_init(&vi);

        /*int ret =*/ vorbis_encode_init_vbr(&vi, 2, 44100, 0.1f);

        // Appears in the file info to show which app created it
        vorbis_comment_init(&vc);
        vorbis_comment_add_tag(&vc, "ENCODER", "A.L.I.V.E");

        /* set up the analysis state and auxiliary encoding storage */
        vorbis_analysis_init(&vd, &vi);
        vorbis_block_init(&vd, &vb);

        /* set up our packet->stream encoder */
        /* pick a random serial number; that way we can more likely build
        chained streams just by concatenation */
        srand(static_cast<unsigned int>(time(NULL)));
        ogg_stream_init(&os, rand());

        /* Vorbis streams begin with three headers; the initial header (with
        most of the codec setup parameters) which is mandated by the Ogg
        bitstream spec.  The second header holds any comment fields.  The
        third header holds the bitstream codebook.  We merely need to
        make the headers, then pass them to libvorbis one at a time;
        libvorbis handles the additional Ogg bitstream constraints */


        FILE* output = fopen("F:\\Data\\alive\\alive\\test.ogg", "wb");

        ogg_packet header;
        ogg_packet header_comm;
        ogg_packet header_code;

        vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
        ogg_stream_packetin(&os, &header); // automatically placed in its own page
        ogg_stream_packetin(&os, &header_comm);
        ogg_stream_packetin(&os, &header_code);

        /* This ensures the actual
        * audio data will start on a new page, as per spec
        */
        /*int result =*/ ogg_stream_flush(&os, &og);

        fwrite(og.header, 1, og.header_len, output);
        fwrite(og.body, 1, og.body_len, output);

        std::unique_ptr<IMusic> music = locator.LocateMusic("d1_D1SEQ_0_D1SNDFX_AoPsx");
        if (!music)
        {
            return;
        }

        AliveAudio audio;
        SequencePlayer seqPlayer(audio);
        auto soundBank = std::make_unique<AliveAudioSoundbank>(*music->mVab, audio);
        audio.SetSoundbank(std::move(soundBank));
        seqPlayer.LoadSequenceStream(*music->mSeqData);
        seqPlayer.PlaySequence();

        for (int i = 0; i < 7000; i++)
        {
            signed char buffer[1024 * 4] = {};

            seqPlayer.PlayerThreadFunction();

            audio.Play((u8*)buffer, 1024 * 4);

            Consume((float*)buffer, 1024 * 4, output);
        }
      
        /* clean up and exit.  vorbis_info_clear() must be called last */
        ogg_stream_clear(&os);
        vorbis_block_clear(&vb);
        vorbis_dsp_clear(&vd);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);

    }

    // float buffer
    void Consume(float* readbuffer, long bufferSizeInBytes, FILE* out)
    {
        if (bufferSizeInBytes == 0)
        {
            // Mark as the last frame
            vorbis_analysis_wrote(&vd, 0);
        }
        else
        {
            /* data to encode */
            int numFloats = bufferSizeInBytes / sizeof(float);
            const auto numChannels = 2;
            const auto floatsPerChannel = numFloats / numChannels;

            /* expose the buffer to submit data */
            float** buffer = vorbis_analysis_buffer(&vd, floatsPerChannel); // 32bit float buffer

            int pos = 0;
            for (auto i = 0; i <floatsPerChannel; i++)
            {
                buffer[0][i] = readbuffer[pos++];
                buffer[1][i] = readbuffer[pos++];
            }

            /* tell the library how much we actually submitted */
            vorbis_analysis_wrote(&vd, floatsPerChannel);
        }

        /* vorbis does some data preanalysis, then divvies up blocks for
        more involved (potentially parallel) processing.  Get a single
        block for encoding now */
        while (vorbis_analysis_blockout(&vd, &vb) == 1)
        {
            /* analysis, assume we want to use bitrate management */
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);

            while (vorbis_bitrate_flushpacket(&vd, &op))
            {
                /* weld the packet into the bitstream */
                ogg_stream_packetin(&os, &op);

                /* write out pages (if any) */
                int result = ogg_stream_pageout(&os, &og);
                if (result == 0)
                {
                    // Error or not enough data, ogg_stream_flush can force a new page
                    break;
                }

                fwrite(og.header, 1, og.header_len, out);
                fwrite(og.body, 1, og.body_len, out);
            }
        }
    }

private:
    ogg_stream_state os; // take physical pages, weld into a logical stream of packets
    ogg_page         og; // one Ogg bitstream page.  Vorbis packets are inside
    ogg_packet       op; // one raw packet of data for decode
    vorbis_info      vi; // struct that stores all the static vorbis bitstream settings
    vorbis_comment   vc; // struct that stores all the user comments
    vorbis_dsp_state vd; // central working state for the packet->PCM decoder
    vorbis_block     vb; // local working space for packet->PCM decode
};

void LoopFlagTest()
{
    
    //Vab

    Oddlib::LvlArchive lvl("F:\\Data\\alive\\all_data\\Oddworld Abes Exoddus\\mi.lvl");

    Oddlib::LvlArchive::File* vh = lvl.FileByName("MINES.VH");
    Oddlib::LvlArchive::File* vb = lvl.FileByName("MINES.VB");

    auto stream = std::make_unique<Oddlib::FileStream>("F:\\Data\\alive\\all_data\\Oddworld Abes Exoddus\\sounds.dat", Oddlib::IStream::ReadMode::ReadOnly);
    
    Vab vab;
    vab.ReadVh(*vh->ChunkByIndex(0)->Stream(), false);
    vab.ReadVb(*vb->ChunkByIndex(0)->Stream(), false, true, stream.get());

    stream = nullptr;

    FILE* f = fopen("F:\\Data\\alive\\all_data\\Oddworld Abes Exoddus\\sounds.dat", "r+b");

    for (AEVh& rec : vab.iOffs)
    {
        fseek(f, rec.iFileOffset, 0);

        char b[3] = { 1, 1, 1};

        for (u32  i = 0; i < 14; i++)
        {
            fwrite(b, 1, 1, f);
        }
    }

    fclose(f);
}

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

    const std::map<eDataSetType, const std::vector<std::string>*> DataTypeLvlMap =
    {
        { eDataSetType::eAoPc, &aoPcLvls },
        { eDataSetType::eAoPsx, &aoPsxLvls },
        { eDataSetType::eAePc, &aePcLvls },
        { eDataSetType::eAePsxCd1, &aePsxCd1Lvls },
        { eDataSetType::eAePsxCd2, &aePsxCd2Lvls },
        { eDataSetType::eAoPcDemo, &aoPcDemoLvls },
        { eDataSetType::eAoPsxDemo, &aoPsxDemoLvls },
        { eDataSetType::eAePcDemo, &aePcDemoLvls },
        { eDataSetType::eAePsxDemo, &aePsxDemoLvls }
    };

    const std::string dataPath = "F:\\Data\\alive\\all_data\\";

    const std::vector<std::pair<eDataSetType, std::string>> datas =
    {
        { eDataSetType::eAePc, dataPath + "Oddworld Abes Exoddus" },
        { eDataSetType::eAePcDemo, dataPath + "exoddemo" },
        { eDataSetType::eAePsxDemo, dataPath + "Euro Demo 38 (E) (Track 1) [SCED-01148].bin" },
        { eDataSetType::eAePsxCd1, dataPath + "Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin" },
        { eDataSetType::eAePsxCd2, dataPath + "Oddworld - Abe's Exoddus (E) (Disc 2) [SLES-11480].bin" },

        { eDataSetType::eAoPc, dataPath + "Oddworld Abes Oddysee" },
        { eDataSetType::eAoPcDemo, dataPath + "abeodd" },
        { eDataSetType::eAoPsxDemo, dataPath + "Oddworld - Abe's Oddysee (Demo) (E) [SLED-00725].bin" },
        { eDataSetType::eAoPsx, dataPath + "Oddworld - Abe's Oddysee (E) [SLES-00664].bin" },
    };


    // Used to create the "base" json db's. E.g the list of LVL's each data set has, unique collection of anim id's
    // and what BAN/BND's they live in. And which LVL's each BAN/BND lives in.
    class Db
    {
    private:
        std::vector<std::unique_ptr<DeDuplicatedAnimation>> mDeDuplicatedAnimations;
    public:
        Db() = default;
        Db(Db&&) = delete;
        Db& operator = (Db&&) = delete;

        void DumpAePsxDemoCameras(const std::vector<std::string>& lvls)
        {
            for (auto& lvlName : lvls)
            {
                const std::string lvlPath = "F:\\Data\\alive\\all_data\\exoddemo\\" + lvlName;
                auto stream = std::make_unique<Oddlib::FileStream>(lvlPath, Oddlib::IStream::ReadMode::ReadOnly);
                Oddlib::LvlArchive archive(std::move(stream));
                for (u32 i = 0; i < archive.FileCount(); i++)
                {
                    Oddlib::LvlArchive::File* file = archive.FileByIndex(i);
                    for (u32 j = 0; j < file->ChunkCount(); j++)
                    {
                        Oddlib::LvlArchive::FileChunk* chunk = file->ChunkByIndex(j);
                        if (chunk->Type() == Oddlib::MakeType("Bits"))
                        {
                            auto bitsStream = chunk->Stream();
                            auto bits = Oddlib::MakeBits(*bitsStream, nullptr);
                            bits->Save();
                        }
                    }
                }
            }
        }

        void DumpFg1(IFileSystem& gameFs, const std::string& resourcePath, const std::vector<std::string>& lvls, eDataSetType dataSet)
        {
            auto fs = IFileSystem::Factory(gameFs, resourcePath);
            if (!fs)
            {
                throw std::runtime_error("FS init failed");
            }

            auto dataSetName = ToString(dataSet);

            for (auto& lvlName : lvls)
            {
                auto stream = fs->Open(lvlName);
                Oddlib::LvlArchive archive(std::move(stream));
                for (u32 i = 0; i < archive.FileCount(); i++)
                {
                    Oddlib::LvlArchive::File* file = archive.FileByIndex(i);
                    for (u32 j = 0; j < file->ChunkCount(); j++)
                    {
                        Oddlib::LvlArchive::FileChunk* chunk = file->ChunkByIndex(j);
                        if (chunk->Type() == Oddlib::MakeType("Bits"))
                        {
                            auto bitsStream = chunk->Stream();

                            auto fg1Chunk = file->ChunkByType(Oddlib::MakeType("FG1 "));
                            if (fg1Chunk)
                            {
                                auto fg1Stream = fg1Chunk->Stream();
                                auto bits = Oddlib::MakeBits(*bitsStream, fg1Stream.get());
                                bits->GetFg1()->Save(dataSetName);
                            }
                        }
                    }
                }
            }
        }

        void DumpPaths(IFileSystem& gameFs, eDataSetType dataSet, const std::string& resourcePath, const std::vector<std::string>& lvlFiles)
        {
            ResourceMapper mapper(gameFs, "{GameDir}/data/resources.json");

            auto fs = IFileSystem::Factory(gameFs, resourcePath);
            if (!fs)
            {
                throw std::runtime_error("FS init failed");
            }

            for (const auto& lvl : lvlFiles)
            {
                LOG_INFO("Opening LVL: " << lvl);
                auto stream = fs->Open(lvl);
                if (stream)
                {
                    HandleLvl(mapper, std::make_unique<Oddlib::LvlArchive>(std::move(stream)), lvl, dataSet);
                }
                else
                {
                    LOG_WARNING("LVL not found: " << lvl);
                    abort();
                }
            }
        }

        void HandleLvl(ResourceMapper& mapper, std::unique_ptr<Oddlib::LvlArchive> archive, const std::string& lvl, eDataSetType eType)
        {
            for (u32 i = 0; i < archive->FileCount(); i++)
            {
                Oddlib::LvlArchive::File* file = archive->FileByIndex(i);
                for (u32 j = 0; j < file->ChunkCount(); j++)
                {
                    Oddlib::LvlArchive::FileChunk* chunk = file->ChunkByIndex(j);
                    if (chunk->Type() == Oddlib::MakeType("Path"))
                    {
                        // Get the data from resource.json about hard coded path info
                        std::string baseName = file->FileName();
                        string_util::replace_all(baseName, ".BND", "");
                        const std::string genResName = baseName + "_" + std::to_string(chunk->Id());
                        const ResourceMapper::PathMapping* pathData = mapper.FindPath(genResName.c_str());
                        if (!pathData)
                        {
                            abort();
                        }

                        // Check the lvl/data set we have matches whats in the json
                        bool found = false;
                        for (const ResourceMapper::PathLocation& loc : pathData->mLocations)
                        {
                            if (loc.mDataSetName == ToString(eType) && loc.mDataSetFileName == file->FileName())
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            // A legal case in AePsxCd1/2 split data
                            LOG_ERROR("Path " << genResName << " not found in lvl " << lvl << " for " << ToString(eType));
                            //continue;
                            abort();
                        }

                        // TODO: Figure out if the lines using alt prev/next are even used in this path, seems like they probably are not.
                        if (baseName == "BMPATH") continue; 

                        // Load the path block
                        auto pathStream = chunk->Stream();
                        Oddlib::Path path(*pathStream, pathData->mCollisionOffset, pathData->mIndexTableOffset, pathData->mObjectOffset, pathData->mNumberOfScreensX, pathData->mNumberOfScreensY, IsAo(eType));
                        CheckSecondLinkIsNotUsed(path.CollisionItems());
                    }
                }
            }
        }

        void CheckSecondLinkIsNotUsed(const std::vector<Oddlib::Path::CollisionItem>& items)
        {

            for (const Oddlib::Path::CollisionItem& item : items)
            {
                CollisionLine::ToType(item.mType);
                if (   item.mType != CollisionLine::eBulletWall
                    && item.mType != CollisionLine::eArt
                    && item.mType != CollisionLine::eCeiling
                    && item.mType != CollisionLine::eWallRight
                    && item.mType != CollisionLine::eFlyingSligCeiling)
                {
                    if (item.mLinks[1].mNext != -1 || item.mLinks[1].mPrevious != -1)
                    {
                       // abort();
                    }
                }
            }
        }

        void MergeDuplicatedLvlChunks(IFileSystem& fs, eDataSetType eType, const std::string& resourcePath, const std::vector<std::string>& lvls)
        {
            mLvlChunkReducer.MergeReduceLvlChunks(fs, resourcePath, lvls, eType);
        }

        struct LvlSoundInfo
        {
            LvlSoundInfo(Oddlib::LvlArchive::File* vh, Oddlib::LvlArchive::File* vb)
                : mVh(vh), mVb(vb)
            {

            }

            LvlSoundInfo(LvlSoundInfo&& other)
            {
                *this = std::move(other);
            }

            LvlSoundInfo& operator = (LvlSoundInfo&& other)
            {
                mVh = other.mVh;
                mVb = other.mVb;
                mVab = std::move(other.mVab);
                return *this;
            }

            Oddlib::LvlArchive::File* mVh;
            Oddlib::LvlArchive::File* mVb;
            std::unique_ptr<Vab> mVab;
        };

        struct Sounds
        {
            std::vector<LvlSoundInfo> mSoundSets;
            std::vector<Oddlib::LvlArchive::File*> mSeqs;
            Oddlib::LvlArchive* mLvl;
            std::string mLvlName;
        };

        struct AllSounds
        {
            std::map<eDataSetType, std::map<std::string, Sounds>> mSounds;
            std::vector<std::unique_ptr<Oddlib::LvlArchive>> mLvls;
        };
        AllSounds mSounds;

        void CollectSounds(IFileSystem& parentFs, eDataSetType eType, const std::string& resourcePath, const std::vector<std::string>& lvls)
        {
            auto fs = IFileSystem::Factory(parentFs, resourcePath);
            if (!fs)
            {
                throw std::runtime_error("FS init failed");
            }

            for (const std::string& lvl : lvls)
            {
                Sounds& sounds = mSounds.mSounds[eType][lvl];

                std::unique_ptr<Oddlib::LvlArchive> archive = std::make_unique<Oddlib::LvlArchive>(fs->Open(lvl));
                std::vector<Oddlib::LvlArchive::File*> bsqFiles;

                sounds.mLvl = archive.get();
                sounds.mLvlName = lvl;

                for (u32 i = 0; i < archive->FileCount(); i++)
                {
                    Oddlib::LvlArchive::File* f = archive->FileByIndex(i);
                    if (string_util::ends_with(f->FileName(), ".VH", true))
                    {
                        // Get matching VB
                        Oddlib::LvlArchive::File* vbFile = archive->FileByName(f->FileName().substr(0, f->FileName().length() - 2) + "VB");
                        
                        // Store the VH/VB pair
                        sounds.mSoundSets.push_back(LvlSoundInfo(f, vbFile));
                    }
                    else if (string_util::ends_with(f->FileName(), ".BSQ", true))
                    {
                        bsqFiles.push_back(f);
                    }
                }

                if (bsqFiles.empty())
                {
                    // Should be at least 1 BSQ per LVL
                    abort();
                }

                sounds.mSeqs = bsqFiles;
                mSounds.mLvls.push_back(std::move(archive));
            }
        }

        const LvlSoundInfo* Find(eDataSetType dataSet, const char* lvl, const char* vab)
        {
            auto it = mSounds.mSounds.find(dataSet);
            if (it != std::end(mSounds.mSounds))
            {
                auto lvlIt = it->second.find(lvl);
                if (lvlIt != std::end(it->second))
                {
                    for (const LvlSoundInfo& vhVb : lvlIt->second.mSoundSets)
                    {
                        if (vhVb.mVh->FileName() == vab)
                        {
                            return &vhVb;
                        }
                    }
                }
            }
            return nullptr;
        }

        void Compare(const Vab& v1, const Vab& v2)
        {
            if (v1.mSamples.size() != v2.mSamples.size())
            {
                abort();
            }
        }

        void LoadVabs()
        {
            
            Oddlib::FileStream aePcSoundsDat("F:\\Data\\alive\\all_data\\Oddworld Abes Exoddus\\sounds.dat", Oddlib::IStream::ReadMode::ReadOnly);
            for (auto& soundMap : mSounds.mSounds)
            {
                std::map<std::string, Sounds>& lvlsSoundMap = soundMap.second;
                for (auto& s : lvlsSoundMap)
                {
                    auto& soundSets = s.second.mSoundSets;
                    for (auto& vhVb : soundSets)
                    {
                        auto vhStream = vhVb.mVh->ChunkByIndex(0)->Stream();

                        const bool kIsPsx = IsPsx(soundMap.first);

                        auto vab = std::make_unique<Vab>();
                        vab->ReadVh(*vhStream, kIsPsx);

                        const bool kUseSoundsDat = soundMap.first == eAePc;

                        auto vbStream = vhVb.mVb->ChunkByIndex(0)->Stream();
                        vab->ReadVb(*vbStream, kIsPsx, kUseSoundsDat, &aePcSoundsDat);

                        vhVb.mVab = std::move(vab);
                    }
                }
            }
            

            // Get RFSNDFX from R1.LVL for PSX and PC
            /*
            const LvlSoundInfo* pc = Find(eDataSetType::eAoPc, "RFSNDFX.VH");
            const LvlSoundInfo* psx = Find(eDataSetType::eAoPsx, "RFSNDFX.VH");
            if (pc && psx)
            {
              //  Compare(*pc->mVab, *psx->mVab);
            }
            */

            // TODO
            // De-duplicate the raw sample data - check PC-PSX samples are equal after decoding
            // Update VH to point to de-duplicated sample index
            // De-duplicate VH programs - now that they have common sample index


            // Update seq prog index to use de-duplicated VH progs
            // De-duplicate seqs the same way as VH progs

            // Write out deduplicated master sample DB and
            // real to de-duplicated tone indexes so VH and SEQ can be remapped to reduced data

            // Record which programs are not used, these will likely be the SFX?
        }

        static std::string LvlCode(const std::string& lvl)
        {
            auto pos = lvl.find_last_of(".");
            std::string r = lvl.substr(0, pos);
            r = r.substr(r.length() - 2);
            return r;
        }

        static std::string BsqName(const std::string& bsq)
        {
            auto pos = bsq.find_last_of(".");
            std::string r = bsq.substr(0, pos);
            return r;
        }

        void SoundsToJson()
        {

            jsonxx::Array resources;

            jsonxx::Array musics;
            jsonxx::Array vabs;
            jsonxx::Array soundEffects;

            // TODO: Check that AePsxCd1/2 can be merged
            // TODO: Check that AePsxCd1/2/AePc and AoPsx/AoPc can be merged
            // TODO: Trim off useless vab combos, XXENDER may only apply to the
            // 2nd BSQ - which is merged in PC? Need to reduce these down
            for (auto& soundMap : mSounds.mSounds)
            {
                std::map<std::string, Sounds>& s = soundMap.second;
                for (auto& lvlSounds : s)
                {
                    // SEQs
                    for (auto& bsq : lvlSounds.second.mSeqs)
                    {
                        for (u32 k = 0; k < bsq->ChunkCount(); k++)
                        {
                            for (LvlSoundInfo& vhVb : lvlSounds.second.mSoundSets)
                            {
                                jsonxx::Object music;

                                // Needs lvl name since R1.LVL contains both R1SEQ and R2SEQ for example
                                music << "resource_name"
                                    <<
                                    LvlCode(lvlSounds.second.mLvlName) + "_" +
                                    BsqName(bsq->FileName()) + "_" +
                                    std::to_string(k) + "_" +
                                    BsqName(vhVb.mVh->FileName()) + "_" +
                                    ToString(soundMap.first);

                                music << "sound_bank" <<
                                    (LvlCode(lvlSounds.second.mLvlName) + "_"
                                        + BsqName(vhVb.mVh->FileName()) + "_"
                                        + ToString(soundMap.first));

                                music << "data_set" << std::string(ToString(soundMap.first));
                                music << "file_name" << bsq->FileName();
                                music << "lvl" << lvlSounds.second.mLvlName;
                                music << "index" << k;

                                musics << music;
                            }
                        }
                    }

                    for (LvlSoundInfo& vhVb : lvlSounds.second.mSoundSets)
                    {
                        // VABs
                        jsonxx::Object vab;
                        vab << "resource_name"
                            << (LvlCode(lvlSounds.second.mLvlName) + "_"
                            + BsqName(vhVb.mVh->FileName()) + "_"
                            + ToString(soundMap.first));
                        vab << "data_set" << std::string(ToString(soundMap.first));
                        vab << "lvl" << lvlSounds.second.mLvlName;

                        vab << "vab_header" << BsqName(vhVb.mVh->FileName()) + ".VH";
                        vab << "vab_body" << BsqName(vhVb.mVh->FileName()) + ".VB";

                        vabs << vab;

                        // INSTRs/sounds effects

                        // TODO: This mapping isn't right as each key in a program could be made of many 
                        // tones! Might have to manually collect all SFX
                        for (int progIdx = 0; progIdx < vhVb.mVab->mHeader.iNumProgs; progIdx++)
                        {
                            const ProgAtr& prog = vhVb.mVab->mProgs[progIdx];
                            int min = 0;
                            int max = 0;
                            for (int toneIdx = 0; toneIdx < prog.iNumTones; toneIdx++)
                            {
                                const VagAtr& tone = *prog.iTones[toneIdx];
                                if (toneIdx == 0)
                                {
                                    min = tone.iMin;
                                    max = tone.iMax;
                                }
                                else
                                {
                                    if (tone.iMin > min)
                                    {
                                        min = tone.iMin;
                                    }
                                    if (tone.iMax > max)
                                    {
                                        max = tone.iMax;
                                    }
                                }
                            }

                            for (int i = min; i < max; i++)
                            {
                                jsonxx::Object soundEffect;
                                soundEffect << "resource_name" << (LvlCode(lvlSounds.second.mLvlName) + "_"
                                    + BsqName(vhVb.mVh->FileName()) + "_"
                                    + "P" + std::to_string(progIdx) + "_"
                                    + "N" + std::to_string(i) + "_"
                                    + ToString(soundMap.first));

                                soundEffect << "program" << progIdx;
                                //soundEffect << "tone" << toneIdx;
                                soundEffect << "note" << i;

                                soundEffect << "data_set" << std::string(ToString(soundMap.first));
                                //soundEffect << "vab_header" << BsqName(vhVb.mVh->FileName()) + ".VH";
                                //soundEffect << "vab_body" << BsqName(vhVb.mVh->FileName()) + ".VB";
                                soundEffect << "sound_bank" <<
                                    (LvlCode(lvlSounds.second.mLvlName) + "_"
                                        + BsqName(vhVb.mVh->FileName()) + "_"
                                        + ToString(soundMap.first));

                                soundEffects << soundEffect;
                            }
                        }
                    }
                }
            }

            jsonxx::Object musicsObj;
            musicsObj << "musics" << musics;
            resources << musicsObj;

            jsonxx::Object vabsObj;
            vabsObj << "sound_banks" << vabs;
            resources << vabsObj;

            jsonxx::Object soundEffectsObj;
            soundEffectsObj << "sound_effects" << soundEffects;
            resources << soundEffectsObj;

            std::ofstream jsonFile("..\\data\\soundsz.json");
            if (!jsonFile.is_open())
            {
                abort();
            }
            jsonFile << resources.json().c_str() << std::endl;
        }

        void MergeDuplicateAnimations()
        {
            // Load the animation set for each unique Anim chunk
            std::vector<std::unique_ptr<DeDuplicatedLvlChunk>>& chunks = mLvlChunkReducer.UniqueChunks();
            for (std::unique_ptr<DeDuplicatedLvlChunk>& chunk : chunks)
            {
                Oddlib::LvlArchive::FileChunk* lvlFileChunk = chunk->mChunk->mChunk;
                if (lvlFileChunk->Type() == Oddlib::MakeType("Anim"))
                {
                    auto stream = lvlFileChunk->Stream();
                    Oddlib::AnimSerializer as(*stream, IsPsx(chunk->mChunk->mDataSet));
                    chunk->mAnimSet = std::make_unique<Oddlib::AnimationSet>(as);
                }
            }

            // Create a list of deduplicated animations by creating an instance for every animation in
            // each unique chunks animation set
            for (size_t i = 0; i < chunks.size(); i++)
            {
                std::unique_ptr<DeDuplicatedLvlChunk>& chunk = chunks[i];
                Oddlib::AnimationSet& animSet = *chunk->mAnimSet;
                for (u32 j = 0; j < animSet.NumberOfAnimations(); j++)
                {
                    auto deDuplicatedAnimation = std::make_unique<DeDuplicatedAnimation>();
                    deDuplicatedAnimation->mAnimation = animSet.AnimationAt(j);
                    deDuplicatedAnimation->mAnimationIndex = j;
                    deDuplicatedAnimation->mContainingChunk = chunk.get();
                    mDeDuplicatedAnimations.push_back(std::move(deDuplicatedAnimation));
                }
            }

            // De-duplicating all anims and checking that each data set actually has all of the required
            // anims for the base game to work is too time consuming. Instead we allow any anim to be loaded
            // from any dataset from any of its per data set dups.
            // AePc is taken as the "gold" dataset, so this will always be required to play any game as
            // anims animations, bomb animations etc will come from this dataset.

            /*
            for (size_t i = 0; i < mDeDuplicatedAnimations.size(); i++)
            {
                std::unique_ptr<DeDuplicatedAnimation>& ddAnim = mDeDuplicatedAnimations[i];
                auto it = mDeDuplicatedAnimations.begin() + i + 1;
                while(it != mDeDuplicatedAnimations.end())
                {
                    std::unique_ptr<DeDuplicatedAnimation>& otherDdAnim = *it;
                    if (*ddAnim == *otherDdAnim)
                    {
                        // Consume the others chunk as a duplicate
                        ddAnim->mDuplicates.push_back(std::make_pair(otherDdAnim->mAnimationIndex, otherDdAnim->mContainingChunk));

                        // And take it out of the list of unique anims
                        assert(otherDdAnim->mDuplicates.empty());
                        it = mDeDuplicatedAnimations.erase(it);
                    }
                    else
                    {
                        // Move on to checking the next one
                        ++it;
                    }
                }
            }
            */
        }

        void DumpAnimationSpriteSheets()
        {
            for (const std::unique_ptr<DeDuplicatedAnimation>& deDupedAnim : mDeDuplicatedAnimations)
            {
                auto stream = deDupedAnim->mContainingChunk->mChunk->mChunk->Stream();
                Oddlib::AnimSerializer as(*stream, IsPsx(deDupedAnim->mContainingChunk->mChunk->mDataSet));

                const std::string name = deDupedAnim->mContainingChunk->mChunk->mFileName + "_" +
                    std::to_string(deDupedAnim->mContainingChunk->mChunk->mChunk->Id()) +
                    "_" +
                    ToString(deDupedAnim->mContainingChunk->mChunk->mDataSet) +
                    "_" +
                    std::to_string(deDupedAnim->mAnimationIndex);


                const int frameW = MaxW(*deDupedAnim->mAnimation);
                const int w = frameW * deDupedAnim->mAnimation->NumFrames();
                const int h = MaxH(*deDupedAnim->mAnimation);
                const auto& calcFrame = deDupedAnim->mAnimation->GetFrame(0);
                SDL_SurfacePtr sprites(SDL_CreateRGBSurface(0,
                    w, h,
                    calcFrame.mFrame->format->BitsPerPixel,
                    calcFrame.mFrame->format->Rmask,
                    calcFrame.mFrame->format->Gmask,
                    calcFrame.mFrame->format->Bmask,
                    calcFrame.mFrame->format->Amask));
                SDL_SetSurfaceBlendMode(sprites.get(), SDL_BLENDMODE_NONE);

                int xpos = 0;
                for (s32 i = 0; i < deDupedAnim->mAnimation->NumFrames(); i++)
                {
                    const Oddlib::Animation::Frame& frame = deDupedAnim->mAnimation->GetFrame(i);

                    SDL_Rect dstRect = { xpos, h - frame.mFrame->h, frame.mFrame->w, frame.mFrame->h };
                    xpos += frame.mFrame->w;

                    SDL_BlitSurface(frame.mFrame, NULL, sprites.get(), &dstRect);

                }

                SDLHelpers::SaveSurfaceAsPng((name + ".png").c_str(), sprites.get());

                /*
                Oddlib::DebugAnimationSpriteSheet dss(as, name,
                deDupedAnim->mContainingChunk->mChunk->mChunk->Id(),
                ToString(deDupedAnim->mContainingChunk->mChunk->mDataSet));
                */
            }
        }

        void AnimationsToJson()
        {
            jsonxx::Array resources;

            jsonxx::Array animations;


            for (const std::unique_ptr<DeDuplicatedAnimation>& deDupedAnim : mDeDuplicatedAnimations)
            {
                jsonxx::Object anim;

                // Generated globally unique name
                const std::string strName = 
                    deDupedAnim->mContainingChunk->mChunk->mFileName + "_" +
                    std::to_string(deDupedAnim->mContainingChunk->mChunk->mChunk->Id()) +
                    "_" +
                    ToString(deDupedAnim->mContainingChunk->mChunk->mDataSet) +
                    "_" +
                    std::to_string(deDupedAnim->mAnimationIndex);

                anim
                    << "name"
                    << strName;

                // Guessed blending mode
                anim << "blend_mode" << 1;

                // TODO: Semi trans flag
                // TODO: pallet res id?
                

                WriteAnimLocations(anim, deDupedAnim);

                // We don't attempt to Fix Ao*Pc offsets anymore as we use
                // AePc anims for most things. The only Ao anims used are the ones
                // unique to Ao which are usually correct or only have slightly "jiggly" offsets

                //WriteAnimFrameOffsets(anim, deDupedAnim);

                animations << anim;
            }

            jsonxx::Object animationsContainter;
            animationsContainter << "animations" << animations;

            resources << animationsContainter;

            WriteLvlContentMappings(resources);


            std::ofstream jsonFile("..\\data\\resources.json");
            if (!jsonFile.is_open())
            {
                abort();
            }
            jsonFile << resources.json().c_str() << std::endl;
        }

        private:
            bool HasCorrectPsxFrameOffsets(const eDataSetType dataType)
            {
                switch (dataType)
                {
                //case eAePc:
                //case eAePsxCd1:
                //case eAePsxCd2:
                case eAePcDemo: // Results in fractional offsets, these seems to "fix" Ao anim offsets but many are missing from the demo
                    return true;

                //case eAoPsx:
               // case eAoPsxDemo:
                    /*
                case eAePc: // TODO: Has PSX scales?
                case eAePcDemo:
                case eAePsxCd1:
                case eAePsxCd2:
                case eAePsxDemo:
                */
                   // return true;

                default:
                    return false;
                }
            }

            const Oddlib::Animation* GetAnimationWithCorrectOffsets(const std::unique_ptr<DeDuplicatedAnimation>& anim)
            {
                // AoPc and AoPsxDemo have scaled offsets with rounding errors
                // AePc and AePcDemo have unscaled offsets
                // AePsxCd1/2, AePcsDemo, AoPsx and AoPsxDemo have correct offsets
                // Thus we attempt to write out *Psx or AePc* offsets which can then be correctly
                // scaled by the engine for AoPc and yields consistent logic for offset handling.
                if (HasCorrectPsxFrameOffsets(anim->mContainingChunk->mChunk->mDataSet))
                {
                    return anim->mAnimation;
                }
                else
                {
                    for (const auto& chunkPair : anim->mDuplicates)
                    {
                        if (HasCorrectPsxFrameOffsets(chunkPair.second->mChunk->mDataSet))
                        {
                            return chunkPair.second->mAnimSet->AnimationAt(chunkPair.first);
                        }
                    }
                }
                return nullptr;
            }

            void WriteAnimFrameOffsets(jsonxx::Object& anim, const std::unique_ptr<DeDuplicatedAnimation>& deDupedAnim)
            {
                jsonxx::Array frameOffsetsArray;
                const Oddlib::Animation* animPtr = GetAnimationWithCorrectOffsets(deDupedAnim);
                // If for some reason there was only AoPc and no Psx match was found this can happen
                // but if the anim duplicate finder is working correctly it should never happen in theory
                if (animPtr)
                {
                    for (s32 j = 0; j < animPtr->NumFrames(); j++)
                    {
                        const Oddlib::Animation::Frame& frame = animPtr->GetFrame(j);
                        jsonxx::Object frameOffsetObj;
                        frameOffsetObj << "x" << frame.mOffX;
                        frameOffsetObj << "y" << frame.mOffY;
                        frameOffsetsArray << frameOffsetObj;

                        // TOOD: Probably need collision rectangle data too
                    }
                    anim << "frame_offsets" << frameOffsetsArray;
                }
            }

            void WriteAnimLocations(jsonxx::Object& anim, const std::unique_ptr<DeDuplicatedAnimation>& deDupedAnim)
            {
                jsonxx::Array locationsArray;

                std::map<eDataSetType, std::set<LocationFileInfo>> locationMap = deDupedAnim->Locations();
                for (const auto& dataSetFileInfoPair : locationMap)
                {
                    const eDataSetType dataSet = dataSetFileInfoPair.first;
                    const std::set<LocationFileInfo>& locations = dataSetFileInfoPair.second;

                    jsonxx::Object locationObj;
                    jsonxx::Array files;
                    locationObj << "dataset" << std::string(ToString(dataSet));
                    for (const LocationFileInfo& fileInfo : locations)
                    {
                        jsonxx::Object fileObj;
                        fileObj << "filename" << fileInfo.mFileName;
                        fileObj << "id" << fileInfo.mChunkId;
                        fileObj << "index" << fileInfo.mAnimIndex;
                        files << fileObj;
                    }
                    locationObj << "files" << files;
                    locationsArray << locationObj;
                }

                anim << "locations" << locationsArray;
            }

            void WriteLvlContentMappings(jsonxx::Array& resources)
            {

                // Map of which LVL's live in what data set
                for (const auto& dataSetPair : mLvlChunkReducer.LvlContent())
                {
                    jsonxx::Object dataSet;
                    const std::string strName = ToString(dataSetPair.first);
                    dataSet << "data_set_name" << strName;
                    dataSet << "is_psx" << IsPsx(dataSetPair.first);
                    dataSet << "scale_frame_offsets" << (dataSetPair.first == eAePc || dataSetPair.first == eAePcDemo);

                    jsonxx::Array lvlsArray;


                    for (const auto& lvlData : dataSetPair.second)
                    {
                        jsonxx::Object lvlObj;
                        lvlObj << "name" << lvlData.first;

                        jsonxx::Array lvlContent;
                        const std::set<std::string>& content = lvlData.second;
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
            }

            /*
        void AddNumAnimationsMapping(u32 resId, u32 numAnims)
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
        */

    private:
        LvlFileChunkReducer mLvlChunkReducer;
    };

    Db db;

    GameFileSystem gameFs;
    if (!gameFs.Init())
    {
        std::cout << "Game FS init failed" << std::endl;
        return 1;
    }

    DataPaths dataPaths(gameFs, "{GameDir}/data/DataSetIds.json", "{UserDir}/DataSets.json");
    ResourceMapper mapper(gameFs, "{GameDir}/data/resources.json");

    const auto jsonFiles = gameFs.EnumerateFiles("{GameDir}/data/GameDefinitions", "*.json");
    std::vector<GameDefinition> gameDefs;
    for (const auto& gameDef : jsonFiles)
    {
        gameDefs.emplace_back(gameFs, (std::string("{GameDir}/data/GameDefinitions") + "/" + gameDef).c_str(), false);
    }

    ResourceLocator resourceLocator(std::move(mapper), std::move(dataPaths));

    DataSetPathVector dataSet;

    for (const auto& gd : gameDefs)
    {
        DataSetPath pd(gd.DataSetName(), &gd);
        pd.mDataSetPath = resourceLocator.GetDataPaths().PathFor(pd.mDataSetName);
        dataSet.emplace_back(pd);
    }

    resourceLocator.GetDataPaths().SetActiveDataPaths(gameFs, dataSet);

    OggEncoder ogg;
    ogg.InitEncoder(resourceLocator);

    LoopFlagTest();

    for (const auto& data : datas)
    {
        const auto it = DataTypeLvlMap.find(data.first);
        if (it == std::end(DataTypeLvlMap))
        {
            // Defined struct is wrong
            abort();
        }

        {
            //db.DumpAoPcFg1({ "R1.LVL" });

            //db.DumpFg1(gameFs, data.second, *it->second, it->first);
            //db.DumpPaths(gameFs, data.first, data.second, *it->second);
        }
    }
    /*

    // db.MergeDuplicateAnimations();
    // db.AnimationsToJson();
    // db.DumpAnimationSpriteSheets();
    
    db.LoadVabs();
    // db.MergeDuplicateVhandVbs();
    db.SoundsToJson();
    */

    return 0;
}
