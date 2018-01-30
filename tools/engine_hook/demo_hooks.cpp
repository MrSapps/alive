#include "demo_hooks.hpp"
#include "game_functions.hpp"

void DemoHooksForceLink() {} 

ALIVE_FUNC_NOT_IMPL(0x0, 0x4FA9C0, int __cdecl(int controllerNumber), sub_4FA9C0);
ALIVE_VAR(0x0, 0x5C1BBE, WORD, word_5C1BBE);
ALIVE_VAR(0x0, 0x5C1B9A, WORD, word_5C1B9A);
ALIVE_VAR(0x0, 0x5C1B84, DWORD, gnFrame_dword_5C1B84);



char __cdecl Input_Command_Convert_404354(__int16 cmd)
{
    unsigned int v1 = 0;
    if (cmd & 8)
    {
        v1 = 1;
    }

    if (cmd & 2)
    {
        ++v1;
    }

    if (cmd & 4)
    {
        ++v1;
    }

    if (cmd & 1)
    {
        ++v1;
    }

    if (v1 > 1)
    {
        return 0;
    }

    char result = 0;
    if (cmd & 0x1000)
    {
        result = 1;
    }

    if (cmd & 0x4000)
    {
        result |= 2u;
    }

    if (cmd < 0)
    {
        result |= 4u;
    }

    if (cmd & 0x2000)
    {
        result |= 8u;
    }

    if (cmd & 8)
    {
        result |= 0x10u;
    }

    if (cmd & 2)
    {
        result |= 0x40u;
    }

    if (!(cmd & 4) && !(cmd & 1))
    {
        if ((cmd & 0x80u) != 0)
        {
            result |= 0x20u;
        }

        if (cmd & 0x20)
        {
            result |= 0x80u;
        }
    }
    return result;
}

const unsigned char byte_545A4C[20] =
{
    0, 64, 192, 0, 128, 96, 160, 128, 0, 32,
    224, 0, 0, 64, 192, 0, 0, 0, 0, 0
};

char __fastcall Input_update_45F040(InputObject* pThis, void*)
{
    pThis->field_0_pads[0].field_8 = pThis->field_0_pads[0].field_0;
    pThis->field_0_pads[0].field_0 = sub_4FA9C0(0);

    if (pThis->field_38_bDemoPlaying & 1)
    {
        if (pThis->field_0_pads[word_5C1BBE].field_0)
        {
            word_5C1B9A = 0;
            pThis->field_38_bDemoPlaying &= 0xFFFEu;
            return static_cast<char>(word_5C1BBE);
        }

        if (gnFrame_dword_5C1B84 >= pThis->field_40_command_duration)
        {
            const DWORD command = (*pThis->field_30_pDemoRes)[pThis->field_34_demo_command_index];
            pThis->field_34_demo_command_index++;
            pThis->field_3C_command = command >> 16;
            pThis->field_40_command_duration = gnFrame_dword_5C1B84 + (unsigned __int16)command;
            
            // End demo/quit command
            if ((command & 0x8000) != 0)
            {
                pThis->field_38_bDemoPlaying &= 0xFFFE;
            }
        }

        if (pThis->field_38_bDemoPlaying & 1)
        {
            pThis->field_0_pads[0].field_0 = Input_Command_Convert_404354(LOWORD(pThis->field_3C_command));
        }
    }

    pThis->field_0_pads[0].field_10 = pThis->field_0_pads[0].field_8 & ~pThis->field_0_pads[0].field_0;
    pThis->field_0_pads[0].field_C = pThis->field_0_pads[0].field_0 & ~pThis->field_0_pads[0].field_8;
    pThis->field_0_pads[0].field_4 = byte_545A4C[pThis->field_0_pads[0].field_0 & 0xF];
    pThis->field_0_pads[1].field_8 = pThis->field_0_pads[1].field_0;

    pThis->field_0_pads[1].field_0 = sub_4FA9C0(1);
    pThis->field_0_pads[1].field_10 = pThis->field_0_pads[1].field_8 & ~pThis->field_0_pads[1].field_0;
    pThis->field_0_pads[1].field_C = pThis->field_0_pads[1].field_0 & ~pThis->field_0_pads[1].field_8;
    pThis->field_0_pads[1].field_4 = byte_545A4C[pThis->field_0_pads[1].field_0 & 0xF];

    return pThis->field_0_pads[1].field_4;
}
ALIVE_FUNC_IMPLEX(0x0, 0x45F040, Input_update_45F040, true);
