#include <gmock/gmock.h>
#include "oddlib/lvlarchive.hpp"

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(LvlArchive, FileNotFound)
{
    ASSERT_THROW(Oddlib::LvlArchive("not_found.lvl"), Oddlib::Exception);
}

TEST(LvlArchive, DISABLED_Integration)
{
    // Load AE lvl
    Oddlib::LvlArchive lvl("MI.LVL");

    const auto file = lvl.FileByName("FLYSLIG.BND");
    ASSERT_NE(nullptr, file);

    const auto chunk = file->ChunkById(450);
    ASSERT_NE(nullptr, chunk);

    ASSERT_EQ(450, chunk->Id());

    const auto data = chunk->ReadData();
    ASSERT_EQ(false, data.empty());
}
