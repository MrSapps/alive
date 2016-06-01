#include <gmock/gmock.h>
#include "string_util.hpp"

TEST(string_util, replace_all_char)
{
    std::string input = "121212121";
    char find = '1';
    char replace = '9';

    string_util::replace_all(input, find, replace);
    ASSERT_EQ("929292929", input);
}

TEST(string_util, replace_all_string)
{
    std::string input = "yes no yes no";
    std::string find = "yes";
    std::string replace = "rofl";

    string_util::replace_all(input, find, replace);
    ASSERT_EQ("rofl no rofl no", input);
}

TEST(string_util, starts_with)
{
    std::string s1 = "HelloFool";
    ASSERT_TRUE(string_util::starts_with(s1, "HelloFool"));
    ASSERT_TRUE(string_util::starts_with(s1, "Hello"));
    ASSERT_TRUE(string_util::starts_with(s1, "H"));
    ASSERT_TRUE(string_util::starts_with(s1, "h", true));
    ASSERT_TRUE(string_util::starts_with(s1, "HELLO", true));

    std::string s2 = "AB";
    ASSERT_TRUE(string_util::starts_with(s2, "a", true));
    ASSERT_FALSE(string_util::starts_with(s2, "a", false));

    std::string s3;
    ASSERT_FALSE(string_util::starts_with(s3, "blah"));
    ASSERT_FALSE(string_util::starts_with(s3, "blah", true));
}

TEST(string_util, endsWith)
{
    std::string t1 = "LOLrofl";
    std::string t2 = "roflLOL";
    std::string t3 = "LroflL";
    ASSERT_FALSE(string_util::ends_with(t1, "LOL"));
    ASSERT_TRUE(string_util::ends_with(t1, "rofl"));
    ASSERT_TRUE(string_util::ends_with(t2, "LOL"));
    ASSERT_FALSE(string_util::ends_with(t2, "rofl"));
    ASSERT_FALSE(string_util::ends_with(t3, "Lr"));
    ASSERT_TRUE(string_util::ends_with(t3, ""));
    ASSERT_TRUE(string_util::ends_with(t3, "lL"));
}

TEST(string_util, contains)
{
    std::string t1 = "LOLrofl";
    std::string t2 = "roflLOL";
    std::string t3 = "LroflL";
    ASSERT_FALSE(string_util::contains(t1, "zzz"));
    ASSERT_TRUE(string_util::contains(t1, "LOL"));
    ASSERT_TRUE(string_util::contains(t1, "rofl"));
    ASSERT_TRUE(string_util::contains(t2, "LOL"));
    ASSERT_TRUE(string_util::contains(t2, "rofl"));
    ASSERT_TRUE(string_util::contains(t3, "Lr"));
    ASSERT_TRUE(string_util::contains(t3, ""));
    ASSERT_TRUE(string_util::contains(t3, "lL"));
}

TEST(string_util, iequals)
{
    ASSERT_TRUE(string_util::iequals("fool", "fool"));
    ASSERT_TRUE(string_util::iequals("FOol", "foOL"));
    ASSERT_TRUE(string_util::iequals("Fool", "fool"));
    ASSERT_FALSE(string_util::iequals("Fool", "fooz"));
}

TEST(string_util, split)
{
    std::string splitMe = "Horse,battery,staple";
    auto parts = string_util::split(splitMe, ',');
    ASSERT_EQ(3u, parts.size());
    ASSERT_EQ("Horse", parts[0]);
    ASSERT_EQ("battery", parts[1]);
    ASSERT_EQ("staple", parts[2]);
}

TEST(string_util, trim)
{
    ASSERT_EQ("lol lol", string_util::trim(" lol lol "));
    ASSERT_EQ("", string_util::trim(""));
    ASSERT_EQ("", string_util::trim(" "));
    ASSERT_EQ("lol", string_util::trim("  lol"));
    ASSERT_EQ("lol", string_util::trim("lol  "));
}
