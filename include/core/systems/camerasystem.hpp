#include <glm/glm/vec2.hpp>

#include "core/system.hpp"

class CameraSystem : public System
{
public:
    DECLARE_SYSTEM(CameraSystem);

public:
    void Update() final;

public:
    void Render() const;

public:
    glm::vec2 kVirtualScreenSize;
    glm::vec2 kCameraBlockSize;
    glm::vec2 kCamGapSize;
    glm::vec2 kCameraBlockImageOffset;
};