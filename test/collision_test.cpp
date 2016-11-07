#include <gmock/gmock.h>
#include "string_util.hpp"
#include "resourcemapper.hpp"
#include "gridmap.hpp"
#include "renderer.hpp"
#include <array>

class TestCoordinateSpace : public CoordinateSpace
{
public:
    using CoordinateSpace::CoordinateSpace;
    using CoordinateSpace::UpdateCamera;
};

/*
TEST(CoordinateSpace, WorldToScreen)
{
    TestCoordinateSpace coords;
    coords.mCameraPosition = { 10.0f, 10.0f };
    coords.mScreenSize = { 320.0f, 240.0f };
    coords.UpdateCamera();

    // TODO FIX ME
    const glm::vec2 actual = coords.WorldToScreen({ 30.0f, 30.0f });
    //ASSERT_EQ(glm::round(43.3102f), glm::round(actual.x));
    //ASSERT_EQ(glm::round(43.3642f), glm::round(actual.y));
}
*/

TEST(CoordinateSpace, ScreenToWorld)
{
    TestCoordinateSpace coords;
    coords.mCameraPosition = { 10.0f, 10.0f };
    coords.mScreenSize = { 320.0f, 240.0f };
    coords.UpdateCamera();

    const glm::vec2 actual = coords.ScreenToWorld({ 30.0f, 30.0f });
    ASSERT_EQ(glm::round(43.3102f), glm::round(actual.x));
    ASSERT_EQ(glm::round(43.3642f), glm::round(actual.y));
}

TEST(CollisionLines, IsPointInCircle)
{
    ASSERT_TRUE(Physics::IsPointInCircle(glm::vec2(0.0f, 0.0f), 5.0f, glm::vec2(0.0f, 0.0f)));
    ASSERT_FALSE(Physics::IsPointInCircle(glm::vec2(0.0f, 0.0f), 5.0f, glm::vec2(0.0f, 7.0f)));
    ASSERT_TRUE(Physics::IsPointInCircle(glm::vec2(0.0f, 0.0f), 5.0f, glm::vec2(-3.0f, 3.0f)));
}

TEST(CollisionLines, IsPointInTriangle)
{
    ASSERT_TRUE(Physics::IsPointInTriangle(glm::vec2(0.0f, 0.0f), glm::vec2(100.0f, 0.0f), glm::vec2(0.0f, 50.0f), glm::vec2(30.0f, 10.0f)));
    ASSERT_FALSE(Physics::IsPointInTriangle(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 100.0f), glm::vec2(50.0f, 100.0f), glm::vec2(1000.0f, 1000.0f)));
}

TEST(CollisionLines, PointInLineSegmentRect)
{
    const f32 kWidth = 2.0f;

    ASSERT_TRUE(Physics::IsPointInThickLine(glm::vec2(0.0f, 0.0f), glm::vec2(20.0f, 0.0f), glm::vec2(10.0f, 0.0f), kWidth));
    ASSERT_FALSE(Physics::IsPointInThickLine(glm::vec2(0.0f, 0.0f), glm::vec2(20.0f, 0.0f), glm::vec2(10.0f, 10.0f), kWidth));

    ASSERT_TRUE(Physics::IsPointInThickLine(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 200.0f), glm::vec2(0.0f, 50.0f), kWidth));
    ASSERT_FALSE(Physics::IsPointInThickLine(glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 200.0f), glm::vec2(10.0f, 50.0f), kWidth));

    ASSERT_TRUE(Physics::IsPointInThickLine(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 10.0f), glm::vec2(5.0f, 5.0f), kWidth));
    ASSERT_FALSE(Physics::IsPointInThickLine(glm::vec2(0.0f, 0.0f), glm::vec2(10.0f, 10.0f), glm::vec2(0.0f, 5.0f), kWidth));
}

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
