#pragma once

#include "core/component.hpp"
#include "resourcemapper.hpp"

class TransformComponent : public Component
{
public:
    float xPos;
    float yPos;
};

class AnimationComponent : public Component
{
public:
    void Load(ResourceLocator& resLoc, const char* animationName);
    void Update() final;
    void Render(AbstractRenderer& rend) const final;
private:
    std::unique_ptr<Animation> mAnimation;
private:
    TransformComponent* mTransformComponent = nullptr;
};

class PhysicsComponent : public Component
{
public:
    void Load();
    void Update() final;
public:
    float xSpeed = 0.0f;
    float ySpeed = 0.0f;
private:
    TransformComponent* mTransformComponent = nullptr;
};

class AbeMovementControllerComponent : public Component
{
public:
    void Load();
    void Update() override;
public:
    bool left = false;
    bool right = false;
private:
    const Actions* mInputMappingActions = nullptr;
    PhysicsComponent* mPhysicsComponent = nullptr;
};

class PlayerControllerComponent : public Component {
public:
    void Load(const InputState& state);
    void Update() override;
private:
    const Actions* mInputMappingActions = nullptr;
    AbeMovementControllerComponent *mAbeMovement = nullptr;
};