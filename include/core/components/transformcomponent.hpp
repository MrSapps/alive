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
    void Set(float x, float y);
    void Set(float x, float y, float width, float height);
    void SetX(float x);
    void SetY(float y);
    void SetWidth(float width);
    void SetHeight(float height);

    float GetX() const;
    float GetY() const;
    float GetWidth() const;
    float GetHeight() const;

public:
    void Add(float xAmount, float yAmount);
    void AddX(float xAmount);
    void AddY(float yAmount);

public:
    void SnapXToGrid();

private:
    struct {
        float mX;
        float mY;
        float mWidth;
        float mHeight;
    } mData = {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };
};