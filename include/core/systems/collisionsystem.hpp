#pragma once

#include <initializer_list>

#include "collisionline.hpp"
#include "core/system.hpp"

class CollisionRaycast;

class CollisionSystem final : public System
{
public:
    DECLARE_SYSTEM(CollisionSystem);

public:
    void Update() final;

public:
    void AddCollisionLine(std::unique_ptr<CollisionLine> line);
    void ClearCollisionLines();

public:
    CollisionRaycast Raycast(glm::vec2 origin, glm::vec2 direction, std::initializer_list<CollisionLine::eLineTypes> lineMask) const;

private:
    std::vector<std::unique_ptr<CollisionLine>> mCollisionLines;
};

class CollisionRaycast final
{
public:
    CollisionRaycast() = default;
    CollisionRaycast(float mDistance, const glm::vec2& mPoint, const glm::vec2& mOrigin);

public:
    explicit operator bool() const;

private:
    float mDistance = -1.0f;
    glm::vec2 mPoint = {0.0f, 0.0f};
    glm::vec2 mOrigin = {0.0f, 0.0f};
};