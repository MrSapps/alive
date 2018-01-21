#include "core/entity.hpp"
#include "core/components/physics.hpp"
#include "core/components/transform.hpp"
#include "core/components/animation.hpp"
#include "core/components/abemovement.hpp"

void Entity::AddChild(Entity::UPtr child)
{
    mChildren.emplace_back(std::move(child));
}

void Entity::Update()
{
    for (auto& component : mComponents)
    {
        component->Update();
    }
    for (auto& child : mChildren)
    {
        child->Update();
    }
}

void Entity::Render(AbstractRenderer& rend) const
{
    for (auto const& component : mComponents)
    {
        component->Render(rend);
    }
    for (auto const& child : mChildren)
    {
        child->Render(rend);
    }
}

AbeEntity::AbeEntity(ResourceLocator& resLoc, InputState const &input) // TODO: Input wired here
{
    auto pos = AddComponent<TransformComponent>(ComponentIdentifier::Transform);
    auto physics = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    auto animation = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);
    auto movement = AddComponent<AbeMovementComponent>(ComponentIdentifier::AbeMovementController);
    auto controller = AddComponent<PlayerControllerComponent>(ComponentIdentifier::PlayerController);

	pos->Set(20.0f, 380.0f);
    physics->Load();
    animation->Load(resLoc, "AbeStandIdle");
    movement->Load();
    controller->Load(input);
}

SligEntity::SligEntity(ResourceLocator& resLoc)
{
    auto pos = AddComponent<TransformComponent>(ComponentIdentifier::Transform);
    auto physics = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    auto animation = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);

	pos->Set(0.0f, 380.0f);
    physics->Load();
    physics->xSpeed = 0.1f;
    animation->Load(resLoc, "SLIG.BND_412_AePc_11");
}