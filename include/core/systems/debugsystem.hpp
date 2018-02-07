#pragma once

#include <glm/glm/vec2.hpp>

#include "core/system.hpp"
#include "abstractrenderer.hpp"

class DebugSystem final : public System
{
public:
    DECLARE_SYSTEM(DebugSystem);

public:
    void Update() final;

public:
    void Render(AbstractRenderer& rend) const;

public:
    void RenderGrid(AbstractRenderer& rend) const;
    void RenderRaycast(AbstractRenderer& rend, const glm::vec2& from, const glm::vec2& to, CollisionLine::eLineTypes lineType, const glm::vec2& fromDrawOffset = glm::vec2()) const;
};