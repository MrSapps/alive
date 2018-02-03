#include "core/systems/camerasystem.hpp"

DEFINE_SYSTEM(CameraSystem);

void CameraSystem::Update()
{

}

void CameraSystem::SetGameCameraToCameraAt(u32 x, u32 y)
{
    mCameraPosition = glm::vec2(
        (x * mCameraBlockSize.x) + mCameraBlockImageOffset.x,
        (y * mCameraBlockSize.y) + mCameraBlockImageOffset.y) + glm::vec2(mVirtualScreenSize.x / 2, mVirtualScreenSize.y / 2
    );
}