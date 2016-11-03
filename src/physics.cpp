#include "physics.hpp"
#include <glm/glm.hpp>

namespace Physics
{
    bool raycast_lines(const glm::vec2& line1p1, const glm::vec2& line1p2, const glm::vec2& line2p1, const glm::vec2& line2p2, raycast_collision * collision)
    {
        //bool lines_intersect = false;
        bool segments_intersect = false;
        glm::vec2 intersection;
        glm::vec2 close_p1;
        glm::vec2 close_p2;

        // Get the segments' parameters.
        float dx12 = line1p2.x - line1p1.x;
        float dy12 = line1p2.y - line1p1.y;
        float dx34 = line2p2.x - line2p1.x;
        float dy34 = line2p2.y - line2p1.y;

        // Solve for t1 and t2
        float denominator = (dy12 * dx34 - dx12 * dy34);

        float t1 =
            ((line1p1.x - line2p1.x) * dy34 + (line2p1.y - line1p1.y) * dx34)
            / denominator;
        if (glm::isinf(t1))
        {
            // The lines are parallel (or close enough to it).
            //lines_intersect = false;
            segments_intersect = false;
            const float fnan = nanf("");
            intersection = glm::vec2(fnan, fnan);
            close_p1 = glm::vec2(fnan, fnan);
            close_p2 = glm::vec2(fnan, fnan);
            if (collision)
                collision->intersection = intersection;

            return segments_intersect;
        }
        //lines_intersect = true;

        float t2 =
            ((line2p1.x - line1p1.x) * dy12 + (line1p1.y - line2p1.y) * dx12)
            / -denominator;

        // Find the point of intersection.
        intersection = glm::vec2(line1p1.x + dx12 * t1, line1p1.y + dy12 * t1);
        if (collision)
            collision->intersection = intersection;

        // The segments intersect if t1 and t2 are between 0 and 1.
        segments_intersect =
            ((t1 >= 0) && (t1 <= 1) &&
            (t2 >= 0) && (t2 <= 1));

        // Find the closest points on the segments.
        if (t1 < 0)
        {
            t1 = 0;
        }
        else if (t1 > 1)
        {
            t1 = 1;
        }

        if (t2 < 0)
        {
            t2 = 0;
        }
        else if (t2 > 1)
        {
            t2 = 1;
        }

        close_p1 = glm::vec2(line1p1.x + dx12 * t1, line1p1.y + dy12 * t1);
        close_p2 = glm::vec2(line2p1.x + dx34 * t2, line2p1.y + dy34 * t2);

        return segments_intersect;
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
        glm::vec2 AB = pointB - pointA;
        glm::vec2 AC = pointC - pointA;
        f32 cross = AB.x * AC.y - AB.y * AC.x;
        return cross;
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

    bool point_in_thick_line(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC, f32 width)
    {
        const f32 dist = LineToPointDistance(pointA, pointB, pointC);
        if (dist < width / 2.0f)
        {
            return true;
        }
        return false;
    }
}
