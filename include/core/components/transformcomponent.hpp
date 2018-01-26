#pragma once

#include "core/component.hpp"

class TransformComponent final : public Component
{
public:
    DECLARE_COMPONENT(TransformComponent);

public:
    void Serialize(std::ostream &os) const final;
    void Deserialize(std::istream &is) final;

public:
    void Set(float xPos, float yPos);
    void SetX(float xPos);
    void SetY(float yPos);
    float GetX() const;
    float GetY() const;
public:
    void Add(float xAmount, float yAmount);
    void AddX(float xAmount);
    void AddY(float yAmount);
public:
    void SnapXToGrid();
private:
    struct {
        float mXPos;
        float mYPos;
    } mData = {};
};