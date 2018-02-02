#include "core/component.hpp"

class CameraComponent final : public Component
{
public:
    DECLARE_COMPONENT(CameraComponent);

public:
    void OnLoad() final;
};