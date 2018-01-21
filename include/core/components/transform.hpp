#pragma once

#include "core/component.hpp"

class TransformComponent final : public Component
{
public:
    void Set(float xPos, float yPos);
    void SetX(float xPos);
    void SetY(float yPos);
    float GetX() const;
    float GetY() const;
public:
    void AddX(float xAmount);
    void AddY(float yAmount);
private:
    float mXPos = 0.0f;
    float mYPos = 0.0f;
};