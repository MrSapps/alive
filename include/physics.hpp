#pragma once

#include <glm/vec2.hpp>

namespace Physics
{
    struct raycast_collision
    {
        glm::vec2 intersection;
    };

    bool raycast_lines(const glm::vec2& line1p1, const glm::vec2& line1p2, const glm::vec2& line2p1, const glm::vec2& line2p2, raycast_collision* collision);
}
