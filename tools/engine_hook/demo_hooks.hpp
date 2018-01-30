#pragma once

#include "game_objects.hpp"

struct InputPadObject
{
    DWORD field_0;
    BYTE field_4;
    BYTE field_5;
    WORD field_6;
    DWORD field_8;
    DWORD field_C;
    DWORD field_10;
    DWORD field_14;
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
