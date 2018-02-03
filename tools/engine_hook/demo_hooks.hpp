#pragma once

#include "game_objects.hpp"

struct InputPadObject
{
    DWORD field_0_pressed;
    BYTE field_4_dir;
    BYTE field_5;
    WORD field_6_padding; // Not confirmed
    DWORD field_8_previous;
    DWORD field_C_held;
    DWORD field_10_released;
    DWORD field_14_padding; // Not confirmed
};
ALIVE_ASSERT_SIZEOF(InputPadObject, 0x18);

struct InputObject
{
    InputPadObject field_0_pads[2];
    DWORD** field_30_pDemoRes;
    DWORD field_34_demo_command_index;
    WORD field_38_bDemoPlaying;
    WORD field_3A_pad_idx;
    DWORD field_3C_command;
    DWORD field_40_command_duration;
};
ALIVE_ASSERT_SIZEOF(InputObject, 0x44);

void DemoHooksForceLink();
char __fastcall Input_update_45F040(InputObject* pThis, void*);
