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
    MOCK_METHOD1(Exists, bool(const char*));
};

TEST(ResourceLocator, ResourceLoaderOpen)
{
    MockFileSystem fs;

    ResourceLoader loader(fs);

    loader.Add("C:\\dataset_location1", 1, "AePc");
    loader.Add("C:\\dataset_location2", 2, "AePc");

    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location1\\SLIGZ.BND")))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(fs, OpenProxy(StrEq("C:\\dataset_location2\\SLIGZ.BND")))
        .Times(1)
        .WillOnce(Return(new Oddlib::Stream(StringToVector("test"))));

    auto stream = loader.Open("SLIGZ.BND");
    ASSERT_NE(nullptr, stream);

    const auto str = stream->LoadAllToString();
    ASSERT_EQ(str, "test");
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

    GameDefinition gd(fs, "test_game_definition.json");
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

    ResourceMapper mapper;
    mapper.AddAnimMapping("SLIGZ.BND_417_1", { "SLIGZ.BND", 417, 1 });
    mapper.AddAnimMapping("SLIGZ.BND_417_2", { "SLIGZ.BND", 417, 1 });

    ResourceLocator locator(fs, aePc, std::move(mapper));

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

    EXPECT_CALL(fs, Exists(StrEq("F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus\\Exoddus.exe")))
        .WillOnce(Return(true));

    EXPECT_CALL(fs, Exists(StrEq("C:\\data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin\\AbeWin.exe")))
        .WillOnce(Return(false));

    EXPECT_CALL(fs, Exists(StrEq("C:\\data\\Oddworld - Abe's Exoddus (E) (Disc 1) [SLES-01480].bin\\Exoddus.exe")))
        .WillOnce(Return(false));

    EXPECT_CALL(fs, Exists(StrEq("F:\\Program Files\\SteamGames\\SteamApps\\common\\Oddworld Abes Exoddus\\sounds.dat")))
        .WillOnce(Return(false));

    EXPECT_CALL(fs, EnumerateFiles(StrEq("${game_files}\\GameDefinitions"), StrEq("*.json")))
        .WillOnce(Return(std::vector<std::string> { "AbesExoddusPc.json" }));

    EXPECT_CALL(fs, EnumerateFiles(StrEq("${user_home}\\Alive\\Mods"), StrEq("*.json")))
        .WillOnce(Return(std::vector<std::string> {  }));


    EXPECT_CALL(fs, Exists(StrEq("datasets.json")))
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
        gds.emplace_back(GameDefinition(fs, (std::string("${game_files}\\GameDefinitions") + "\\" + file).c_str()));
    }

    // load the enumerated "mod" game defs
    const auto modGs = fs.EnumerateFiles("${user_home}\\Alive\\Mods", "*.json");
    for (const auto& file : modGs)
    {
        gds.emplace_back(GameDefinition(fs, (std::string("${user_home}\\Alive\\Mods") + "\\" + file).c_str()));
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
    ResourceMapper mapper;
    mapper.AddAnimMapping("SLIGZ.BND_417_1", { "SLIGZ.BND", 417, 1 });

    ResourceLocator resourceLocator(fs, selected, std::move(mapper));
    
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
