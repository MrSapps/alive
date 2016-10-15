#include <gmock/gmock.h>
#include "string_util.hpp"
#include "resourcemapper.hpp"
#include "gridmap.hpp"
#include <array>

TEST(Collision, NoLines)
{
    Physics::raycast_collision hitPoint;
    CollisionLines lines;
    ASSERT_FALSE(CollisionLine::RayCast<1>(lines, { 0, 0 }, { 10, 10 }, { CollisionLine::eFloor }, &hitPoint));
}

TEST(Collision, NoLinesMultiInput)
{
    Physics::raycast_collision hitPoint;
    CollisionLines lines;
    ASSERT_FALSE(CollisionLine::RayCast<2>(lines, { 0, 0 }, { 10, 10 }, { CollisionLine::eFloor, CollisionLine::eBulletWall }, &hitPoint));
}

// TODO FIX ME
TEST(Collision, DISABLED_LineWithinLine)
{
    Physics::raycast_collision hitPoint;
    CollisionLines lines;
    lines.emplace_back(std::make_unique<CollisionLine>(glm::vec2 { 0, 0 }, glm::vec2{ 10, 0 }, CollisionLine::eFloor));

    ASSERT_TRUE(CollisionLine::RayCast<1>(lines, { 5, 0 }, { 9, 0 }, { CollisionLine::eFloor }, &hitPoint));
    ASSERT_EQ(hitPoint.intersection.x, 0);
    ASSERT_EQ(hitPoint.intersection.y, 0);
}

// If we collide with 2 lines make sure the nearest collision point wins
TEST(Collision, NearestHitPointWins)
{
    Physics::raycast_collision hitPoint;
    CollisionLines lines;
    lines.emplace_back(std::make_unique<CollisionLine>(glm::vec2{ 1949, 1240 }, glm::vec2{ 2250, 1240 }, CollisionLine::eFloor));
    lines.emplace_back(std::make_unique<CollisionLine>(glm::vec2{ 1751, 1140 }, glm::vec2{ 1975, 1140 }, CollisionLine::eFloor));

    ASSERT_TRUE(CollisionLine::RayCast<1>(lines, { 1957, 1090 }, { 1957, 1590 }, { CollisionLine::eFloor }, &hitPoint));
    ASSERT_EQ(hitPoint.intersection.x, 1957);
    ASSERT_EQ(hitPoint.intersection.y, 1140);
}
