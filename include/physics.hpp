#pragma once

#include <glm/vec2.hpp>
#include "types.hpp"

namespace Physics
{
    struct RaycastCollision
    {
        glm::vec2 mIntersection;
    };

    template<class T>
    inline bool PointInRect(T px, T py, T x, T y, T w, T h)
    {
        if (px < x) return false;
        if (py < y) return false;
        if (px >= x + w) return false;
        if (py >= y + h) return false;

        return true;
    }

    bool IsLineSegmentsIntersecting(const glm::vec2& line1p1, const glm::vec2& line1p2, const glm::vec2& line2p1, const glm::vec2& line2p2, RaycastCollision* const collision);
    bool IsPointInTriangle(const glm::vec2& triPointA, const glm::vec2& triPointB, const glm::vec2& triPointC, const glm::vec2& point);
    bool IsPointInThickLine(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC, f32 width);
    bool IsPointInCircle(const glm::vec2& circleCentre, f32 radius, const glm::vec2& point);
}
