#include <glm/glm/vec2.hpp>

#include "core/system.hpp"

class Entity;

class CameraSystem final : public System
{
public:
    DECLARE_SYSTEM(CameraSystem);

public:
    void Update() final;

public:
    void Render() const;

public:
    Entity* mTarget = nullptr;

public:
    glm::vec2 mCameraPosition;
    glm::vec2 mVirtualScreenSize;

public:
    glm::vec2 mCamGapSize;
    glm::vec2 mCameraBlockSize;
    glm::vec2 mCameraBlockImageOffset;
};