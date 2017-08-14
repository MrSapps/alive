#pragma once

#include "types.hpp"
#include <string>

class GameObjectList
{
public:
    struct BaseObj
    {
        void* mVTable;
        u16 mTypeId;
        u8 mState;
        u8 pad1;
        u32 pad2;
        u32* mLoadedBans;

        // TODO: Correct offsets + offsets change per game
        u32 mXPos;
        u32 mYPos;
        u32 mVelX;
        u32 mVelY;
        u32 mScale;
    };

    static std::string AeTypeToString(u16 type);

    struct Objs
    {
        BaseObj** mPointerToObjects;
        u16 mCount;
    };

    static void LogObjects();
    static Objs* GetObjectsPtr();
};
