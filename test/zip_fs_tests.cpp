#include <gmock/gmock.h>
#include "zipfilesystem.hpp"
#include "inmemoryfs.hpp"
#include "SimpleNoComp.zip.g.h"
#include "MaxECDRComment.zip.g.h"

TEST(ZipFileSystem, SimpleZip)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.Init());
    fs.AddFile("test.zip", get_SimpleNoComp());

    ZipFileSystem z("test.zip", fs);
    ASSERT_TRUE(z.Init());

    ASSERT_EQ("test.zip", z.FsPath());
    ASSERT_EQ(std::vector<std::string>{ }, z.EnumerateFiles("NotExists", "*.*"));
    ASSERT_EQ(std::vector<std::string>{ "Example.txt" }, z.EnumerateFiles("", "*.*"));
    ASSERT_EQ(std::vector<std::string>{ "Sub.txt" }, z.EnumerateFiles("TestDir", "*.*"));

    ASSERT_FALSE(z.FileExists("NotHere.Txt"));
    ASSERT_TRUE(z.FileExists("Example.txt"));
    ASSERT_TRUE(z.FileExists("TestDir/Sub.txt"));

    auto s1 = z.Open("Example.txt");
    ASSERT_NE(nullptr, s1);
    ASSERT_EQ("Hello world!", s1->LoadAllToString());

    auto s2 = z.Open("TestDir/Sub.txt");
    ASSERT_NE(nullptr, s2);
    ASSERT_EQ("Blah", s2->LoadAllToString());
    
    auto s3 = z.Open("Blop.Txt");
    ASSERT_EQ(nullptr, s3);

}

TEST(ZipFileSystem, MaxECDRComment)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.Init());
    fs.AddFile("test.zip", get_MaxECDRComment());

    ZipFileSystem z("test.zip", fs);
    ASSERT_TRUE(z.Init());

    auto s1 = z.Open("Example.txt");
    ASSERT_NE(nullptr, s1);
    ASSERT_EQ("Hello world!", s1->LoadAllToString());
}
