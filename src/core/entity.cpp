#include "core/entity.hpp"
#include "core/components/physics.hpp"
#include "core/components/transform.hpp"
#include "core/components/animation.hpp"
#include "core/components/abemovement.hpp"
#include "core/components/sligmovement.hpp"

void Entity::AddChild(Entity::UPtr child)
{
    child->mParent = this;
    mChildren.emplace_back(std::move(child));
}

Entity* Entity::GetParent() const
{
    return mParent;
}

std::vector<Entity*> Entity::FindChildrenByComponent(ComponentIdentifier id)
{
    std::vector<Entity*> entities;
    for (auto& entity : mChildren)
    {
        if (entity->GetComponent<Component>(id) != nullptr)
        {
            entities.emplace_back(entity.get());
        }
    }
    return entities;
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

AbeEntity::AbeEntity(ResourceLocator& resLoc, InputState const& input) // TODO: Input wired here
{
    auto pos = AddComponent<TransformComponent>(ComponentIdentifier::Transform);
    auto controller = AddComponent<AbePlayerControllerComponent>(ComponentIdentifier::PlayerController);
    auto movement = AddComponent<AbeMovementComponent>(ComponentIdentifier::AbeMovementController);
    auto physics = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    auto animation = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);

    pos->Set(125.0f, 380.0f+(80.0f));
    physics->Load();
    animation->Load(resLoc, "AbeStandIdle");
    movement->Load();
    controller->Load(input);
}

SligEntity::SligEntity(ResourceLocator& resLoc, InputState const& input) // TODO: Input wired here
{
    auto pos = AddComponent<TransformComponent>(ComponentIdentifier::Transform);
    auto physics = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    auto animation = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);
	auto movement = AddComponent<SligMovementComponent>(ComponentIdentifier::SligMovementController);
	auto controller = AddComponent<SligPlayerControllerComponent>(ComponentIdentifier::PlayerController);

    pos->Set(0.0f, 380.0f);
    physics->Load();
    physics->xSpeed = 0.1f;
    animation->Load(resLoc, "SLIG.BND_412_AePc_11");
	movement->Load();
	controller->Load(input);
}