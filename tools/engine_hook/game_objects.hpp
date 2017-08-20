#pragma once

#include "types.hpp"
#include <string>

// First 16bits is the whole number, last 16bits is the remainder (??)
class HalfFloat
{
public:
    HalfFloat() = default;
    HalfFloat(s32 value) : mValue(value) { }
    f64 AsDouble() const { return static_cast<double>(mValue) / 65536.0; }
    HalfFloat& operator = (int value) { mValue = value; return *this; }
    bool operator != (const HalfFloat& r) const { return mValue != r.mValue; }
private:
    friend HalfFloat operator -(const HalfFloat& l, const HalfFloat& r);
    s32 mValue;
};

inline HalfFloat operator - (const HalfFloat& l, const HalfFloat& r) { return HalfFloat(l.mValue - r.mValue); }

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
        u32 pad3;
        u32* mLoadedBans;

        // TODO: Correct offsets + offsets change per game
        u32 mXPos;
        u32 mYPos;
        u32 mVelX;
        u32 mVelY;
        u32 mScale;
        HalfFloat xpos();
        HalfFloat ypos();
        HalfFloat velocity_x();
        HalfFloat velocity_y();
    };

    static std::string AeTypeToString(u16 type);

    struct Objs
    {
        BaseObj** mPointerToObjects;
        u16 mCount;
    };

    static void LogObjects();
    static Objs* GetObjectsPtr();
    static BaseObj* HeroPtr();
};
