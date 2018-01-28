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

struct Rect16
{
    WORD x, y, w, h;
};
ALIVE_ASSERT_SIZEOF(Rect16, 8);

struct Animation2
{
    void* field_0_VTable;
    DWORD field_4_flags;
    BYTE field_8_r;
    BYTE field_9_g;
    BYTE field_A_b;
    BYTE field_B_render_mode;
    WORD field_C_render_layer;
    WORD field_E_frame_change_counter;
};
ALIVE_ASSERT_SIZEOF(Animation2, 0x10);

#pragma pack(push)
#pragma pack(2)
struct AnimationEx : public Animation2
{
    WORD field_10_frame_delay;
    DWORD field_12_scale;
    DWORD field_16_dataOffset;
    WORD field_1A;
    DWORD field_1C;
    DWORD field_20_ppBlock;
    DWORD field_24_dbuf;
    WORD field_28_dbuf_size;

    BYTE field_2A[38];
    BYTE field_50[38];
    DWORD field_76_pad;

    Rect16 field_7A;
    Rect16 field_82;

    WORD field_8A_x;
    WORD field_8C_y;
    WORD field_8E_num_cols;
    WORD field_90_pad; // padding ??
    WORD field_92_current_frame;
    DWORD field_94_unknown_pointer; // Pointer to something
};
ALIVE_ASSERT_SIZEOF(AnimationEx, 0x98);
#pragma pack(pop)

struct BaseGameObject
{
    void* mVTbl;
    u16 field_4_typeId;
    FlagsUnion field_6_flags;
    u32 field_8_flagsEx;
    u32 field_C;
    GameObjectList::Objs field_10_resources_object_list;
    DWORD field_1C_update_delay;
};
ALIVE_ASSERT_SIZEOF(BaseGameObject, 0x20);

struct BaseAnimatedWithPhysicsGameObject : public BaseGameObject
{
    AnimationEx field_20_animation;
    DWORD field_B8_xpos;
    DWORD field_BC_ypos;
    WORD field_C0_path_number;
    WORD field_C2_lvl_number; // e.g BR is 9
    DWORD field_C4_velx;
    DWORD field_C8_vely;
    HalfFloat field_CC_sprite_scale; // The actual scale of the sprite, e.g when abe blows up and "flys" into the screen his meat chunks are huge
    WORD field_D0_r;
    WORD field_D2_g;
    WORD field_D4_b;
    WORD field_D6_scale; // 1 = "normal" 0 = background, 2/others = normal but can't interact with objects ?? Changing this will not resize the sprite
    WORD field_D8_yOffset; // These offset abes pos by this amount
    WORD field_DA_xOffset;
    WORD field_DC_bApplyShadows; // false = shadows zones have no effect
    WORD field_DE_pad; // No effect, padding?
    DWORD field_E0_176_ptr; // shadow pointer, nullptr = crash ?
};
ALIVE_ASSERT_SIZEOF(BaseAnimatedWithPhysicsGameObject, 0xE4);

#pragma pack(push)
#pragma pack(1)
struct BaseAliveGameObject : public BaseAnimatedWithPhysicsGameObject
{
    DWORD field_E4;
    DWORD field_E8;
    DWORD field_EC;
    DWORD field_F0;
    WORD field_F4;
    WORD field_F6;
    DWORD field_F8;
    DWORD* field_FC_pPathTLV;
    DWORD* field_100_pCollisionLine;
    WORD field_104;
    WORD field_106_animation_num;
    WORD field_108; // something to do with the turning state
    WORD field_10A;
    DWORD field_10C_health;
    DWORD field_110;
    BYTE field_114_flags;
    BYTE field_115_flags;
};
#pragma pack(pop)
ALIVE_ASSERT_SIZEOF(BaseAliveGameObject, 0x116);

struct Abe : public BaseAliveGameObject
{
    WORD field_116;
    WORD field_118;
    WORD field_11A;
    WORD field_11C; // 1 = abe dies when rolling off an edge by splatting
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
    BYTE field_1A2_rock_or_bone_count;
    BYTE field_1A3_throw_direction;
    DWORD field_1A4;
    DWORD field_1A8;
    WORD field_1AC;
    WORD field_1AE;
    DWORD field_1B0;
    DWORD field_1B4;
    DWORD field_1B8;
};
ALIVE_ASSERT_SIZEOF(Abe, 0x1BC);

struct BaseObj // Aka BaseAnimatedWithPhysicsGameObject
{
    Abe mAbe;

    HalfFloat xpos();
    HalfFloat ypos();
    HalfFloat velocity_x();
    HalfFloat velocity_y();
};
