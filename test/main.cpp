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

TEST(LvlArchive, Integration)
{
    // Load AE lvl
    Oddlib::LvlArchive lvl("MI.LVL");

    auto file = lvl.FileByName("FLYSLIG.BND");
    ASSERT_NE(nullptr, file);

}