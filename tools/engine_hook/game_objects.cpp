#include "game_objects.hpp"
#include "logger.hpp"

std::string GameObjectList::AeTypeToString(u16 type)
{
    switch (type)
    {
    case 0:
        return "No ID";
    case 7:
        return "Animation";
    case 13:
        return "Brew Machine";
    case 30:
        return "Grinder";
    case 33:
        return "Door";
    case 34:
        return "Door Lock";
    case 35:
        return "Bird";
    case 39:
        return "Electrocute";
    case 48:
        return "Rock Spawner";
    case 50:
        return "Fleech";
    case 53:
        return "Item Count";
    case 54:
        return "Flying Slig";
    case 61:
        return "Locked Soul";
    case 64:
        return "Greeter";
    case 67:
        return "Gluckon";
    case 68:
        return "Help Phone";
    case 69:
        return "Hero";
    case 78:
        return "Pulley";
    case 83:
        return "Anti Chant";
    case 84:
        return "Meat";
    case 85:
        return "Meat Sack";
    case 88:
        return "Mine";
    case 91:
        return "Greeter Body";
    case 95:
        return "Pause Menu";
    case 96:
        return "Paramite";
    case 103:
        return "Pull Rope";
    case 105:
        return "Rock";
    case 106:
        return "Rock Sack";
    case 110:
        return "Mudokon";
    case 111:
        return "Red Laser";
    case 112:
        return "Scrab";
    case 122:
        return "Gate";
    case 124:
        return "Snooz Particle";
    case 125:
        return "Slig";
    case 126:
        return "Slog";
    case 129:
        return "Slug";
    case 134:
        return "Particle";
    case 139:
        return "Lever";
    case 142:
        return "Trapdoor";
    case 143:
        return "UXB";
    case 146:
        return "Web";
    default:
        return "Unknown";
    }
}

void GameObjectList::LogObjects()
{
    Objs* p = GetObjectsPtr();
    if (p)
    {
        for (int i = 0; i < p->mCount; i++)
        {
            BaseObj* objPtr = p->mPointerToObjects[i];
            if (objPtr)
            {
                LOG_INFO("Ptr is " << AeTypeToString(objPtr->mTypeId));
            }
        }
    }
}

GameObjectList::Objs* GameObjectList::GetObjectsPtr()
{
    Objs** ppp = (Objs**)0x00BB47C4;
    Objs* p = *ppp;
    return p;
}
