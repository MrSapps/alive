#include <gmock/gmock.h>
#include <set>
#include "oddlib/stream.hpp"
#include <jsonxx/jsonxx.h>
#include "logger.hpp"
#include "resourcemapper.hpp"
#include "inmemoryfs.hpp"

using namespace ::testing;

TEST(IFileSystem, NormalizePath)
{
    {
        std::string win32Path = "C:\\Windows\\System32";
        InMemoryFileSystem::NormalizePath(win32Path);
        ASSERT_EQ("C:/Windows/System32", win32Path);
    }
    {
        std::string linuxPath = "/home/fool/somedir";
        InMemoryFileSystem::NormalizePath(linuxPath);
        ASSERT_EQ("/home/fool/somedir", linuxPath);
    }
    {
        std::string mixedPath = "C:\\\\Windows/System32//Foo\\\\Bar";
        InMemoryFileSystem::NormalizePath(mixedPath);
        ASSERT_EQ("C:/Windows/System32/Foo/Bar", mixedPath);
    }
}

TEST(IFileSystem, WildCardMatcher)
{
    ASSERT_TRUE(InMemoryFileSystem::WildCardMatcher("Hello.txt", "*.txt", IFileSystem::IgnoreCase));
    ASSERT_FALSE(InMemoryFileSystem::WildCardMatcher("Hello.txt", "*.TXT", IFileSystem::MatchCase));
    ASSERT_TRUE(InMemoryFileSystem::WildCardMatcher("Hello.txt", "*.TXT", IFileSystem::IgnoreCase));

    ASSERT_TRUE(InMemoryFileSystem::WildCardMatcher("Hello.txt", "Hello.???", IFileSystem::MatchCase));
    ASSERT_FALSE(InMemoryFileSystem::WildCardMatcher("Hello.txt", "HELLO.???", IFileSystem::MatchCase));
    ASSERT_TRUE(InMemoryFileSystem::WildCardMatcher("Hello.txt", "HELLO.???", IFileSystem::IgnoreCase));

    ASSERT_TRUE(InMemoryFileSystem::WildCardMatcher("BlahHelloBlah", "*Hello*", IFileSystem::MatchCase));
    ASSERT_TRUE(InMemoryFileSystem::WildCardMatcher("BlahHelloBlah", "*HELLO*", IFileSystem::IgnoreCase));
    ASSERT_TRUE(InMemoryFileSystem::WildCardMatcher("BlahHelloBlah", "*HELLO*", IFileSystem::IgnoreCase));
    ASSERT_FALSE(InMemoryFileSystem::WildCardMatcher("BlahHelzloBlah", "*HELLO*", IFileSystem::MatchCase));

}

TEST(InMemoryFileSystem, Open)
{
    InMemoryFileSystem fs;
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

TEST(InMemoryFileSystem, EnumerateFiles)
{
    InMemoryFileSystem fs;
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

TEST(InMemoryFileSystem, FileExists)
{
    InMemoryFileSystem fs;
    ASSERT_FALSE(fs.FileExists("Rubbish"));
    ASSERT_FALSE(fs.FileExists("/"));
    ASSERT_FALSE(fs.FileExists("/Home"));
    ASSERT_FALSE(fs.FileExists("/Home/Test.txt"));
    ASSERT_FALSE(fs.FileExists("/Root.txt"));

    fs.AddFile("/Home/Test.txt", "File content");
    fs.AddFile("/Root.txt", "Blah");

    ASSERT_FALSE(fs.FileExists("Rubbish"));
    ASSERT_FALSE(fs.FileExists("/"));
    ASSERT_FALSE(fs.FileExists("/Home"));
    ASSERT_TRUE(fs.FileExists("/Home/Test.txt"));
    ASSERT_TRUE(fs.FileExists("/Root.txt"));
}

/*
TEST(ResourceLocator, DISABLED_ResourceGroup)
{
    InMemoryFileSystem fs;

    fs.AddFile("C:\\dataset_location2\\SLIGZ.BND", "test"); // For SLIGZ.BND_417_1, 2nd call should be cached

    DataPaths paths(fs, "{GameDir}/data/DataSetIds.json", "{GameDir}/data/DataSets.json");
    ResourceMapper mapper;
    mapper.AddAnimMapping("SLIGZ.BND_417_1",
    {
        1, 
        std::vector < ResourceMapper::AnimFileLocations > 
        { 
            ResourceMapper::AnimFileLocations
            { 
                "AoPc", 
                std::vector<ResourceMapper::AnimFile> 
                {
                    ResourceMapper::AnimFile { "SLIGZ.BND", 417, 1 }
                }
            }
        }
    });
    mapper.AddAnimMapping("BLAH.BND_417_2", 
    { 
        1, 
        std::vector < ResourceMapper::AnimFileLocations >
        { 
            ResourceMapper::AnimFileLocations
            {
                "AoPc", 
                std::vector<ResourceMapper::AnimFile>
                {
                    ResourceMapper::AnimFile { "SLIGZ.BND", 417, 1 }
                }
            }
        } 
    });

    ResourceLocator locator(std::move(mapper), std::move(paths));

    ResourceCache<Animation> group(locator);

    ASSERT_EQ(nullptr, group.Get("foo"));
    ASSERT_EQ(nullptr, group.Get("foo", "bar"));

    ASSERT_NE(nullptr, group.Get("SLIGZ.BND_417_1"));
    ASSERT_NE(nullptr, group.Get("SLIGZ.BND_417_1", "AoPc"));
    ASSERT_EQ(nullptr, group.Get("SLIGZ.BND_417_1", "AePc"));

    ASSERT_NE(nullptr, group.Get("BLAH.BND_417_2"));
    ASSERT_NE(nullptr, group.Get("BLAH.BND_417_2", "AePc"));
    ASSERT_EQ(nullptr, group.Get("BLAH.BND_417_2", "AoPc"));
}
*/

TEST(ResourceLocator, ParseResourceMap)
{
    const std::string resourceMapsJson = 
        R"(
[{
    "paths": [{
        "collision_offset": 400,
        "id": 88,
        "locations": [{
            "dataset": "AePc",
            "file_name": "BAPATH.BND"
        }, {
            "dataset": "AePsxCd2",
            "file_name": "BLOP.BND"
        }],
        "number_of_screens_x": 6,
        "number_of_screens_y": 8,
        "object_indextable_offset": 7628,
        "object_offset": 2460,
        "resource_name": "BAPATH_1"
    }],
    "animations": [{
        "blend_mode": 1,
        "locations": [{
            "dataset": "AoPc",
            "files": [{
                "filename": "ABEBSIC.BAN",
                "id": 10,
                "index": 1
            }, {
                "filename": "ANOTHER.BAN",
                "id": 50,
                "index": 99
            }]
        }, {
            "dataset": "AoPcDemo",
            "files": [{
                "filename": "ABEBSIC.BAN",
                "id": 10,
                "index": 1
            }]
        }],
        "name": "ABEBSIC.BAN_10_AePc_1"
    }],
    "fmvs": [{
        "locations": [{
            "dataset": "AePc",
            "file": "TRAIN2.DDV"
        }, {
            "dataset": "AoPsx",
            "file": "BLAH.MOV",
            "start_sector": 12345,
            "end_sector": 56789
        }],
        "name": "TRAIN2_DDV_AePc"
    }, {
        "locations": [{
            "dataset": "AoPsx",
            "file": "F2.MOV",
            "start_sector": 12359,
            "end_sector": 12877
        }],
        "name": "F2_MOV_22_AoPsx"
    }]
}]
)";

    InMemoryFileSystem fs;
    fs.AddFile("resource_maps.json", resourceMapsJson);

    ResourceMapper mapper(fs, "resource_maps.json");

    {
        const ResourceMapper::AnimMapping* r0 = mapper.FindAnimation("I don't exist");
        ASSERT_EQ(nullptr, r0);

        const ResourceMapper::AnimMapping* r1 = mapper.FindAnimation("ABEBSIC.BAN_10_AePc_1");
        ASSERT_NE(nullptr, r1);

        // Check location mappings
        ASSERT_EQ(2u, r1->mLocations.size());
        ASSERT_EQ(2u, r1->mLocations[0].mFiles.size());
        ASSERT_EQ(1u, r1->mLocations[1].mFiles.size());

        ASSERT_EQ("AoPc", r1->mLocations[0].mDataSetName);
        ASSERT_EQ("ABEBSIC.BAN", r1->mLocations[0].mFiles[0].mFile);
        ASSERT_EQ(1u, r1->mLocations[0].mFiles[0].mAnimationIndex);
        ASSERT_EQ(10u, r1->mLocations[0].mFiles[0].mId);

        ASSERT_EQ("ANOTHER.BAN", r1->mLocations[0].mFiles[1].mFile);
        ASSERT_EQ(99u, r1->mLocations[0].mFiles[1].mAnimationIndex);
        ASSERT_EQ(50u, r1->mLocations[0].mFiles[1].mId);

        ASSERT_EQ("AoPcDemo", r1->mLocations[1].mDataSetName);
        ASSERT_EQ("ABEBSIC.BAN", r1->mLocations[1].mFiles[0].mFile);
        ASSERT_EQ(1u, r1->mLocations[1].mFiles[0].mAnimationIndex);
        ASSERT_EQ(10u, r1->mLocations[1].mFiles[0].mId);

        ASSERT_EQ(1u, r1->mBlendingMode);
    }

    {
        const ResourceMapper::FmvMapping* r0 = mapper.FindFmv("I don't exist");
        ASSERT_EQ(nullptr, r0);

        const ResourceMapper::FmvMapping* r1 = mapper.FindFmv("TRAIN2_DDV_AePc");
        ASSERT_NE(nullptr, r1);

        ASSERT_EQ(2u, r1->mLocations.size());
        ASSERT_EQ("AePc", r1->mLocations[0].mDataSetName);
        ASSERT_EQ("TRAIN2.DDV", r1->mLocations[0].mFileName);
        ASSERT_EQ(0u, r1->mLocations[0].mStartSector);
        ASSERT_EQ(0u, r1->mLocations[0].mEndSector);

        ASSERT_EQ("AoPsx", r1->mLocations[1].mDataSetName);
        ASSERT_EQ("BLAH.MOV", r1->mLocations[1].mFileName);
        ASSERT_EQ(12345u, r1->mLocations[1].mStartSector);
        ASSERT_EQ(56789u, r1->mLocations[1].mEndSector);

        const ResourceMapper::FmvMapping* r3 = mapper.FindFmv("F2_MOV_22_AoPsx");
        ASSERT_NE(nullptr, r3);

        ASSERT_EQ(1u, r3->mLocations.size());
        ASSERT_EQ("AoPsx", r3->mLocations[0].mDataSetName);
        ASSERT_EQ("F2.MOV", r3->mLocations[0].mFileName);
        ASSERT_EQ(12359u, r3->mLocations[0].mStartSector);
        ASSERT_EQ(12877u, r3->mLocations[0].mEndSector);
    }

    {
        const ResourceMapper::PathMapping* r0 = mapper.FindPath("I don't exist");
        ASSERT_EQ(nullptr, r0);

        const ResourceMapper::PathMapping* r1 = mapper.FindPath("BAPATH_1");
        ASSERT_NE(nullptr, r1);
        ASSERT_EQ(88u, r1->mId);
        ASSERT_EQ(400u, r1->mCollisionOffset);
        ASSERT_EQ(7628u, r1->mIndexTableOffset);
        ASSERT_EQ(2460u, r1->mObjectOffset);
        ASSERT_EQ(6u, r1->mNumberOfScreensX);
        ASSERT_EQ(8u, r1->mNumberOfScreensY);

        ASSERT_EQ(2u, r1->mLocations.size());
        ASSERT_EQ("AePc", r1->mLocations[0].mDataSetName);
        ASSERT_EQ("BAPATH.BND", r1->mLocations[0].mDataSetFileName);
        
        ASSERT_EQ("AePsxCd2", r1->mLocations[1].mDataSetName);
        ASSERT_EQ("BLOP.BND", r1->mLocations[1].mDataSetFileName);
    }
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

    InMemoryFileSystem fs;
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

    InMemoryFileSystem fs;

    fs.AddFile("C:\\dataset_location2\\SLIGZ.BND", "test"); // For SLIGZ.BND_417_1, 2nd call should be cached

    DataPaths paths(fs, "{GameDir}/data/DataSetIds.json", "{GameDir}/data/DataSets.json");
    ResourceMapper mapper;
    mapper.AddAnimMapping("BLAH.BND_417_1",
    {
        1,
        std::vector < ResourceMapper::AnimFileLocations >
        {
            ResourceMapper::AnimFileLocations
            {
                "AoPc",
                std::vector<ResourceMapper::AnimFile>
                {
                    ResourceMapper::AnimFile{ "SLIGZ.BND", 417, 1 }
                }
            }
        }
    });

    mapper.AddAnimMapping("BLAH.BND_417_2",
    {
        1,
        std::vector < ResourceMapper::AnimFileLocations >
        {
            ResourceMapper::AnimFileLocations
            {
                "AoPc",
                std::vector<ResourceMapper::AnimFile>
                {
                    ResourceMapper::AnimFile{ "SLIGZ.BND", 417, 1 }
                }
            }
        }
    });

    ResourceLocator locator(std::move(mapper), std::move(paths));

    std::unique_ptr<Animation> resMapped1 = locator.LocateAnimation("SLIGZ.BND_417_1");

    std::unique_ptr<Animation> resMapped2 = locator.LocateAnimation("SLIGZ.BND_417_1");

    // Can explicitly set the dataset to obtain it from a known location
    auto resDirect = locator.LocateAnimation("SLIGZ.BND_417_1", "AePc");
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

    InMemoryFileSystem fs;

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

    auto aoPath = dataPaths.PathFor("AoPc");
    ASSERT_TRUE(aoPath.empty());


    auto aePath = dataPaths.PathFor("AePc");
    ASSERT_EQ(aePath, "F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus");

   
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
    const auto missing = dataPaths.MissingDataSetPaths(requiredSets);
    const std::vector<std::string> expected{ "Foo1", "Foo2" };
    ASSERT_EQ(expected, missing);

    // create the resource mapper loading the resource maps from the json db
    DataPaths dataPaths2(fs, "{GameDir}/data/DataSetIds.json", "{GameDir}/data/DataSets.json");
    ResourceMapper mapper;
    mapper.AddAnimMapping("BLAH.BND_417_1",
    {
        1,
        std::vector < ResourceMapper::AnimFileLocations >
        {
            ResourceMapper::AnimFileLocations
            {
                "AoPc",
                std::vector<ResourceMapper::AnimFile>
                {
                    ResourceMapper::AnimFile{ "SLIGZ.BND", 417, 1 }
                }
            }
        }
    });

    ResourceLocator resourceLocator(std::move(mapper), std::move(dataPaths2));
    
    // TODO: Handle extra mod dependent data sets
    // Need to merge GD dataset lists so that none "default" data paths appear first
    // then the "default" paths should appear in order.
  //  auto dataSets = GameDefinition::GetMergedDatasets(selected, builtInGds, modGs);


    // Now we can obtain resources
    std::unique_ptr<Animation> resMapped1 = resourceLocator.LocateAnimation("SLIGZ.BND_417_1");
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
