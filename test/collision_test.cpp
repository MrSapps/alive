#include <gmock/gmock.h>
#include "string_util.hpp"
#include "resourcemapper.hpp"
#include "gridmap.hpp"
#include "renderer.hpp"
#include <array>

class TestCoordinateSpace : public CoordinateSpace
{
public:
    TestCoordinateSpace(int w, int h)
    {
        mW = w;
        mH = h;
    }
    using CoordinateSpace::UpdateCamera;
};

/*
TEST(CoordinateSpace, WorldToScreen_11)
{
    TestCoordinateSpace coords(640, 480);
    coords.mCameraPosition = { 0.0f, 0.0f };
    coords.mScreenSize = { 640.0f, 480.0f };
    coords.UpdateCamera();

    const glm::vec2 actual = coords.WorldToScreen({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round(50.0f), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f), glm::round(actual.y));
}


TEST(CoordinateSpace, ScreenToWorld_11)
{
    TestCoordinateSpace coords(640, 480);
    coords.mCameraPosition = { 0.0f, 0.0f };
    coords.mScreenSize = { 640.0f, 480.0f };
    coords.UpdateCamera();

    const glm::vec2 actual = coords.ScreenToWorld({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round(50.0f - 640/2), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f - 480/2), glm::round(actual.y));
}
*/

/*
TEST(CoordinateSpace, WorldToScreen_12)
{
    TestCoordinateSpace coords(640*2, 480*2);
    coords.mCameraPosition = { 0.0f, 0.0f };
    coords.mScreenSize = { 640.0f, 480.0f };
    coords.UpdateCamera();

    const glm::vec2 actual = coords.WorldToScreen({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round(50.0f*2), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f*2), glm::round(actual.y));
}

TEST(CoordinateSpace, ScreenToWorld_12)
{
    TestCoordinateSpace coords(640 * 2, 480 * 2);
    coords.mCameraPosition = { 0.0f, 0.0f };
    coords.mScreenSize = { 640.0f, 480.0f };
    coords.UpdateCamera();

    const glm::vec2 actual = coords.ScreenToWorld({ 50.0f * 2, 100.0f * 2 });
    ASSERT_EQ(glm::round(50.0f), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f), glm::round(actual.y));
}

TEST(CoordinateSpace, WorldToScreen_11_offset)
{
    TestCoordinateSpace coords(640, 480);
    coords.mCameraPosition = { 10.0f, 10.0f };
    coords.mScreenSize = { 640.0f, 480.0f };
    coords.UpdateCamera();

    const glm::vec2 actual = coords.WorldToScreen({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round(50.0f + 10.0f), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f + 10.0f), glm::round(actual.y));
}

// TODO: Screen

TEST(CoordinateSpace, WorldToScreen_12_offset)
{
    TestCoordinateSpace coords(640*2, 480 * 2);
    coords.mCameraPosition = { 10.0f, 10.0f };
    coords.mScreenSize = { 640.0f, 480.0f };
    coords.UpdateCamera();

    const glm::vec2 actual = coords.WorldToScreen({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round((50.0f + 10.0f)*2), glm::round(actual.x));
    ASSERT_EQ(glm::round((100.0f + 10.0f)*2), glm::round(actual.y));
}

// TODO: Screen
*/

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
