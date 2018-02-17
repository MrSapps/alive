#include "core/entity.hpp"
#include "core/entitymanager.hpp"
#include "core/systems/camerasystem.hpp"
#include "core/components/cameracomponent.hpp"

DEFINE_COMPONENT(CameraComponent);

void CameraComponent::OnLoad()
{
    mEntity.GetManager()->GetSystem<CameraSystem>()->mTarget = mEntity;
}
