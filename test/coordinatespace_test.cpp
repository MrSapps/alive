#include <gmock/gmock.h>
#include "abstractrenderer.hpp"

class TestCoordinateSpace : public CoordinateSpace
{
public:
    TestCoordinateSpace(int w, int h)
    {
        mW = w;
        mH = h;
        mSmoothCameraPosition = false;
    }
};


TEST(CoordinateSpace, WorldToScreen_11)
{
    TestCoordinateSpace coords(640, 480);
    coords.SetCameraPosition({ 0.0f, 0.0f });
    coords.SetScreenSize({ 640.0f, 480.0f });

    const glm::vec2 actual = coords.WorldToScreen({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round((640.0f / 2) + 50.0f), glm::round(actual.x));
    ASSERT_EQ(glm::round((480.0f / 2) + 100.0f), glm::round(actual.y));
}


TEST(CoordinateSpace, ScreenToWorld_11)
{
    TestCoordinateSpace coords(640, 480);
    coords.SetCameraPosition({ 0.0f, 0.0f });
    coords.SetScreenSize({ 640.0f, 480.0f });

    const glm::vec2 actual = coords.ScreenToWorld({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round(50.0f - 640 / 2), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f - 480 / 2), glm::round(actual.y));
}


TEST(CoordinateSpace, WorldToScreen_12)
{
    TestCoordinateSpace coords(640 * 2, 480 * 2);
    coords.SetCameraPosition({ 0.0f, 0.0f });
    coords.SetScreenSize({ 640.0f, 480.0f });

    const glm::vec2 actual = coords.WorldToScreen({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round(640.0f + 50.0f * 2), glm::round(actual.x));
    ASSERT_EQ(glm::round(480.0f + 100.0f * 2), glm::round(actual.y));
}

TEST(CoordinateSpace, ScreenToWorld_12)
{
    TestCoordinateSpace coords(640 * 2, 480 * 2);
    coords.SetCameraPosition({ 0.0f, 0.0f });
    coords.SetScreenSize({ 640.0f, 480.0f });

    const glm::vec2 actual = coords.ScreenToWorld({ 640.0f + 50.0f * 2, 480.0f + 100.0f * 2 });
    ASSERT_EQ(glm::round(50.0f), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f), glm::round(actual.y));
}


TEST(CoordinateSpace, WorldToScreen_11_offset)
{
    TestCoordinateSpace coords(640, 480);
    coords.SetCameraPosition({ 10.0f, 10.0f });
    coords.SetScreenSize({ 640.0f, 480.0f });

    const glm::vec2 actual = coords.WorldToScreen({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round((640.0f / 2) + 50.0f - 10.0f), glm::round(actual.x));
    ASSERT_EQ(glm::round((480.0f / 2) + 100.0f - 10.0f), glm::round(actual.y));
}

TEST(CoordinateSpace, ScreenToWorld_11_offset)
{
    TestCoordinateSpace coords(640, 480);
    coords.SetCameraPosition({ 10.0f, 10.0f });
    coords.SetScreenSize({ 640.0f, 480.0f });

    const glm::vec2 actual = coords.ScreenToWorld({ (640.0f / 2) + 50.0f - 10.0f, (480.0f / 2) + 100.0f - 10.0f });
    ASSERT_EQ(glm::round(50.0f), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f), glm::round(actual.y));
}

TEST(CoordinateSpace, WorldToScreen_12_offset)
{
    TestCoordinateSpace coords(640 * 2, 480 * 2);
    coords.SetCameraPosition({ 10.0f, 10.0f });
    coords.SetScreenSize({ 640.0f, 480.0f });

    const glm::vec2 actual = coords.WorldToScreen({ 50.0f, 100.0f });
    ASSERT_EQ(glm::round((640.0f) + (50.0f - 10.0f) * 2), glm::round(actual.x));
    ASSERT_EQ(glm::round((480.0f) + (100.0f - 10.0f) * 2), glm::round(actual.y));
}

TEST(CoordinateSpace, ScreenToWorld_12_offset)
{
    TestCoordinateSpace coords(640 * 2, 480 * 2);
    coords.SetCameraPosition({ 10.0f, 10.0f });
    coords.SetScreenSize({ 640.0f, 480.0f });

    const glm::vec2 actual = coords.ScreenToWorld({ (640.0f) + (50.0f - 10.0f) * 2, (480.0f) + (100.0f - 10.0f) * 2 });
    ASSERT_EQ(glm::round(50.0f), glm::round(actual.x));
    ASSERT_EQ(glm::round(100.0f), glm::round(actual.y));
}
