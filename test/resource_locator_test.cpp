#include <gmock/gmock.h>
#include <set>
#include "oddlib/stream.hpp"
#include <jsonxx/jsonxx.h>
#include "logger.hpp"
#include "resourcemapper.hpp"

using namespace ::testing;

class MockFileSystem : public IFileSystem
{
public:
    virtual std::unique_ptr<Oddlib::IStream> Open(const char* fileName) override
    {
        // Can't mock unique_ptr return, so mock raw one which the unique_ptr one will call
        return std::unique_ptr<Oddlib::IStream>(OpenProxy(fileName));
    }

    MOCK_METHOD1(OpenProxy, Oddlib::IStream*(const char*));
    MOCK_METHOD2(EnumerateFiles, std::vector<std::string>(const char*, const char* ));
    MOCK_METHOD1(FileExists, bool(const char*));
};

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

    MockFileSystem fs;

    EXPECT_CALL(fs, OpenProxy(StrEq("resource_maps.json")))
        .WillRepeatedly(Return(new Oddlib::Stream(StringToVector(resourceMapsJson))));

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

    MockFileSystem fs;
    EXPECT_CALL(fs, OpenProxy(StrEq("test_game_definition.json")))
        .WillRepeatedly(Return(new Oddlib::Stream(StringToVector(gameDefJson))));

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

    MockFileSystem fs;

    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location1\\SLIGZ.BND")))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location2\\SLIGZ.BND")))
        .Times(1)
        .WillOnce(Return(new Oddlib::Stream(StringToVector("test"))));   // For SLIGZ.BND_417_1, 2nd call should be cached

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

    MockFileSystem fs;

    const std::string datasetIdsJson = R"(
    {
      "data_set_ids" :
      {
        "AoPc": { "contains_any":  [ "AbeWin.exe" ] },
        "AePc": { "contains_any":  [ "Exoddus.exe" ], "not_contains": [ "sounds.dat" ] }
      }
    }
    )";

    EXPECT_CALL(fs, OpenProxy(StrEq("datasetids.json")))
        .WillRepeatedly(Return(new Oddlib::Stream(StringToVector(datasetIdsJson))));

    const std::string dataSetsJson = R"(
    {
     "paths": [
        "F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus",
        "C:\\data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin"
      ]
    }
    )";

    EXPECT_CALL(fs, OpenProxy(StrEq("datasets.json")))
        .WillRepeatedly(Return(new Oddlib::Stream(StringToVector(dataSetsJson))));

    EXPECT_CALL(fs, FileExists(StrEq("F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus\\Exoddus.exe")))
        .WillOnce(Return(true));

    EXPECT_CALL(fs, FileExists(StrEq("C:\\data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin\\AbeWin.exe")))
        .WillOnce(Return(false));

    EXPECT_CALL(fs, FileExists(StrEq("C:\\data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin\\Exoddus.exe")))
        .WillOnce(Return(false));

    EXPECT_CALL(fs, FileExists(StrEq("F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus\\sounds.dat")))
        .WillOnce(Return(false));

    EXPECT_CALL(fs, EnumerateFiles(StrEq("${game_files}\\GameDefinitions"), StrEq("*.json")))
        .WillOnce(Return(std::vector<std::string> { "AbesExoddusPc.json" }));

    EXPECT_CALL(fs, EnumerateFiles(StrEq("${user_home}\\Alive\\Mods"), StrEq("*.json")))
        .WillOnce(Return(std::vector<std::string> {  }));


    EXPECT_CALL(fs, FileExists(StrEq("datasets.json")))
        .WillOnce(Return(true));

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

    EXPECT_CALL(fs, OpenProxy(StrEq("${game_files}\\GameDefinitions\\AbesExoddusPc.json")))
        .WillRepeatedly(Return(new Oddlib::Stream(StringToVector(aePcGameDefJson))));


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

        const std::set<std::string> expectedMissingDataSets { "SetB", "SetC" };
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

        const std::vector< std::pair<std::string, const GameDefinition*>> builtIns{ { "SetD", &d }, { "SetE", &e } };
        ASSERT_EQ(builtIns, sorted.gameDefs);

        const std::vector< std::pair<std::string, const GameDefinition*>> mods{ { "SetA", &a }, { "SetB", &b }, { "SetC", &c } };
        ASSERT_EQ(mods, sorted.modDefs);
    }
}
