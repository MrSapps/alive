#include <gmock/gmock.h>
#include "string_util.hpp"
#include "resourcemapper.hpp"
#include "gridmap.hpp"
#include <array>

TEST(Collision, NoLines)
{
    Physics::raycast_collision hitPoint;
    std::vector<Oddlib::Path::CollisionItem> lines;
    ASSERT_FALSE(Physics::raycast_map(lines, { 0, 0 }, { 10, 10 }, { 0u }, &hitPoint));
}

TEST(Collision, NoLinesMultiInput)
{
    Physics::raycast_collision hitPoint;
    std::vector<Oddlib::Path::CollisionItem> lines;
    ASSERT_FALSE(Physics::raycast_map(lines, { 0, 0 }, { 10, 10 }, { 0u, 7u }, &hitPoint));
}

// TODO FIX ME
TEST(Collision, DISABLED_LineWithinLine)
{
    Physics::raycast_collision hitPoint;
    std::vector<Oddlib::Path::CollisionItem> lines = 
    {
        { { 0, 0 }, { 10, 0 }, 0, {}, 0 }
    };
    ASSERT_TRUE(Physics::raycast_map(lines, { 5, 0 }, { 9, 0 }, { 0u }, &hitPoint));
    ASSERT_EQ(hitPoint.intersection.x, 0);
    ASSERT_EQ(hitPoint.intersection.y, 0);
}

// If we collide with 2 lines make sure the nearest collision point wins
TEST(Collision, NearestHitPointWins)
{
    Physics::raycast_collision hitPoint;
    std::vector<Oddlib::Path::CollisionItem> lines =
    {
        { { 1949, 1240 },{ 2250, 1240 }, 0, { 65535, 65535, 65535, 65535 }, 301 },
        { { 1751, 1140 },{ 1975, 1140 }, 0, { 65535, 65535, 65535, 65535 }, 224 }
    };
    ASSERT_TRUE(Physics::raycast_map(lines, { 1957, 1090 }, { 1957, 1590 }, { 0u }, &hitPoint));
    ASSERT_EQ(hitPoint.intersection.x, 1957);
    ASSERT_EQ(hitPoint.intersection.y, 1140);
}
