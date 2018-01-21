#include "core/entity.hpp"
#include "core/components/transform.hpp"
#include "core/components/physics.hpp"

void PhysicsComponent::Load()
{
    mTransformComponent = mEntity->GetComponent<TransformComponent>(ComponentIdentifier::Transform);
}

void PhysicsComponent::Update()
{
    mTransformComponent->AddX(xSpeed);
    mTransformComponent->AddY(ySpeed);
}