#pragma once

#include "types.hpp"
#include "half_float.hpp"
#include <string>
#include <windows.h>

#define ALIVE_ASSERT_SIZEOF(structureName, expectedSize) static_assert(sizeof(structureName) == expectedSize, "sizeof(" #structureName ") must be " #expectedSize)

struct BaseObj;

class GameObjectList
{
public:
    static std::string AeTypeToString(u16 type);

    struct Objs
    {
        BaseObj** mPointerToObjects; // This can actually be a pointer to any type depending on what the list is used for
        u16 mCount;
        u16 mMaxCount;
        u16 mExpandSize;
        u16 mFreeCount;
    };
    ALIVE_ASSERT_SIZEOF(Objs, 0xc);

    static void LogObjects();
    static Objs* GetObjectsPtr();
    static BaseObj* HeroPtr();
};

struct Abe_1BC_20_sub_object
{
    void* field_0_vtbl;
    BYTE field_4;
    BYTE field_5_flags;
    WORD field_6;
    BYTE field_8;
    BYTE field_9;
    BYTE field_A;
    BYTE field_B;
    WORD field_C;
    WORD field_E;
    DWORD field_10;
    DWORD field_14;
    WORD field_18_state;
    WORD field_1A;
};
ALIVE_ASSERT_SIZEOF(Abe_1BC_20_sub_object, 0x1c);

union FlagsUnion
{
    struct Parts
    {
        BYTE mLo;
        BYTE mHi;
    };
    Parts mParts;
    WORD mAll;
};
ALIVE_ASSERT_SIZEOF(FlagsUnion, 0x2);

struct BaseObj
{
    void* field_0_vtbl;
    u16 field_4_typeId;
    FlagsUnion field_6_flags2;
    u32 field_8;
    u32 field_C;
    GameObjectList::Objs field_10_resources_object_list;
    DWORD field_1C;
    Abe_1BC_20_sub_object field_20_obj;
    void* field_3C_vtbl;
    DWORD field_40;
    DWORD field_44;
    DWORD field_48;
    DWORD field_4C;
    DWORD field_50;
    DWORD field_54;
    DWORD field_58;
    DWORD field_5C;
    DWORD field_60;
    DWORD field_64;
    DWORD field_68;
    DWORD field_6C;
    DWORD field_70;
    DWORD field_74;
    DWORD field_78;
    DWORD field_7C;
    DWORD field_80;
    DWORD field_84;
    DWORD field_88;
    DWORD field_8C;
    DWORD field_90;
    DWORD field_94;
    DWORD field_98;
    DWORD field_9C;
    DWORD field_A0;
    DWORD field_A4;
    DWORD field_A8;
    DWORD field_AC;
    DWORD field_B0;
    DWORD field_B4;
    DWORD field_B8_xpos;
    DWORD field_BC_ypos;
    WORD field_C0;
    WORD field_C2;
    DWORD field_C4_velx;
    DWORD field_C8_vely;
    DWORD field_CC;
    WORD field_D0_127;
    WORD field_D2_127;
    WORD field_D4_127;
    WORD field_D6;
    WORD field_D8;
    WORD field_DA;
    WORD field_DC;
    WORD field_DE;
    DWORD field_E0_176_ptr;
    DWORD field_E4;
    DWORD field_E8;
    DWORD field_EC;
    DWORD field_F0;
    WORD field_F4;
    WORD field_F6;
    DWORD field_F8;
    DWORD field_FC;
    DWORD field_100;
    WORD field_104;
    WORD field_106;
    WORD field_108;
    WORD field_10A;
    DWORD field_10C;
    DWORD field_110;
    BYTE field_114_flags;
    BYTE field_115;
    WORD field_116;
    WORD field_118;
    WORD field_11A;
    WORD field_11C;
    WORD field_11E;
    WORD field_120;
    WORD field_122;
    DWORD field_124;
    Abe_1BC_20_sub_object field_128_obj_derived;
    DWORD field_144;
    DWORD field_148;
    DWORD field_14C;
    DWORD field_150;
    DWORD field_154;
    DWORD field_158;
    DWORD field_15C;
    DWORD field_160;
    DWORD field_164;
    DWORD field_168;
    WORD field_16C;
    WORD field_16E;
    DWORD field_170;
    WORD field_174;
    WORD field_176;
    DWORD field_178;
    DWORD field_17C;
    DWORD field_180;
    DWORD field_184;
    DWORD field_188;
    DWORD field_18C;
    DWORD field_190;
    DWORD field_194;
    WORD field_198;
    WORD field_19A;
    DWORD field_19C;
    WORD field_1A0;
    BYTE field_1A2;
    BYTE field_1A3;
    DWORD field_1A4;
    DWORD field_1A8;
    WORD field_1AC;
    WORD field_1AE;
    DWORD field_1B0;
    DWORD field_1B4;
    DWORD field_1B8;

    HalfFloat xpos();
    HalfFloat ypos();
    HalfFloat velocity_x();
    HalfFloat velocity_y();
};
ALIVE_ASSERT_SIZEOF(BaseObj, 0x1BC);
