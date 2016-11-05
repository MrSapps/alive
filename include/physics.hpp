#pragma once

#include <glm/vec2.hpp>
#include "types.hpp"

namespace Physics
{
    struct raycast_collision
    {
        glm::vec2 intersection;
    };

    bool IsLineSegmentsIntersecting(const glm::vec2& line1p1, const glm::vec2& line1p2, const glm::vec2& line2p1, const glm::vec2& line2p2, raycast_collision* const collision);
    bool IsPointInTriangle(const glm::vec2& triPointA, const glm::vec2& triPointB, const glm::vec2& triPointC, const glm::vec2& point);
    bool IsPointInThickLine(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC, f32 width);
    bool IsPointInCircle(const glm::vec2& circleCentre, f32 radius, const glm::vec2& point);
}
