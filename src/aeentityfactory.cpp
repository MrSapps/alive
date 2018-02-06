#include <map>

#include "core/entity.hpp"
#include "core/components/animationcomponent.hpp"
#include "core/components/transformcomponent.hpp"

#include "oddlib/stream.hpp"

#include "aeentityfactory.hpp"
#include "gridmap.hpp"


static Entity* MakeTrapDoor(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    Entity* entity = entityManager.CreateEntityWith<TransformComponent, AnimationComponent>();

    ReadU16(ms); // id
    u16 startState = ReadU16(ms); // startState
    ReadU16(ms); // selfClosing
    ReadU16(ms); // scale
    ReadU16(ms); // destinationLevel
    ReadU16(ms); // direction
    ReadU16(ms); // animationOffset
    ReadU16(ms); // openDuration

    entity->GetComponent<TransformComponent>()->Set(static_cast<float>(object.mRectTopLeft.mX), static_cast<float>(object.mRectTopLeft.mY));

    if (startState == 0) // closed
        entity->GetComponent<AnimationComponent>()->Change("TRAPDOOR.BAN_1004_AePc_1");
    else if (startState == 1) // open
        entity->GetComponent<AnimationComponent>()->Change("TRAPDOOR.BAN_1004_AePc_2");

    return entity;
}


static std::map<const u32, std::function<Entity*(const Oddlib::Path::MapObject&, EntityManager&, Oddlib::MemoryStream&)>> sEnumMap =
{
    {ObjectTypesAe::eTrapDoor, MakeTrapDoor}
};


Entity* AeEntityFactory::Create(const Oddlib::Path::MapObject& object, EntityManager& entityManager, Oddlib::MemoryStream& ms)
{
    return sEnumMap[object.mType](object, entityManager, ms);
}