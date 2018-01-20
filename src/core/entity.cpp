#include "core/entity.hpp"

void Component::SetEntity(class Entity *entity) {
    mEntity = entity;
}

void Component::SetId(ComponentIdentifier id) {
    mId = id;
}

ComponentIdentifier Component::GetId() const {
    return mId;
}

void AnimationComponent::Load(ResourceLocator& resLoc, const char* animationName)
{
    mAnimation = resLoc.LocateAnimation(animationName).get();
}

void AnimationComponent::Render(AbstractRenderer& rend) const
{
    mAnimation->Render(rend, false, AbstractRenderer::eLayers::eForegroundLayer1);
}

void AnimationComponent::Update()
{
    mAnimation->Update();
}

void PhysicsComponent::Update()
{
    if (mXSpeed > 0)
    {
        mXSpeed = mXSpeed - mXVelocity;
        if (FacingLeft())
        {
            if (mInvertX)
            {
                mXPos = mXPos + mXSpeed;
            }
            else
            {
                mXPos = mXPos - mXSpeed;
            }
        }
        else
        {
            if (mInvertX)
            {
                mXPos = mXPos - mXSpeed;
            }
            else
            {
                mXPos = mXPos + mXSpeed;
            }
        }
    }
}

void PhysicsComponent::SetX(float xPos) {
    mXPos = xPos;
}

void PhysicsComponent::SetY(float yPos) {
    mYPos = yPos;
}

void AbeControllerComponent::Load() {
    mPhysicsComponent = mEntity->GetComponent<PhysicsComponent>(ComponentIdentifier::Physics);
}

void AbeControllerComponent::Update() {

}

void Entity::AddChild(Entity::UPtr child)
{
    mChildren.emplace_back(std::move(child));
}

void Entity::Update()
{
    for (auto &component : mComponents)
    {
        component->Update();
    }
    for (auto &child : mChildren)
    {
        child->Update();
    }
}

void Entity::Render(AbstractRenderer& rend) const
{
    for (auto const &component : mComponents)
    {
        component->Render(rend);
    }
    for (auto const &child : mChildren)
    {
        child->Render(rend);
    }
}

AbeEntity::AbeEntity(ResourceLocator& resLoc)
{
    mPhysicsComponent = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    mAnimationComponent = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);

    mAnimationComponent->Load(resLoc, "AbeStandIdle");
}

SligEntity::SligEntity(ResourceLocator& resLoc)
{
    mPhysicsComponent = AddComponent<PhysicsComponent>(ComponentIdentifier::Physics);
    mAnimationComponent = AddComponent<AnimationComponent>(ComponentIdentifier::Animation);

    mPhysicsComponent->SetX(32.0f);
    mAnimationComponent->Load(resLoc, "AbeStandIdle");
}
