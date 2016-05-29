#include <gmock/gmock.h>
#include "zipfilesystem.hpp"
#include "inmemoryfs.hpp"
#include "SimpleNoComp.zip.g.h"
#include "MaxECDRComment.zip.g.h"

TEST(ZipFileSystem, SimpleZip)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.Init());
    fs.AddFile("test.zip", get_MaxECDRComment());

    ZipFileSystem z("test.zip", fs);
    ASSERT_TRUE(z.Init());
}

TEST(ZipFileSystem, MaxECDRComment)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.Init());
    fs.AddFile("test.zip", get_SimpleNoComp());

    ZipFileSystem z("test.zip", fs);
    ASSERT_TRUE(z.Init());
}
