#include "core/entity.hpp"
#include "core/components/transform.hpp"
#include "core/components/physics.hpp"
#include "mapobject.hpp"

DEFINE_COMPONENT(PhysicsComponent)

void PhysicsComponent::Load()
{
    mTransformComponent = mEntity->GetComponent<TransformComponent>();
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
