#include "physics.hpp"
#include <glm/glm.hpp>

namespace Physics
{
    bool IsLineSegmentsIntersecting(const glm::vec2& line1p1, const glm::vec2& line1p2, const glm::vec2& line2p1, const glm::vec2& line2p2, raycast_collision* const collision)
    {
        // Get the segments' parameters.
        const float dx12 = line1p2.x - line1p1.x;
        const float dy12 = line1p2.y - line1p1.y;
        const float dx34 = line2p2.x - line2p1.x;
        const float dy34 = line2p2.y - line2p1.y;

        // Solve for t1 and t2
        const float denominator = (dy12 * dx34 - dx12 * dy34);
        if (denominator == 0.0f) // Collinear
        {
            // collision is undefined when there isn't a collision
            return false;
        }

        const float t1 = ((line1p1.x - line2p1.x) * dy34 + (line2p1.y - line1p1.y) * dx34) / denominator;
        const float t2 = ((line2p1.x - line1p1.x) * dy12 + (line1p1.y - line2p1.y) * dx12) / -denominator;

        // The segments intersect if t1 and t2 are between 0 and 1.
        const bool intersecting = (t1 >= 0.0f) && (t1 <= 1.0f) && (t2 >= 0.0f) && (t2 <= 1.0f);

        // Find the point of intersection.
        if (collision && intersecting)
        {
            collision->intersection = glm::vec2(line1p1.x + dx12 * t1, line1p1.y + dy12 * t1);
        }

        return intersecting;
    }

    // Compute the dot product AB . BC
    f32 DotProduct(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC)
    {
        glm::vec2 AB = pointB - pointA;
        glm::vec2 BC = pointC - pointB;
        return glm::dot(AB, BC);
    }

    // Compute the cross product AB x AC
    f32 CrossProduct(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC)
    {
        const glm::vec2 AB = pointB - pointA;
        const glm::vec2 AC = pointC - pointA;
        return AB.x * AC.y - AB.y * AC.x;
    }

    f32 LineToPointDistance(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC)
    {
        const f32 dot1 = DotProduct(pointA, pointB, pointC);
        if (dot1 > 0.0f)
        {
            return glm::distance(pointB, pointC);
        }

        const f32 dot2 = DotProduct(pointB, pointA, pointC);
        if (dot2 > 0.0f)
        {
            return glm::distance(pointA, pointC);
        }

        const f32 dist = CrossProduct(pointA, pointB, pointC) / glm::distance(pointA, pointB);
        return glm::abs(dist);
    }

    
    f32 Sign(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3)
    {
         return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    }

    bool IsPointInTriangle(const glm::vec2& triPointA, const glm::vec2& triPointB, const glm::vec2& triPointC, const glm::vec2& point)
    {
        const bool b1 = Sign(point, triPointA, triPointB) < 0.0f;
        const bool b2 = Sign(point, triPointB, triPointC) < 0.0f;
        const bool b3 = Sign(point, triPointC, triPointA) < 0.0f;
        return (b1 == b2) && (b2 == b3);
    }

    bool IsPointInThickLine(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC, f32 width)
    {
        const f32 dist = LineToPointDistance(pointA, pointB, pointC);
        return dist <= (width / 2.0f) + 0.5f;
    }

    bool IsPointInCircle(const glm::vec2& circleCentre, f32 radius, const glm::vec2& point)
    {
        const f32 distance = glm::pow(circleCentre.x - point.x, 2.0f) + glm::pow(circleCentre.y - point.y, 2.0f);
        return distance <= glm::pow(radius, 2.0f);
    }
}
