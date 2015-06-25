#include <gmock/gmock.h>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/masher.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/exceptions.hpp"
#include "logger.hpp"
#include "SDL.h"
#include "sample.lvl.g.h"
#include "all_colours_high_compression_30_fps.ddv.g.h"
#include "all_colours_low_compression_15fps_8bit_mono_high_compression_5_frames_interleave.ddv.g.h"
#include "all_colours_low_compression_30_fps.ddv.g.h"
#include "all_colours_max_compression_30_fps.ddv.g.h"
#include "all_colours_medium_compression_30_fps.ddv.g.h"
#include "all_colours_min_compression_30_fps.ddv.g.h"
#include "mono_16_high_compression_all_samples.ddv.g.h"
#include "mono_16_low_compression_all_samples.ddv.g.h"
#include "mono_8_high_compression_all_samples.ddv.g.h"
#include "mono_8_low_compression_all_samples.ddv.g.h"
#include "stereo_16_high_compression_all_samples.ddv.g.h"
#include "stereo_16_low_compression_all_samples.ddv.g.h"
#include "stereo_8_high_compression_all_samples.ddv.g.h"
#include "stereo_8_low_compression_all_samples.ddv.g.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(LvlArchive, FileNotFound)
{
    ASSERT_THROW(Oddlib::LvlArchive("not_found.lvl"), Oddlib::Exception);
}

static std::string FileChunkToString(Oddlib::LvlArchive::FileChunk& chunk)
{
    auto fileData = chunk.ReadData();
    char* fileDataCharPtr = reinterpret_cast<char*>(fileData.data());
    return std::string(fileDataCharPtr, fileDataCharPtr + fileData.size()); 
}

TEST(LvlArchive, ReadFiles)
{
    Oddlib::LvlArchive lvl(get_sample_lvl());

    // Check number of read files is correct
    ASSERT_EQ(3, lvl.FileCount());

    // Check looking for non existing file returns nullptr
    ASSERT_EQ(nullptr, lvl.FileByName("not_found"));

    // And check these files can be found
    ASSERT_NE(nullptr, lvl.FileByName("GRENGLOW.BAN"));
    ASSERT_NE(nullptr, lvl.FileByName("HELLO.VB"));
    ASSERT_NE(nullptr, lvl.FileByName("HELLO.VH"));

    // Now check the contents of the files
    auto file = lvl.FileByName("HELLO.VB");
    auto fileChunk = file->ChunkById(0);
    ASSERT_NE(nullptr, fileChunk);
    ASSERT_EQ("Example VB file", FileChunkToString(*fileChunk));

    file = lvl.FileByName("HELLO.VH");
    fileChunk = file->ChunkById(0);
    ASSERT_NE(nullptr, fileChunk);
    ASSERT_EQ("Example VH file :)", FileChunkToString(*fileChunk));

    file = lvl.FileByName("GRENGLOW.BAN");
    fileChunk = file->ChunkById(0);
    ASSERT_EQ(nullptr, fileChunk);

    fileChunk = file->ChunkById(0x177a);
    ASSERT_NE(nullptr, fileChunk);

    ASSERT_EQ(Oddlib::MakeType('A', 'n', 'i', 'm'), fileChunk->Type());
}

TEST(Masher, all_colours_low_compression_15fps_8bit_mono_high_compression_5_frames_interleave)
{
    /*
    std::vector<Uint8> data;
    Oddlib::Masher masher(std::move(data));

    masher.Update();
    */


}

// Audio and video test
TEST(Masher, all_colours_high_compression_30_fps)
{

}

// All video compression tests
TEST(Masher, all_colours_low_compression_30_fps)
{

}

TEST(Masher, all_colours_max_compression_30_fps)
{

}

TEST(Masher, all_colours_medium_compression_30_fps)
{

}

TEST(Masher, all_colours_min_compression_30_fps)
{

}

// All audio compression and formats tests
TEST(Masher, mono_8_high_compression_all_samples)
{

}

TEST(Masher, mono_8_low_compression_all_samples)
{

}

TEST(Masher, mono_16_high_compression_all_samples)
{

}

TEST(Masher, mono_16_low_compression_all_samples)
{

}

TEST(Masher, stereo_8_high_compression_all_samples)
{

}

TEST(Masher, stereo_8_low_compression_all_samples)
{

}

TEST(Masher, stereo_16_high_compression_all_samples)
{

}

TEST(Masher, stereo_16_low_compression_all_samples)
{

}


static void IndentTest(int level)
{
    TRACE_ENTRYEXIT;
    if (level < 5)
    {
        LOG_INFO("At level %d", level);
        IndentTest(level + 1);
    }
}

TEST(Logger, Indentation)
{
    // TODO: Test with more than one thread
    TRACE_ENTRYEXIT;
    IndentTest(0);

    LOG_INFO("Info test");
    LOG_TRACE("Trace test");
    LOG_WARNING("Warning test");
    LOG_ERROR("Error test");
}

/*
TEST(LvlArchive, DISABLED_Integration)
{
    // Load AE lvl
    Oddlib::LvlArchive lvl("MI.LVL");

    const auto file = lvl.FileByName("FLYSLIG.BND");
    ASSERT_NE(nullptr, file);


    const auto chunk = file->ChunkById(450);
    ASSERT_NE(nullptr, chunk);

    ASSERT_EQ(450u, chunk->Id());

    const auto data = chunk->ReadData();
    ASSERT_FALSE(data.empty());

    Oddlib::LvlArchive lvl2("R1.LVL");

    std::vector<std::unique_ptr<Oddlib::Animation>> animations = Oddlib::AnimationFactory::Create(lvl2, "ROPES.BAN", 1000);

}
*/
