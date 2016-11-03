#pragma once

#include <glm/vec2.hpp>
#include "types.hpp"

namespace Physics
{
    struct raycast_collision
    {
        glm::vec2 intersection;
    };

    bool raycast_lines(const glm::vec2& line1p1, const glm::vec2& line1p2, const glm::vec2& line2p1, const glm::vec2& line2p2, raycast_collision* collision);
    bool point_in_thick_line(const glm::vec2& pointA, const glm::vec2& pointB, const glm::vec2& pointC, f32 width);
}
