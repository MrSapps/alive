#pragma once

#include <initializer_list>

#include "collisionline.hpp"
#include "core/system.hpp"

class CollisionRaycast;

class CollisionSystem : public System
{
public:
    DECLARE_SYSTEM(CollisionSystem);

public:
    void Update() final;

public:
    void AddCollisionLine(CollisionLine* line);
    void RemoveCollisionLine(CollisionLine* line);
    void ClearCollisionLines();

public:
    CollisionRaycast Raycast(glm::vec2 origin, glm::vec2 direction, float distance, std::initializer_list<CollisionLine::eLineTypes> lineMask) const;
};

class CollisionRaycast final
{
private:
    float mDistance;
    glm::vec2 mPoint;
    glm::vec2 mOrigin;
};