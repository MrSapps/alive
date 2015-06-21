#include <gmock/gmock.h>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/exceptions.hpp"
#include "SDL.h"
#include "sample.lvl.g.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


TEST(LvlArchive, FileNotFound)
{
    ASSERT_THROW(Oddlib::LvlArchive("not_found.lvl"), Oddlib::Exception);
}

std::string FileChunkToString(Oddlib::LvlArchive::FileChunk& chunk)
{
    auto fileData = chunk.ReadData();
    char* fileDataCharPtr = reinterpret_cast<char*>(fileData.data());
    return std::string(fileDataCharPtr, fileDataCharPtr + fileData.size()); 
}

TEST(LvlArchive, ReadFiles)
{
    std::vector<Uint8> data(std::begin(sample_lvl), std::end(sample_lvl));
    Oddlib::LvlArchive lvl(std::move(data));

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
