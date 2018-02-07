#include <glm/glm/vec2.hpp>

#include "types.hpp"
#include "core/system.hpp"
#include "core/entity.hpp"

class Entity;

class CameraSystem final : public System
{
public:
    DECLARE_SYSTEM(CameraSystem);

public:
    void Update() final;

public:
    void SetGameCameraToCameraAt(u32 x, u32 y);

public:
    Entity mTarget;

public:
    glm::vec2 mCameraPosition;
    glm::vec2 mVirtualScreenSize;

public:
    glm::vec2 mCamGapSize;
    glm::vec2 mCameraBlockSize;
    glm::vec2 mCameraBlockImageOffset;

public:
    static constexpr u8 kEditorGridSizeX = 25;
    static constexpr u8 kEditorGridSizeY = 20;
};