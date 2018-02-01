#include "core/system.hpp"

class CameraSystem : public System
{
public:
    DECLARE_SYSTEM(CameraSystem);

public:
    void Update() final;
};