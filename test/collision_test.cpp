#include <gmock/gmock.h>
#include "string_util.hpp"
#include "resourcemapper.hpp"
#include "gridmap.hpp"
#include <array>

TEST(Collision, NoLines)
{
    Physics::raycast_collision hitPoint;
    std::vector<CollisionLine> lines;
    ASSERT_FALSE(Physics::raycast_map<1>(lines, { 0, 0 }, { 10, 10 }, { CollisionLine::eFloor }, &hitPoint));
}

TEST(Collision, NoLinesMultiInput)
{
    Physics::raycast_collision hitPoint;
    std::vector<CollisionLine> lines;
    ASSERT_FALSE(Physics::raycast_map<2>(lines, { 0, 0 }, { 10, 10 }, { CollisionLine::eFloor, CollisionLine::eBulletWall }, &hitPoint));
}

// TODO FIX ME
TEST(Collision, DISABLED_LineWithinLine)
{
    Physics::raycast_collision hitPoint;
    std::vector<CollisionLine> lines =
    {
        { { 0, 0 }, { 10, 0 }, CollisionLine::eFloor }
    };
    ASSERT_TRUE(Physics::raycast_map<1>(lines, { 5, 0 }, { 9, 0 }, { CollisionLine::eFloor }, &hitPoint));
    ASSERT_EQ(hitPoint.intersection.x, 0);
    ASSERT_EQ(hitPoint.intersection.y, 0);
}

// If we collide with 2 lines make sure the nearest collision point wins
TEST(Collision, NearestHitPointWins)
{
    Physics::raycast_collision hitPoint;
    std::vector<CollisionLine> lines =
    {
        { { 1949, 1240 },{ 2250, 1240 }, CollisionLine::eFloor },
        { { 1751, 1140 },{ 1975, 1140 }, CollisionLine::eFloor }
    };
    ASSERT_TRUE(Physics::raycast_map<1>(lines, { 1957, 1090 }, { 1957, 1590 }, { CollisionLine::eFloor }, &hitPoint));
    ASSERT_EQ(hitPoint.intersection.x, 1957);
    ASSERT_EQ(hitPoint.intersection.y, 1140);
}
