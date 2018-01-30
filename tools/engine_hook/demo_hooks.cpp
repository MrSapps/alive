#include "demo_hooks.hpp"
#include "game_functions.hpp"
#include <vector>
#include "oddlib/stream.hpp"

void DemoHooksForceLink() {} 

ALIVE_FUNC_NOT_IMPL(0x0, 0x4FA9C0, int __cdecl(int controllerNumber), sub_4FA9C0);
ALIVE_VAR(0x0, 0x5C1BBE, WORD, word_5C1BBE);
ALIVE_VAR(0x0, 0x5C1B9A, WORD, word_5C1B9A);
ALIVE_VAR(0x0, 0x5C1B84, DWORD, gnFrame_dword_5C1B84);


DWORD __cdecl Input_Command_Convert_404354(DWORD cmd)
{
    unsigned int count = 0;
    if (cmd & 8)
    {
        count = 1;
    }

    if (cmd & 2)
    {
        ++count;
    }

    if (cmd & 4)
    {
        ++count;
    }

    if (cmd & 1)
    {
        ++count;
    }

    if (count > 1)
    {
        return 0x40000;
    }

    WORD flags = 0;
    if (cmd & 0x1000)
    {
        flags = 1;
    }

    if (cmd & 0x4000)
    {
        flags = flags | 2;
    }

    if ((cmd & 0x8000) != 0)
    {
        flags = flags | 4;
    }

    if (cmd & 0x2000)
    {
        flags = flags | 8;
    }

    if (cmd & 8)
    {
        flags = flags | 0x10;
    }

    if (cmd & 2)
    {
        flags = flags | 0x40;
    }

    if (cmd & 4)
    {
        if (cmd & 0x10)
        {
            flags |= 0x400u;
        }
    
        if ((cmd & 0x80u) != 0)
        {
            flags |= 0x800u;
        }

        if (cmd & 0x40)
        {
            flags |= 0x1000u;
        }

        if (cmd & 0x20)
        {
            flags |= 0x2000u;
        }
    }
    else if (cmd & 1)
    {
        if (cmd & 0x40)
        {
            flags |= 0x4000u;
        }

        if (cmd & 0x10)
        {
            flags |= 0x8000u;
        }
    }
    else
    {
        if (cmd & 0x10)
        {
            flags |= 0x100u;
        }

        if ((cmd & 0x80u) != 0)
        {
            flags = flags | 0x20;
        }

        if (cmd & 0x20)
        {
            flags = flags | 0x80;
        }

        if (cmd & 0x40)
        {
            flags |= 0x200u;
        }
    }
    return flags;
}
ALIVE_FUNC_IMPLEX(0x0, 0x404354, Input_Command_Convert_404354, false);

const unsigned char byte_545A4C[20] =
{
    0, // left?
    64, // up?
    192, // down?
    0,
    128, // right?
    96,
    160,
    128,
    0,
    32,
    224,
    0,
    0,
    64,
    192,
    0,
    0,
    0,
    0,
    0
};

static char UpdateImpl(InputObject* pThis)
{
    pThis->field_0_pads[0].field_8_previous = pThis->field_0_pads[0].field_0_pressed;
    pThis->field_0_pads[0].field_0_pressed = sub_4FA9C0(0);

    if (pThis->field_38_bDemoPlaying & 1)
    {
        // Stop if any button on any pad is pressed
        if (pThis->field_0_pads[word_5C1BBE].field_0_pressed)
        {
            word_5C1B9A = 0;
            pThis->field_38_bDemoPlaying &= 0xFFFEu;
            return static_cast<char>(word_5C1BBE);
        }

        if (gnFrame_dword_5C1B84 >= pThis->field_40_command_duration)
        {
            const DWORD command = (*pThis->field_30_pDemoRes)[pThis->field_34_demo_command_index++];
            pThis->field_3C_command = command >> 16;
            pThis->field_40_command_duration = gnFrame_dword_5C1B84 + command & 0xFFFF;

            // End demo/quit command
            if (command & 0x8000)
            {
                pThis->field_38_bDemoPlaying &= 0xFFFE;
            }
        }

        // Will do nothing if we hit the end command..
        if (pThis->field_38_bDemoPlaying & 1)
        {
            pThis->field_0_pads[0].field_0_pressed = Input_Command_Convert_404354(pThis->field_3C_command);
        }
    }

    pThis->field_0_pads[0].field_10_released = pThis->field_0_pads[0].field_8_previous & ~pThis->field_0_pads[0].field_0_pressed;
    pThis->field_0_pads[0].field_C_held = pThis->field_0_pads[0].field_0_pressed & ~pThis->field_0_pads[0].field_8_previous;
    pThis->field_0_pads[0].field_4_dir = byte_545A4C[pThis->field_0_pads[0].field_0_pressed & 0xF];

    pThis->field_0_pads[1].field_8_previous = pThis->field_0_pads[1].field_0_pressed;
    pThis->field_0_pads[1].field_0_pressed = sub_4FA9C0(1);
    pThis->field_0_pads[1].field_10_released = pThis->field_0_pads[1].field_8_previous & ~pThis->field_0_pads[1].field_0_pressed;
    pThis->field_0_pads[1].field_C_held = pThis->field_0_pads[1].field_0_pressed & ~pThis->field_0_pads[1].field_8_previous;
    pThis->field_0_pads[1].field_4_dir = byte_545A4C[pThis->field_0_pads[1].field_0_pressed & 0xF];

    return pThis->field_0_pads[1].field_4_dir;
}

char __fastcall Input_update_45F040(InputObject* pThis, void* )
{
    return UpdateImpl(pThis);
}
ALIVE_FUNC_IMPLEX(0x0, 0x45F040, Input_update_45F040, true);

signed int __fastcall Input_SetDemo_45F1E0(InputObject* pThis, void*, DWORD **pDemoRes)
{
    pThis->field_34_demo_command_index = 2; // First 2 dwords are some sort of header?
    pThis->field_30_pDemoRes = pDemoRes;
    pThis->field_38_bDemoPlaying |= 1u;
    pThis->field_40_command_duration = 0;
    return 1;
}
ALIVE_FUNC_IMPLEX(0x0, 0x45F1E0, Input_SetDemo_45F1E0, true);
