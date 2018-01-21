#include "core/entity.hpp"
#include "core/components/transform.hpp"
#include "core/components/physics.hpp"
#include "mapobject.hpp"

void PhysicsComponent::Load()
{
    mTransformComponent = mEntity->GetComponent<TransformComponent>(ComponentIdentifier::Transform);
}

void PhysicsComponent::Update()
{
    mTransformComponent->AddX(xSpeed);
    mTransformComponent->AddY(ySpeed);
}

void PhysicsComponent::SnapXToGrid()
{
    f32 snapped = ::SnapXToGrid(mTransformComponent->GetX());
    mTransformComponent->SetX(snapped);
}
