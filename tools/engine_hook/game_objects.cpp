#include "game_objects.hpp"
#include "logger.hpp"
#include "addresses.hpp"
#include "hook_utils.hpp"
#include <map>

static std::map<u32, std::string> gAeObjectNames = 
{
    { 0, "No ID"},
    { 7, "Animation" },
    { 13, "Brew Machine" },
    { 30, "Grinder" },
    { 33, "Door" },
    { 34, "Door Lock" },
    { 35, "Bird" },
    { 39, "Electrocute" },
    { 48, "Rock Spawner" },
    { 50, "Fleech" },
    { 53, "Item Count" },
    { 54, "Flying Slig" },
    { 61, "Locked Soul" },
    { 64, "Greeter" },
    { 67, "Gluckon" },
    { 68, "Help Phone" },
    { 69, "Hero" },
    { 78, "Pulley" },
    { 83, "Anti Chant" },
    { 84, "Meat" },
    { 85, "Meat Sack" },
    { 88, "Mine" },
    { 91, "Greeter Body" },
    { 95, "Pause Menu" },
    { 96, "Paramite" },
    { 103, "Pull Rope" },
    { 105, "Rock" },
    { 106, "Rock Sack" },
    { 110, "Mudokon" },
    { 111, "Red Laser" },
    { 112, "Scrab" },
    { 122, "Gate" },
    { 124, "Snooz Particle" },
    { 125, "Slig" },
    { 126, "Slog" },
    { 129, "Slug" },
    { 134, "Particle" },
    { 139, "Lever" },
    { 142, "Trapdoor" },
    { 143, "UXB" },
    { 146, "Web" }
};

static std::map<u32, std::string> gAoObjectNames =
{
    { 0, "No ID" },
    { 43, "Hero" },
    { 67, "FG1" },
};

std::string GameObjectList::AeTypeToString(u16 type)
{
    if (Utils::IsAe())
    {
        auto it = gAeObjectNames.find(static_cast<u32>(type));
        if (it != std::end(gAeObjectNames))
        {
            return it->second;
        }
    }
    else
    {
        auto it = gAoObjectNames.find(static_cast<u32>(type));
        if (it != std::end(gAoObjectNames))
        {
            return it->second;
        }
    }

    return "Unknown(" + std::to_string(type) + ")";
}

void GameObjectList::LogObjects()
{
    auto p = GetObjectsPtr();
    if (p)
    {
        for (int i = 0; i < p->mCount; i++)
        {
            auto objPtr = p->mArray[i];
            if (objPtr)
            {
                LOG_INFO("Ptr is " << AeTypeToString(objPtr->field_4_typeId));
            }
        }
    }
}

GameObjectList::Objs<BaseAnimatedWithPhysicsGameObject*>* GameObjectList::GetObjectsPtr()
{
    auto p = (Objs<BaseAnimatedWithPhysicsGameObject*>**)Addrs().ObjectList();
    return *p;
}

BaseAnimatedWithPhysicsGameObject* GameObjectList::HeroPtr()
{
    const u32 heroId = Utils::IsAe() ? 69 : 43;

    auto ptrs = GetObjectsPtr();
    for (int i = 0; i < ptrs->mCount; i++)
    {
        if (ptrs->mArray[i]->field_4_typeId == heroId)
        {
            return ptrs->mArray[i];
        }
    }
    return nullptr;
}

template<class T>
static T OffsetAs(void* ptr, u32 offset)
{
    BYTE* pThis = (BYTE*)ptr;

    pThis += offset;

    T value = *reinterpret_cast<s32*>(pThis);

    return value;
}

HalfFloat BaseAnimatedWithPhysicsGameObject::xpos()
{
    if (Utils::IsAe())
    {
        return HalfFloat(field_B8_xpos);
    }
    return HalfFloat(OffsetAs<s32>(this, 0xA8));
}

HalfFloat BaseAnimatedWithPhysicsGameObject::ypos()
{
    if (Utils::IsAe())
    {
        return HalfFloat(field_BC_ypos);
    }
    return HalfFloat(OffsetAs<s32>(this, 0xAC));
}

HalfFloat BaseAnimatedWithPhysicsGameObject::velocity_x()
{
    if (Utils::IsAe())
    {
        return HalfFloat(field_C4_velx);
    }
    return HalfFloat(OffsetAs<s32>(this, 0xB4));
}

HalfFloat BaseAnimatedWithPhysicsGameObject::velocity_y()
{
    if (Utils::IsAe())
    {
        return HalfFloat(field_C8_vely);
    }
    return HalfFloat(OffsetAs<s32>(this, 0xB8));
}
