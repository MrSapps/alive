#include "core/entity.hpp"
#include "core/components/animation.h"

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

AbeEntity::AbeEntity(ResourceLocator& resLoc)
{
    auto physics = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    auto animation = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);

    physics->Load();
    animation->Load(resLoc, "AbeStandIdle");
}

SligEntity::SligEntity(ResourceLocator& resLoc)
{
    auto physics = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    auto animation = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);

    physics->Load();
    animation->Load(resLoc, "AbeStandIdle");

    physics->SetX(32.0f);
}