#include <algorithm>

#include "core/systems/collisionsystem.hpp"

DEFINE_SYSTEM(CollisionSystem)

void CollisionSystem::Update()
{

}

void CollisionSystem::AddCollisionLine(CollisionLine *)
{

}

void CollisionSystem::RemoveCollisionLine(CollisionLine*)
{

}

void CollisionSystem::ClearCollisionLines()
{

}

CollisionRaycast CollisionSystem::Raycast(glm::vec2, glm::vec2, float, std::initializer_list<CollisionLine::eLineTypes>) const
{
    return {};
}