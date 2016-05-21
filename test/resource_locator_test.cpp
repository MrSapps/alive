#include <gmock/gmock.h>
#include <set>
#include "oddlib/stream.hpp"
#include <jsonxx/jsonxx.h>
#include "logger.hpp"
#include "resourcemapper.hpp"

using namespace ::testing;

// Pseudo in memory file system
class FakeFileSystem : public IFileSystem
{
private:
    struct File
    {
        std::string mName;
        std::vector<Uint8> mData;
    };

    struct Directory
    {
        std::string mName;
        std::vector<Directory> mChildren;
        std::vector<File> mFiles;
    };

public:
    virtual ~FakeFileSystem() = default;

    void AddFile(std::string strPath, const std::string& content = "")
    {
        AddFile(strPath, StringToVector(content));
    }

    void AddFile(std::string strPath, const std::vector<Uint8>& content)
    {
        DirectoryAndFileName path(strPath);
        Directory* dir = FindPath(path.mDir, true);

        // Don't allow duplicated file names
        File* file = FindFile(*dir, path.mFile);
        if (file)
        {
            // Update existing
            file->mData = content;
            return;
        }
        dir->mFiles.emplace_back(File{ path.mFile, content });
    }

// IFileSystem
    virtual bool Init() override
    {
        return true;
    }

    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) override
    {
        DirectoryAndFileName path(fileName);
        Directory* dir = FindPath(path.mDir, true);
        if (!dir)
        {
            return nullptr;
        }

        File* file = FindFile(*dir, path.mFile);
        if (!file)
        {
            return nullptr;
        }


        return std::make_unique<Oddlib::Stream>(std::vector<Uint8>(file->mData));
    }

    virtual std::vector<std::string> EnumerateFiles(const char* directory, const char* filter) override
    {
        Directory* dir = FindPath(directory, true);
        std::vector<std::string> ret;
        if (!dir)
        {
            return ret;
        }

        for (const File& file : dir->mFiles)
        {
            if (WildCardMatcher(file.mName, filter, IgnoreCase))
            {
                ret.emplace_back(file.mName);
            }
        }

        return ret;
    }

    virtual bool FileExists(const char* fileName) override
    {
        DirectoryAndFileName path(fileName);
        Directory* dir = FindPath(path.mDir, true);
        if (!dir)
        {
            return false;
        }

        for (const File& file : dir->mFiles)
        {
            if (file.mName == path.mFile)
            {
                return true;
            }
        }
        return false;
    }

    // Make public so it can be tested
    static bool WildCardMatcher(const std::string& text, std::string wildcardPattern, EMatchType caseSensitive)
    {
        return IFileSystem::WildCardMatcher(text, wildcardPattern, caseSensitive);
    }

    static void NormalizePath(std::string& path)
    {
        IFileSystem::NormalizePath(path);
    }

private:

    struct DirectoryAndFileName
    {
        DirectoryAndFileName(std::string path)
        {
            NormalizePath(path);

            auto lastDirPos = path.find_last_of('/');
            if (lastDirPos != std::string::npos)
            {
                mDir = path.substr(0, lastDirPos);
                mFile = path.substr(lastDirPos+1);
            }
            else
            {
                mDir = path;
            }
        }

        std::string mDir;
        std::string mFile;
    };

    File* FindFile(Directory& dir, const std::string& fileName)
    {
        for (File& file : dir.mFiles)
        {
            if (file.mName == fileName)
            {
                return &file;
            }
        }
        return nullptr;
    }

    Directory* FindPath(Directory& currentDir, std::deque<std::string>& paths, bool insert)
    {
        std::string dir = paths.front();
        paths.pop_front();

        for (Directory& subDir : currentDir.mChildren)
        {
            if (subDir.mName == dir)
            {
                if (paths.empty())
                {
                    return &subDir;
                }
                else
                {
                    return FindPath(subDir, paths, insert);
                }
            }
        }

        if (insert)
        {
            currentDir.mChildren.push_back(Directory{ dir, {}, {} });
            if (paths.empty())
            {
                return &currentDir.mChildren.back();
            }
            else
            {
                return FindPath(currentDir.mChildren.back(), paths, insert);
            }
        }
        else
        {
            return nullptr;
        }
    }

    Directory* FindPath(std::string path, bool insert)
    {
        string_util::replace_all(path, "\\", "/");
        auto dirs = string_util::split(path, '/');
        return FindPath(mRoot, dirs, insert);
    }

    Directory mRoot;
};


TEST(IFileSystem, NormalizePath)
{
    {
        std::string win32Path = "C:\\Windows\\System32";
        FakeFileSystem::NormalizePath(win32Path);
        ASSERT_EQ("C:/Windows/System32", win32Path);
    }
    {
        std::string linuxPath = "/home/fool/somedir";
        FakeFileSystem::NormalizePath(linuxPath);
        ASSERT_EQ("/home/fool/somedir", linuxPath);
    }
    {
        std::string mixedPath = "C:\\\\Windows/System32//Foo\\\\Bar";
        FakeFileSystem::NormalizePath(mixedPath);
        ASSERT_EQ("C:/Windows/System32/Foo/Bar", mixedPath);
    }
}

TEST(IFileSystem, WildCardMatcher)
{
    ASSERT_TRUE(FakeFileSystem::WildCardMatcher("Hello.txt", "*.txt", IFileSystem::IgnoreCase));
    ASSERT_FALSE(FakeFileSystem::WildCardMatcher("Hello.txt", "*.TXT", IFileSystem::MatchCase));
    ASSERT_TRUE(FakeFileSystem::WildCardMatcher("Hello.txt", "*.TXT", IFileSystem::IgnoreCase));

    ASSERT_TRUE(FakeFileSystem::WildCardMatcher("Hello.txt", "Hello.???", IFileSystem::MatchCase));
    ASSERT_FALSE(FakeFileSystem::WildCardMatcher("Hello.txt", "HELLO.???", IFileSystem::MatchCase));
    ASSERT_TRUE(FakeFileSystem::WildCardMatcher("Hello.txt", "HELLO.???", IFileSystem::IgnoreCase));

    ASSERT_TRUE(FakeFileSystem::WildCardMatcher("BlahHelloBlah", "*Hello*", IFileSystem::MatchCase));
    ASSERT_TRUE(FakeFileSystem::WildCardMatcher("BlahHelloBlah", "*HELLO*", IFileSystem::IgnoreCase));
    ASSERT_TRUE(FakeFileSystem::WildCardMatcher("BlahHelloBlah", "*HELLO*", IFileSystem::IgnoreCase));
    ASSERT_FALSE(FakeFileSystem::WildCardMatcher("BlahHelzloBlah", "*HELLO*", IFileSystem::MatchCase));

}

TEST(FakeFileSystem, Open)
{
    FakeFileSystem fs;
    fs.AddFile("/Home/Test.txt", "File content");
    {
        auto stream = fs.Open("/Home/Test.txt");
        ASSERT_NE(nullptr, stream);
        ASSERT_EQ("File content", stream->LoadAllToString());
    }
    {
        auto stream = fs.Open("/Home/TEST.txt");
        ASSERT_EQ(nullptr, stream);
    }

    fs.AddFile("/Root.txt", "Blah");
    {
        auto stream = fs.Open("/Root.txt");
        ASSERT_NE(nullptr, stream);
        ASSERT_EQ("Blah", stream->LoadAllToString());
    }
    {
        auto stream = fs.Open("/ROOT.txt");
        ASSERT_EQ(nullptr, stream);
    }
}

TEST(FakeFileSystem, EnumerateFiles)
{
    FakeFileSystem fs;
    ASSERT_EQ(std::vector<std::string>{}, fs.EnumerateFiles("/Test", "*"));
   
    fs.AddFile("/Test/1.txt", "");
    fs.AddFile("/Test/1.txt", "");
    fs.AddFile("/Test/2.txt", "");
    fs.AddFile("/Test/Whatever", "");

    const std::vector<std::string> enumAll{ "1.txt", "2.txt", "Whatever" };
    ASSERT_EQ(enumAll, fs.EnumerateFiles("/Test", "*"));

    const std::vector<std::string> enumFiltered{ "1.txt", "2.txt" };
    ASSERT_EQ(enumFiltered, fs.EnumerateFiles("/Test", "*.txt"));
}

TEST(FakeFileSystem, FileExists)
{
    FakeFileSystem fs;
    ASSERT_EQ(false, fs.FileExists("Rubbish"));
    ASSERT_EQ(false, fs.FileExists("/"));
    ASSERT_EQ(false, fs.FileExists("/Home"));
    ASSERT_EQ(false, fs.FileExists("/Home/Test.txt"));
    ASSERT_EQ(false, fs.FileExists("/Root.txt"));

    fs.AddFile("/Home/Test.txt", "File content");
    fs.AddFile("/Root.txt", "Blah");

    ASSERT_EQ(false, fs.FileExists("Rubbish"));
    ASSERT_EQ(false, fs.FileExists("/"));
    ASSERT_EQ(false, fs.FileExists("/Home"));
    ASSERT_EQ(true, fs.FileExists("/Home/Test.txt"));
    ASSERT_EQ(true, fs.FileExists("/Root.txt"));
}

TEST(ResourceLocator, Cache)
{
    ResourceCache cache;

    const size_t resNameHash = StringHash("foo");
    ASSERT_EQ(nullptr, cache.Find<Animation>(resNameHash));
    {
        Resource<Animation> res1(resNameHash, cache, nullptr);

        std::shared_ptr<Animation> cached = cache.Find<Animation>(resNameHash);
        ASSERT_NE(nullptr, cached);
        ASSERT_EQ(cached.get(), res1.Ptr());
    }
    ASSERT_EQ(nullptr, cache.Find<Animation>(resNameHash));
}

TEST(ResourceLocator, ParseResourceMap)
{
    const std::string resourceMapsJson = R"([ {"anims":[{"blend_mode":1,"name":"SLIGZ.BND_417_1"},{"blend_mode":1,"name":"SLIGZ.BND_417_2"}],"file":"SLIGZ.BND","id":417}])";

    FakeFileSystem fs;
    fs.AddFile("resource_maps.json", resourceMapsJson);

    ResourceMapper mapper(fs, "resource_maps.json");
    
    const ResourceMapper::AnimMapping* r0 = mapper.Find("I don't exist");
    ASSERT_EQ(nullptr, r0);

    const ResourceMapper::AnimMapping* r1 = mapper.Find("SLIGZ.BND_417_1");
    ASSERT_NE(nullptr, r1);
    ASSERT_EQ("SLIGZ.BND", r1->mFile);
    ASSERT_EQ(417u, r1->mId);
    ASSERT_EQ(1u, r1->mBlendingMode);

    const ResourceMapper::AnimMapping* r2 = mapper.Find("SLIGZ.BND_417_2");
    ASSERT_NE(nullptr, r2);
    ASSERT_EQ("SLIGZ.BND", r2->mFile);
    ASSERT_EQ(417u, r2->mId);
    ASSERT_EQ(1u, r2->mBlendingMode);

}

TEST(ResourceLocator, ParseGameDefinition)
{

    const std::string gameDefJson = R"(
    {
      "Name" : "Oddworld Abe's Exoddus PC",
      "Description" : "The original PC version of Oddworld Abe's Exoddus",
      "Author" : "Oddworld Inhabitants",
      "InitialLevel" : "st_path1",
      "DatasetName" : "AePc",
      "Hidden" : true,
      "RequiredDatasets"  : []
    }
    )";

    FakeFileSystem fs;
    fs.AddFile("test_game_definition.json", gameDefJson);

    GameDefinition gd(fs, "test_game_definition.json", false);
    ASSERT_EQ(gd.Name(), "Oddworld Abe's Exoddus PC");
    ASSERT_EQ(gd.Description(), "The original PC version of Oddworld Abe's Exoddus");
    ASSERT_EQ(gd.Author(), "Oddworld Inhabitants");
    ASSERT_EQ(gd.InitialLevel(), "st_path1");
    ASSERT_EQ(gd.DataSetName(), "AePc");
    ASSERT_EQ(gd.Hidden(), true);
}

TEST(ResourceLocator, GameDefinitionDiscovery)
{
    // TODO - enumerating GD's
}

TEST(ResourceLocator, LocateAnimation)
{
    GameDefinition aePc;

    FakeFileSystem fs;

    fs.AddFile("C:\\dataset_location2\\SLIGZ.BND", "test"); // For SLIGZ.BND_417_1, 2nd call should be cached

    DataPaths paths(fs, "{GameDir}/data/DataSetIds.json", "{GameDir}/data/DataSets.json");
    ResourceMapper mapper;
    mapper.AddAnimMapping("SLIGZ.BND_417_1", { "SLIGZ.BND", 417, 1 });
    mapper.AddAnimMapping("SLIGZ.BND_417_2", { "SLIGZ.BND", 417, 1 });

    ResourceLocator locator(std::move(mapper), std::move(paths));

    locator.AddDataPath("C:\\dataset_location2", 2, "AoPc");
    locator.AddDataPath("C:\\dataset_location1", 1, "AePc");
  
    Resource<Animation> resMapped1 = locator.Locate<Animation>("SLIGZ.BND_417_1");
    resMapped1.Reload();

    Resource<Animation> resMapped2 = locator.Locate<Animation>("SLIGZ.BND_417_1");
    resMapped2.Reload();

    // Can explicitly set the dataset to obtain it from a known location
    Resource<Animation> resDirect = locator.Locate<Animation>("SLIGZ.BND_417_1", "AePc");
    resDirect.Reload();
}

TEST(ResourceLocator, LocateAnimationMod)
{
    // TODO: Like LocateAnimation but with mod override with and without original data
}

TEST(ResourceLocator, LocateFmv)
{
    // TODO
}

TEST(ResourceLocator, LocateSound)
{
    // TODO
}

TEST(ResourceLocator, LocateMusic)
{
    // TODO
}

TEST(ResourceLocator, LocateCamera)
{
    // TODO
}

TEST(ResourceLocator, LocatePath)
{
    // TODO
}

TEST(ResourceLocator, Construct)
{

    FakeFileSystem fs;

    const std::string datasetIdsJson = R"(
    {
      "data_set_ids" :
      {
        "AoPc": { "contains_any":  [ "AbeWin.exe" ] },
        "AePc": { "contains_any":  [ "Exoddus.exe" ], "not_contains": [ "sounds.dat" ] }
      }
    }
    )";

    fs.AddFile("datasetids.json", datasetIdsJson);

    const std::string dataSetsJson = R"(
    {
     "paths": [
        "F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus",
        "C:\\data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin"
      ]
    }
    )";

    fs.AddFile("datasets.json", dataSetsJson);
    fs.AddFile("F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus\\Exoddus.exe");
    fs.AddFile("${ user_home }\\Alive\\Mods\\Dummy.txt");

    const std::string aePcGameDefJson = R"(
    {
      "Name" : "Oddworld Abe's Exoddus PC",
      "Description" : "The original PC version of Oddworld Abe's Exoddus",
      "Author" : "Oddworld Inhabitants",
      "InitialLevel" : "st_path1",
      "DatasetName" : "AePc",
      "RequiredDatasets"  : [ "Foo1", "Foo2" ]
    }
    )";

    fs.AddFile("${game_files}\\GameDefinitions\\AbesExoddusPc.json", aePcGameDefJson);


    // Data paths are saved user paths to game data
    // load the list of data paths (if any) and discover what they are
    DataPaths dataPaths(fs, "datasetids.json", "datasets.json");

    auto aoPaths = dataPaths.PathsFor("AoPc");
    ASSERT_EQ(aoPaths.size(), 0u);


    auto aePaths = dataPaths.PathsFor("AePc");
    ASSERT_EQ(aePaths.size(), 1u);
    ASSERT_EQ(aePaths[0], "F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus");

   
    std::vector<GameDefinition> gds;
 

    // load the enumerated "built-in" game defs
    const auto builtInGds = fs.EnumerateFiles("${game_files}\\GameDefinitions", "*.json");
    for (const auto& file : builtInGds)
    {
        gds.emplace_back(GameDefinition(fs, (std::string("${game_files}\\GameDefinitions") + "\\" + file).c_str(), false));
    }

    // load the enumerated "mod" game defs
    const auto modGs = fs.EnumerateFiles("${user_home}\\Alive\\Mods", "*.json");
    for (const auto& file : modGs)
    {
        gds.emplace_back(GameDefinition(fs, (std::string("${user_home}\\Alive\\Mods") + "\\" + file).c_str(), true));
    }

    // Get the user selected game def
    GameDefinition& selected = gds[0];

    // ask for any missing data sets
    auto requiredSets = selected.RequiredDataSets();
    requiredSets.emplace_back("AePc"); // add in self to check its found
    const auto missing = dataPaths.MissingDataSets(requiredSets);
    const std::vector<std::string> expected{ "Foo1", "Foo2" };
    ASSERT_EQ(expected, missing);

    // create the resource mapper loading the resource maps from the json db
    DataPaths dataPaths2(fs, "{GameDir}/data/DataSetIds.json", "{GameDir}/data/DataSets.json");
    ResourceMapper mapper;
    mapper.AddAnimMapping("SLIGZ.BND_417_1", { "SLIGZ.BND", 417, 1 });

    ResourceLocator resourceLocator(std::move(mapper), std::move(dataPaths2));
    
    // TODO: Handle extra mod dependent data sets
    // Need to merge GD dataset lists so that none "default" data paths appear first
    // then the "default" paths should appear in order.
  //  auto dataSets = GameDefinition::GetMergedDatasets(selected, builtInGds, modGs);


    for (const auto& requiredSet : selected.RequiredDataSets())
    {
        const auto& paths = dataPaths.PathsFor(requiredSet);
        for (const auto& path : paths)
        {
            // TODO: Priority
            resourceLocator.AddDataPath(path.c_str(), 0, requiredSet);
        }
    }

    // Now we can obtain resources
    Resource<Animation> resMapped1 = resourceLocator.Locate<Animation>("SLIGZ.BND_417_1");
    resMapped1.Reload();

}

TEST(ResourceLocator, GameDefinitionDeps)
{
    {
        // Create a graph like:
        // a
        // |
        // b c
        //   |
        //   d
        const GameDefinition a("a", "SetA", { "SetB", "SetC" }, false);
        const GameDefinition b("b", "SetB", {}, false);
        const GameDefinition c("c", "SetC", { "SetD" }, false);
        const GameDefinition d("d", "SetD", {}, false);

        const std::vector<const GameDefinition*> gds { &a, &b, &c, &d };

        DataSetMap requiredDataSets;
        std::set<std::string> missingDataSets;
        GameDefinition::GetDependencies(requiredDataSets, missingDataSets, &a, gds);

        const std::set<std::string> expectedMissingDataSets {};
        ASSERT_EQ(expectedMissingDataSets, missingDataSets);

        const DataSetMap expectedRequiredDataSets = { { "SetA", &a }, { "SetB", &b }, { "SetC", &c }, { "SetD", &d } };
        ASSERT_EQ(expectedRequiredDataSets, requiredDataSets);
    }

    {
        // Create a graph with cycles
        // a <--|
        // |    |
        // b c <|-|
        //   |  | |
        //   d--| |
        //   |    |
        //   e----|
        const GameDefinition a("a", "SetA", { "SetB", "SetC" }, false);
        const GameDefinition b("b", "SetB", {}, false);
        const GameDefinition c("c", "SetC", { "SetD" }, false);
        const GameDefinition d("d", "SetD", { "SetE", "SetA" }, false);
        const GameDefinition e("e", "SetE", { "SetC" }, false);

        const std::vector<const GameDefinition*> gds { &a, &b, &c, &d, &e };

        DataSetMap requiredDataSets;
        std::set<std::string> missingDataSets;
        GameDefinition::GetDependencies(requiredDataSets, missingDataSets, &a, gds);

        const std::set<std::string> expectedMissingDataSets {};
        ASSERT_EQ(expectedMissingDataSets, missingDataSets);

        const DataSetMap expectedRequiredDataSets{ { "SetA", &a }, { "SetB", &b }, { "SetC", &c }, { "SetD", &d }, { "SetE", &e } };
        ASSERT_EQ(expectedRequiredDataSets, requiredDataSets);
    }

    {
        // Create a graph with missing node(s)
        const GameDefinition a("a", "SetA", { "SetB", "SetC" }, false);

        const std::vector<const GameDefinition*> gds { };

        DataSetMap requiredDataSets;
        std::set<std::string> missingDataSets;
        GameDefinition::GetDependencies(requiredDataSets, missingDataSets, &a, gds);

        const std::set<std::string> expectedMissingDataSets{ "SetB", "SetC" };
        ASSERT_EQ(expectedMissingDataSets, missingDataSets);

        const DataSetMap expectedRequiredDataSets{ { "SetA", &a } };
        ASSERT_EQ(expectedRequiredDataSets, requiredDataSets);
    }

    // Need to know which ones are mods or not so we can check if we have
    // data paths to it
    {
        const GameDefinition a("a", "SetA", { "SetB", "SetC" }, true);
        const GameDefinition b("b", "SetB", {}, true);
        const GameDefinition c("c", "SetC", { "SetD" }, true);
        const GameDefinition d("d", "SetD", { "SetE", "SetA" }, false);
        const GameDefinition e("e", "SetE", { "SetC" }, false);

        const std::vector<const GameDefinition*> gds{ &a, &b, &c, &d, &e };
        const DataSetMap requiredDataSets{ { "SetA", &a }, { "SetB", &b }, { "SetC", &c }, { "SetD", &d }, { "SetE", &e } };

        const BuiltInAndModGameDefs sorted = GameDefinition::SplitInToBuiltInAndMods(requiredDataSets);

        const std::vector<const PriorityDataSet*> builtIns{ &requiredDataSets[3], &requiredDataSets[4] };
        ASSERT_EQ(builtIns, sorted.gameDefs);

        const std::vector<const PriorityDataSet*> mods{ &requiredDataSets[0], &requiredDataSets[1], &requiredDataSets[2] };
        ASSERT_EQ(mods, sorted.modDefs);
    }
}
