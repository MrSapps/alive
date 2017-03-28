#include <gmock/gmock.h>
#include <array>
#include "oddlib/lvlarchive.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/exceptions.hpp"
#include "cdromfilesystem.hpp"
#include "logger.hpp"
#include "SDL.h"
#include "sample.lvl.g.h"
#include "test.bin.g.h"
#include "xa.bin.g.h"
#include "subtitles.hpp"
#include "msvc_sdl_link.hpp"
#include <setjmp.h>

int main(int argc, char** argv)
{
    ::testing::GTEST_FLAG(catch_exceptions) = false;
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}


TEST(LvlArchive, FileNotFound)
{
    ASSERT_THROW(Oddlib::LvlArchive("not_found.lvl"), Oddlib::Exception);
}

TEST(LvlArchive, Corrupted)
{
    std::vector<unsigned char> invalidLvl =
    {
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
    };
    ASSERT_THROW(Oddlib::LvlArchive(std::move(invalidLvl)), Oddlib::InvalidLvl);
}

static std::string FileChunkToString(Oddlib::LvlArchive::FileChunk& chunk)
{
    auto fileData = chunk.ReadData();
    char* fileDataCharPtr = reinterpret_cast<char*>(fileData.data());
    return std::string(fileDataCharPtr, fileDataCharPtr + fileData.size());
}

TEST(LvlArchive, ReadFiles)
{
    Oddlib::LvlArchive lvl(get_sample());

    // Check number of read files is correct
    ASSERT_EQ(3u, lvl.FileCount());

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

    ASSERT_EQ(Oddlib::MakeType("Anim"), fileChunk->Type());

    file->SaveChunks();

}

static void IndentTest(int level)
{
    TRACE_ENTRYEXIT;
    if (level < 5)
    {
        LOG_INFO("At level " << level);
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

TEST(CdFs, Read_FileSystemLimits)
{
    RawCdImage img(get_test());
    img.LogTree();

    ASSERT_GT(img.FileExists("ROOT.TXT"), 0);
    ASSERT_GT(img.FileExists("LEN_TEST\\1.TXT"), 0);
    ASSERT_GT(img.FileExists("LEN_TEST\\12.TXT"), 0);
    ASSERT_GT(img.FileExists("LEN_TEST\\123.TXT"), 0);
    ASSERT_GT(img.FileExists("LEN_TEST\\1234.TXT"), 0);
    ASSERT_GT(img.FileExists("LEN_TEST\\12345.TXT"), 0);
    ASSERT_GT(img.FileExists("LEN_TEST\\123456.TXT"), 0);
    ASSERT_GT(img.FileExists("LEN_TEST\\1234567.TXT"), 0);
    ASSERT_GT(img.FileExists("LEN_TEST\\12345678.TXT"), 0);
    
    // Check case is ignored and mixed slashes
    ASSERT_GT(img.FileExists("level1\\lvl1.TXT"), 0);
    ASSERT_GT(img.FileExists("level1\\\\lvl1.TXT"), 0);
    ASSERT_GT(img.FileExists("level1/lvl1.TXT"), 0);
    ASSERT_GT(img.FileExists("/level1//lvl1.TXT"), 0);

    ASSERT_GT(img.FileExists("LEVEL1\\LVL1.TXT"), 0);
    ASSERT_GT(img.FileExists("LEVEL1\\LEVEL2\\LVL2.TXT"), 0);
    ASSERT_GT(img.FileExists("LEVEL1\\LEVEL2\\LEVEL3\\LVL3.TXT"), 0);
    ASSERT_GT(img.FileExists("LEVEL1\\LEVEL2\\LEVEL3\\LEVEL4\\LVL4.TXT"), 0);
    ASSERT_GT(img.FileExists("LEVEL1\\LEVEL2\\LEVEL3\\LEVEL4\\LEVEL5\\LVL5.TXT"), 0);
    ASSERT_GT(img.FileExists("LEVEL1\\LEVEL2\\LEVEL3\\LEVEL4\\LEVEL5\\LEVEL6\\LVL6.TXT"), 0);
    ASSERT_GT(img.FileExists("LEVEL1\\LEVEL2\\LEVEL3\\LEVEL4\\LEVEL5\\LEVEL6\\LEVEL7\\LVL7.TXT"), 0);
    ASSERT_EQ(img.FileExists("LEVEL1\\LEVEL2\\LEVEL3\\LEVEL4\\LEVEL5\\LEVEL6\\LEVEL7\\LVL77.TXT"), -1);

    ASSERT_GT(img.FileExists("TEST\\SECTORS1\\EXAMPLE.TXT"), 0);
    ASSERT_GT(img.FileExists("TEST\\SECTORS2\\BIG.TXT"), 0);
    ASSERT_GT(img.FileExists("TEST\\XA1\\SMALL.TXT"), 0);
    ASSERT_GT(img.FileExists("TEST\\XA1\\BIG.TXT"), 0);

    auto data = img.ReadFile("TEST\\SECTORS1\\EXAMPLE.TXT", false);

    const std::string expected = "dir entries go over 1 sector size";
    std::vector<u8> buffer(expected.size());
    data->Read(buffer);

    std::string strData(reinterpret_cast<char*>(buffer.data()), buffer.size());
    ASSERT_EQ(expected, strData);
}

TEST(CdFs, Read_XaSectors)
{
    RawCdImage img(get_xa());
    img.LogTree();

    ASSERT_EQ(img.FileExists(""), -1);
    ASSERT_GT(img.FileExists("NOT_XA\\SMALL.TXT"), 0);
    ASSERT_GT(img.FileExists("\\NOT_XA\\SMALL.TXT"), 0);
    ASSERT_GT(img.FileExists("NOT_XA\\BIG.TXT"), 0);
    ASSERT_GT(img.FileExists("XA1\\XBIG.TXT"), 0);
    ASSERT_GT(img.FileExists("XA1\\XSMALL"), 0);
    ASSERT_EQ(img.FileExists("XA2\\XSMALL"), -1);
    ASSERT_EQ(img.FileExists("XA2\\ASDFG"), -1);

}

TEST(SubTitleParser, Parse)
{
    // Check full range
    {
        auto data = std::deque<std::string> { "1", "12:34:56,789 --> 12:34:56,790", "Fool" };
        SubTitle s(data);
        ASSERT_EQ(1u, s.SequnceNumber());
        ASSERT_EQ(45296789u, s.StartTimeStampMsec());
        ASSERT_EQ(45296790u, s.EndTimeStampMsec());
        ASSERT_EQ("Fool", s.Text());
    }

    // Check milisecond range
    {
        auto data = std::deque<std::string> { "1", "00:00:00,000 --> 00:00:00,001", " lol " };
        SubTitle s(data);
        ASSERT_EQ(1u, s.SequnceNumber());
        ASSERT_EQ(0u, s.StartTimeStampMsec());
        ASSERT_EQ(1u, s.EndTimeStampMsec());
        ASSERT_EQ("lol", s.Text());
    }

    // Test finding single subtitle
    {
        SubTitleParser p("1\r\n12:34:56,789 --> 12:34:56,790\r\nFool\r\n");
        ASSERT_EQ(0u, p.Find(45296788u).size());
        ASSERT_EQ(1u, p.Find(45296789u).size());
        ASSERT_EQ("Fool", p.Find(45296789u)[0]->Text());
        ASSERT_EQ(1u, p.Find(45296790u).size());
        ASSERT_EQ("Fool", p.Find(45296790u)[0]->Text());
        ASSERT_EQ(0u, p.Find(45296791u).size());
        ASSERT_EQ(0u, p.Find(1).size());
    }

    // Test finding overlapping subtitle
    {
        SubTitleParser p("1\r\n12:34:56,789 --> 12:34:56,800\r\nFool1\r\n\r\n2\r\n12:34:56,789 --> 12:34:56,810\r\nFool2\r\n");
        ASSERT_EQ(0u, p.Find(45296788u).size());
        ASSERT_EQ(2u, p.Find(45296789u).size());
        ASSERT_EQ("Fool1", p.Find(45296789u)[0]->Text());
        ASSERT_EQ("Fool2", p.Find(45296789u)[1]->Text());
        ASSERT_EQ(2u, p.Find(45296800u).size());
        ASSERT_EQ("Fool1", p.Find(45296800u)[0]->Text());
        ASSERT_EQ("Fool2", p.Find(45296800u)[1]->Text());
        ASSERT_EQ(1u, p.Find(45296801u).size());
        ASSERT_EQ("Fool2", p.Find(45296801u)[0]->Text());
        ASSERT_EQ(0u, p.Find(1).size());
    }

    // Test multi line subs
    {
        auto data = std::deque<std::string> { "1", "00:00:00,000 --> 00:00:00,001", " lol1", "lol2", "lol3", "2" };
        SubTitle s(data);
        ASSERT_EQ(1u, s.SequnceNumber());
        ASSERT_EQ(0u, s.StartTimeStampMsec());
        ASSERT_EQ(1u, s.EndTimeStampMsec());
        ASSERT_EQ("lol1\nlol2\nlol3\n2", s.Text());
    }
}

class DtorTest
{
public:
    DtorTest(const DtorTest&) = delete;
    DtorTest& operator =(const DtorTest&) = delete;
    DtorTest(int& var)
        : mVar(var)
    {
        mVar++;
    }

    ~DtorTest()
    {
        mVar--;
    }
private:
    int& mVar;
};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4611)
#endif
jmp_buf env;
void DoJumper()
{
    longjmp(env, 1);
}

void WrapApi()
{
    if (setjmp(env))
    {
        return;
    }
    DoJumper();
}

void ExceptionTest(int& v)
{
    DtorTest test1(v);
    DtorTest test2(v);
    WrapApi();
    DtorTest test3(v);
}

TEST(LuaExceptionHandling, DestructorsAreCalled)
{
    int v = 0;
    ExceptionTest(v);
    ASSERT_EQ(0, v);
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

TEST(LvlArchive, DISABLED_Integration)
{
    // TODO: Check for IDX file in LVL to know if its AO or not?
    Oddlib::LvlArchive lvl("s1.LVL");

    for (u32 i = 0; i < lvl.FileCount(); i++)
    {
        Oddlib::LvlArchive::File* file = lvl.FileByIndex(i);
        for (u32 j = 0; j < file->ChunkCount(); j++)
        {
            Oddlib::LvlArchive::FileChunk* chunk = file->ChunkByIndex(j);
            if (chunk->Type() == Oddlib::MakeType("Anim"))
            {
                Oddlib::MemoryStream stream(chunk->ReadData());
                Oddlib::DebugDumpAnimationFrames(file->FileName(), chunk->Id(), stream, false, "unknown");
            }
        }
    }



    /*
    const auto file = lvl.FileByName("FLYSLIG.BND");
    ASSERT_NE(nullptr, file);


    const auto chunk = file->ChunkById(450);
    ASSERT_NE(nullptr, chunk);

    ASSERT_EQ(450u, chunk->Id());

    const auto data = chunk->ReadData();
    ASSERT_FALSE(data.empty());

    Oddlib::LvlArchive lvl2("R1.LVL");

    std::vector<std::unique_ptr<Oddlib::Animation>> animations = Oddlib::AnimationFactory::Create(lvl, "FLYSLIG.BND", 450);
    */


}
