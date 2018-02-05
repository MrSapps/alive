#include "demo_hooks.hpp"
#include "game_functions.hpp"
#include <vector>
#include "oddlib/stream.hpp"
#include "logger.hpp"
#include <sstream>
#include <iomanip>
#include "string_util.hpp"

void DemoHooksForceLink() {} 
ALIVE_VAR(0x0, 0x5C1B84, DWORD, gnFrame_dword_5C1B84);


ExternalDemoData gDemoData;

static bool sRecording = false;
static std::vector<DWORD> sRecBuffer;
static DWORD sRecBufferPos = 0;

static DWORD gLastFrame = 0;
static DWORD gLastPressed = 0;
static DWORD gLastReleased = 0;

void StartDemoRecording()
{
    sRecording = true;
    sRecBufferPos = 0;
    sRecBuffer.clear();
    sRecBuffer.reserve(1024 * 8);
    gLastFrame = gnFrame_dword_5C1B84;
}

void UpdateRecording(InputObject& input)
{
    //input.field_3C_command = (command << 16);
    //input.field_40_command_duration = gnFrame_dword_5C1B84 + command & 0xFFFF;
    
    // Has the input state changed?
    if (gLastPressed != input.field_0_pads[0].field_0_pressed || gLastReleased != input.field_0_pads[0].field_10_released)
    {
        // Input value to command value..
        LOG_INFO("Input state changed");

        // Update last values
        gLastPressed = input.field_0_pads[0].field_0_pressed;
        gLastReleased = input.field_0_pads[0].field_10_released;
        gLastFrame = gnFrame_dword_5C1B84;
        sRecBufferPos++;
    }
}

void EndDemoRecording(const std::string& fileName)
{
    sRecording = false;
    if (!sRecBuffer.empty() && !fileName.empty())
    {
        Oddlib::FileStream fs(fileName, Oddlib::IStream::ReadMode::ReadWrite);
        fs.Write(sRecBuffer);
        LOG_INFO("Wrote demo JOY (no header) to " << fileName);
    }
}


static bool sFakeInputEnabled = false;
void Demo_SetFakeInputEnabled(bool enable)
{
    sFakeInputEnabled = enable;
}

static DWORD sFakeInputValue = 0;
void Demo_SetFakeInputValue(const std::string& value)
{
    try
    {
        if (string_util::starts_with(value, "0x"))
        {
            std::string hexStr = value.substr(2);
            std::istringstream converter(hexStr);
            converter >> std::hex >> sFakeInputValue;
        }
        else
        {
            std::istringstream converter(value);
            converter >> std::dec >> sFakeInputValue;
        }
    }
    catch (const std::exception&)
    {
        sFakeInputValue = 0;
    }
    LOG_INFO("Fake input set to " << sFakeInputValue);
}


ALIVE_FUNC_NOT_IMPL(0x0, 0x4FA9C0, int __cdecl(int controllerNumber), sub_4FA9C0);
ALIVE_VAR(0x0, 0x5C1BBE, WORD, word_5C1BBE);
ALIVE_VAR(0x0, 0x5C1B9A, WORD, word_5C1B9A);

ALIVE_VAR(0x0, 0x5C3030, DWORD, gLevelObject_dword_5C3030); // Actually a class instance pointer or global object
ALIVE_FUNC_NOT_IMPL(0x0, 0x4047E1, signed __int16 __fastcall(void* pThis, void*, __int16 a1, __int16 a2, __int16 a3, __int16 a4, __int16 a5, __int16 a6), MapChange_4047E1);

enum DemoCommandBits
{
    eL2 = 0x1,
    eR2 = 0x2,
    eL1 = 0x4,
    eR1 = 0x8,
    eDPadUp = 0x1000,
    eDPadRight = 0x2000,
    eDPadDown = 0x4000,
    eDPadLeft = 0x8000,
    eTriangle = 0x10,
    eCircle = 0x20,
    eCross = 0x40,
    eSquare = 0x80,
    eEnd = 0x8000, // Ends demo, probably maps to start button or something
};

enum RawInputBits
{
    eUp = 0x1,
    eDown = 0x2,
    eLeft = 0x4,
    eRight = 0x8,
    eRun = 0x10,
    eDoAction = 0x20, // Pick up rock, pull lever etc
    eSneak = 0x40,
    eThrowItem = 0x80, // Or I say I dunno if no items
    eHop = 0x100,
    eFart = 0x200,
    eHello = 0x400,
    eFollowMe = 0x800,
    eWait = 0x1000,
    eWork = 0x2000,
    eAnger = 0x4000,
    eAllYa = 0x8000,
    eChant = 0x40000,
    eStopIt = 0x20000,
    eSorry = 0x10000,

    // Don't think its possible to have these in a demo command
    // 0x80000 = pause
    // 0x100000 = unpause
    // 0x400000 = tab/ddcheat

    // 0x200000 = nothing
    // 0x800000 = nothing
    // 0x1000000 = nothing
    // 0x2000000 = nothing
    // 0x4000000 = nothing
    // 0x8000000 = nothing
    // 0x10000000 = nothing
    // 0x20000000 = nothing
    // 0x40000000 = nothing
    // 0x80000000 = nothing
};

DWORD __cdecl Input_Command_Convert_404354(DWORD cmd)
{
    unsigned int shoulderButtonsPressedCount = 0;

    if (cmd & eL2)
    {
        ++shoulderButtonsPressedCount;
    }

    if (cmd & eR2)
    {
        ++shoulderButtonsPressedCount;
    }

    if (cmd & eL1)
    {
        ++shoulderButtonsPressedCount;
    }

    if (cmd & eR1)
    {
        ++shoulderButtonsPressedCount;
    }

    if (shoulderButtonsPressedCount > 1) // Any 2 shoulder button combo = chanting
    {
        return eChant;
    }

    DWORD rawInput = 0;
    if (cmd & eDPadUp)
    {
        rawInput |= eUp;
    }

    if (cmd & eDPadRight)
    {
        rawInput |= eRight;
    }

    if (cmd & eDPadDown)
    {
        rawInput |= eDown;
    }

    if (cmd & eDPadLeft)
    {
        rawInput |= eLeft;
    }

    if (cmd & eR1)
    {
        rawInput |= eRun;
    }

    if (cmd & eR2)
    {
        rawInput |= eSneak;
    }

    if (cmd & eL1)
    {
        if (cmd & eTriangle)
        {
            rawInput |= eHello;
        }

        if (cmd & eCircle)
        {
            rawInput |= eWork;
        }

        if (cmd & eCross)
        {
            rawInput |= eWait;
        }

        if (cmd & eSquare)
        {
            rawInput |= eFollowMe;
        }
    }
    else if (cmd & eL2)
    {
        if (cmd & eTriangle)
        {
            rawInput |= eAllYa;
        }

        if (cmd & eCircle)
        {
            rawInput |= eSorry;
        }

        if (cmd & eCross)
        {
            rawInput |= eAnger;
        }

        if (cmd & eSquare)
        {
            rawInput |= eStopIt;
        }
    }
    else // No shoulder buttons?
    {
        if (cmd & eTriangle)
        {
            rawInput |= eHop;
        }

        if (cmd & eCircle)
        {
            rawInput |= eThrowItem;
        }

        if (cmd & eCross)
        {
            rawInput |= eFart;
        }

        if (cmd & eSquare)
        {
            rawInput |= eDoAction;
        }
    }

    return rawInput;
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
    
    /*
    if (gnFrame_dword_5C1B84 == 8000)
    {
        // This will crash if called from main menu, has to be "in game"
        // this function doesn't seem to change abes position which can be bad
        MapChange_4047E1(&gLevelObject_dword_5C3030, 
            0,  // edx
            1,  // level
            1,  // path
            4, // camera
            0, // ??
            0, // ??
            1); // Change even if already there 
    }*/

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
            if (command & eEnd)
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
    else if (sFakeInputEnabled)
    {
        pThis->field_0_pads[0].field_0_pressed = sFakeInputValue;// Input_Command_Convert_404354(sFakeInputValue);
    }

    if (sRecording)
    {
        UpdateRecording(*pThis);
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


struct gVtbl_animation_2a_544290
{
    // Note: These actually take AnimationEx*'s but since this is hand made vtable it has to use the base type
    void(__thiscall* jAnimation_decode_frame_4039D1)(Animation2 *pthis);
    char(__thiscall* Animation_403F7B)(Animation2 *pthis, int a2, int a3, int a4, __int16 a5, int a6);
    signed __int16(__thiscall* Animation_4029E1)(Animation2 *a1);
    __int16(__thiscall* Animation_4032BF)(Animation2 *a1, int a2);
    char(__thiscall* Animation_4030FD)(Animation2 *pthis, int a1, int a2, int a3, int a4, int a5);
};

static void BeforeRender()
{

}

extern bool gForceDemo;
void HandleDemoLoad();

void __cdecl j_AnimateAllAnimations_40AC20_Hook(GameObjectList::Objs<Animation2*>* pAnims)
{
    if (gForceDemo)
    {
        HandleDemoLoad();
    }

    for (u16 i = 0; i < pAnims->mCount; i++)
    {
        Animation2* pAnim = pAnims->mArray[i];
        if (!pAnim)
        {
            break;
        }

        if (pAnim->field_4_flags & 2)
        {
            if (pAnim->field_E_frame_change_counter > 0)
            {
                pAnim->field_E_frame_change_counter--;
                if (pAnim->field_E_frame_change_counter == 0)
                {
                    gVtbl_animation_2a_544290* pVTbl = reinterpret_cast<gVtbl_animation_2a_544290*>(pAnim->field_0_VTable);
                    pVTbl->jAnimation_decode_frame_4039D1(pAnim);
                }
            }
        }
    }
    BeforeRender();
}
ALIVE_FUNC_IMPLEX(0x0, 0x40AC20, j_AnimateAllAnimations_40AC20_Hook, true);


int __cdecl AbeSnap_sub_449930_hook(int scale, const signed int xpos)
{
    int result = 0;
    int v3 = 0;
    int v4 = 0;

    if (scale == 32768)
    {
        v4 = (xpos % 375 - 6) % 13;
        if (v4 >= 7)
            result = xpos - v4 + 13;
        else
            result = xpos - v4;
    }
    else
    {
        if (scale == 65536)
        {
            v3 = (xpos - 12) % 25;
            if (v3 >= 13)
                result = xpos - v3 + 25;
            else
                result = xpos - v3;

            LOG_INFO("SNAP: " << xpos << " to " << result);
        }
        else
        {
            result = xpos;
        }
    }
    return result;
}
ALIVE_FUNC_IMPLEX(0x0, 0x00449930, AbeSnap_sub_449930_hook, true);


ALIVE_VAR(0x0, 0x5D1E10, BYTE, gRandomIndex_dword_5D1E10);
const BYTE gRandomTable_546744[256]
{
    53, 133,  73,  226, 167,  66, 223,  11,  45,  35,
    221, 222,  31,  23, 187, 207,  78, 163,  25,   4,
    113,  18, 181,  80,  67, 100, 160,  21, 219,  34,
    176, 131,  57, 234, 175, 195, 208, 206, 119,  20,
    173,  86, 128,  95, 110, 210, 217, 192, 230, 246,
    112, 249,   5,  90,  51, 197, 140, 115, 203, 250,
    129,  62, 216, 158,  38, 214,  12, 186, 170, 205,
    126, 157, 255,  29,   6, 196, 237, 242, 244,  91,
    148, 155, 161,  94, 184,  55, 193, 241,  87, 123,
    215, 251,  37, 204, 145, 240,  98, 127, 252,  26,
    150, 114,  47, 218,  56, 162,  58, 191, 180, 177,
    232, 189,  15, 247, 174, 166, 136, 116,  44, 125,
    1,   236,   7,  36,  64,  52,  93,  89, 156, 122,
    154, 238, 231,  70, 159,  97,  99,  48, 178, 151,
    239, 172, 118, 142, 117, 228, 211, 169,  42,  65,
    0,   165, 188, 102,  81, 202,  27, 183, 124,  14,
    24,  107, 199, 120, 132, 106, 108, 130,  96, 213,
    28,   19,  85,  82, 185,  83,  50,  30, 182,  40,
    75,  143,  17, 141, 139, 253,  16, 103,  63, 209,
    54,   69, 134, 201,  74,  84,  79, 248, 121,  41,
    105,   8, 233, 137,  32, 171, 109, 227, 198, 152,
    153, 229, 147,  72,   9, 225, 243,  71,  76, 254,
    138, 149,  60, 235,  43,   3, 245, 168,  88,  61,
    194,  49, 101, 220,  39, 190,  33, 104, 224, 179,
    200, 164,   2,  46, 212,  59, 111,  92, 135,  10,
    146,  13,  77,  22,  68, 144
};

signed __int16 __cdecl RangedRandom_496AB0(signed __int16 from, signed __int16 to)
{
    if (to < from)
    {
        std::swap(to, from);
    }
    else if (to == from)
    {
        return from;
    }

    if (to - from < 256)
    {
        return (gRandomTable_546744[gRandomIndex_dword_5D1E10++] % (to - from + 1)) + from;
    }
    else
    {
        const signed __int16 tableValue = 257 * gRandomTable_546744[gRandomIndex_dword_5D1E10];
        gRandomIndex_dword_5D1E10 += 2;
        return (tableValue % (to - from + 1)) + from;
    }
}
ALIVE_FUNC_IMPLEX(0x0, 0x496AB0, RangedRandom_496AB0, true); // TODO: This function isn't yet tested and might be wrong

struct FakeVTable { void* mFuncs[1]; };

ALIVE_VAR(0x0, 0x54690C, FakeVTable*, gDemoVtbl_type_98_54690C);
ALIVE_VAR(0x0, 0x5C1BA0, WORD, word_5C1BA0);
ALIVE_VAR(0x0, 0x5D1E20, BaseGameObject*, gDemoObject_dword_5D1E20);

ALIVE_FUNC_NOT_IMPL(0x0, 0x4024AA, void* __cdecl(size_t), Allocate_4024AA);
ALIVE_FUNC_NOT_IMPL(0x0, 0x4DBFA0, BaseGameObject* __fastcall(BaseGameObject* pThis, void* edx, __int16 bFlag, int bansArraySize), j_BaseGameObject_ctor_4DBFA0);
ALIVE_FUNC_NOT_IMPL(0x0, 0x403AB7, DWORD** __fastcall(BaseGameObject* pThis, void* edx, unsigned int type, int id), jGetLoadedResource_403AB7);

ALIVE_FUNC_NOT_IMPL(0x0, 0x497880, BaseGameObject* __fastcall (BaseGameObject* pThis, void* edx, char bFree), Demo_V1_497880);

void Demo_Reset()
{
    if (gDemoObject_dword_5D1E20)
    {

        InputObject* gInput_dword_5BD4E0 = reinterpret_cast<InputObject*>(0x5BD4E0);
        
        static DWORD* ptr = (DWORD*)gDemoData.mJoyData.data();
        DWORD** pDemoResourceBlock = &ptr;
        Input_SetDemo_45F1E0(gInput_dword_5BD4E0, 0, pDemoResourceBlock);
        word_5C1BA0 = 1;
    }
}

void __cdecl Demo_ctor_type_98_4D6990(int /*a1*/, int /*a2*/, int /*a3*/, __int16 loadType)
{
    if (loadType == 99) // hack so only gets loaded when we want it to be
//    if (loadType != 1 && loadType != 2)
    {
        if (word_5C1BA0)
        {
            if (!gDemoObject_dword_5D1E20)
            {
                BaseGameObject* pDemoObject = (BaseGameObject *)Allocate_4024AA(0x20u);
                if (pDemoObject)
                {
                    j_BaseGameObject_ctor_4DBFA0(pDemoObject, 0, 1, 0);
                    pDemoObject->mVTbl = &gDemoVtbl_type_98_54690C;
                    if (gDemoObject_dword_5D1E20)
                    {
                        pDemoObject->field_6_flags.mParts.mLo |= 4u;
                    }
                    else
                    {
                        gDemoObject_dword_5D1E20 = pDemoObject;
                        pDemoObject->field_6_flags.mParts.mHi |= 1u;
                        pDemoObject->field_4_typeId = 98;

                     
                        DWORD** pDemoResourceBlock = nullptr;
                        if (gDemoData.mJoyData.empty())
                        {   
                            // This Demo resource is added as a chunk in the Bits/CAM file. The .JOY files
                            // are a Red Herring!
                            pDemoResourceBlock = jGetLoadedResource_403AB7(pDemoObject, 0, 'omeD', 1);
                        }
                        else
                        {
                            // Override with external demo file
                            static DWORD* ptr = (DWORD*)gDemoData.mJoyData.data();
                            pDemoResourceBlock = &ptr;
                        }

                        pDemoObject->field_1C_update_delay = 0; // HACK: Real game has it as 1

                        InputObject* gInput_dword_5BD4E0 = reinterpret_cast<InputObject*>(0x5BD4E0);
                        Input_SetDemo_45F1E0(gInput_dword_5BD4E0, 0, pDemoResourceBlock);
                    }
                }
            }
        }
    }
}
ALIVE_FUNC_IMPLEX(0x0, 0x4D6990, Demo_ctor_type_98_4D6990, true);
