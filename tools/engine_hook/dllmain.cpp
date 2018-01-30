#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <vector>
#include "ddraw7proxy.hpp"
#include "detours.h"
#include "logger.hpp"
#include "types.hpp"
#include "oddlib/anim.hpp"
#include "oddlib/stream.hpp"
#include "sound_hooks.hpp"
#include "gamefilesystem.hpp"
#include "hook.hpp"
#include "types.hpp"
#include "hook_utils.hpp"
#include "window_hooks.hpp"
#include "game_functions.hpp"
#include "anim_logger.hpp"
#include "start_dialog.hpp"
#include "resource.h"
#include "dsound7proxy.hpp"
#include "debug_dialog.hpp"
#include "addresses.hpp"
#include "game_objects.hpp"

#define private public
#include "gridmap.hpp"
#undef private

/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_Primary;
/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_BackBuffer;
/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_FakePrimary;


HINSTANCE gDllInstance = NULL;

static int __cdecl gdi_draw_hook(DWORD * hdc);

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


#pragma pack(push)
#pragma pack(1)
struct anim_struct
{
    void* mVtbl;
    WORD field_4; // max w?
    WORD field_6; // max h?
    DWORD mFlags;
    WORD field_C;
    WORD field_E;
    WORD field_10;
    WORD field_12;
    DWORD field_14;
    DWORD mAnimationHeaderOffset; // offset to frame table from anim data header
    DWORD field_1C;
    BYTE** mAnimChunkPtrs; // pointer to a pointer which points to anim data?
    DWORD iDbufPtr;
    DWORD iAnimSize;
    DWORD field_2C;
    DWORD field_30;
    DWORD field_34;
    DWORD field_38;
    DWORD field_3C;
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
    WORD iRect;
    WORD field_86;
    WORD field_88;
    WORD field_8A;
    DWORD field_8C;
    WORD field_90;
    s16 mFrameNum;
};
#pragma pack(pop)
static_assert(sizeof(anim_struct) == 0x94, "Wrong size");

void __fastcall anim_decode_hook(anim_struct* thisPtr, void*);
typedef void (__thiscall* anim_decode_thiscall)(anim_struct* thisPtr);

int __fastcall get_anim_frame_hook(anim_struct* thisPtr, int a2, __int16 a3);

struct CollisionInfo;
typedef int(__thiscall* sub_418930_thiscall)(void* thisPtr, const CollisionInfo* pCollisionInfo, u8* pPathBlock);
static int __fastcall sub_418930_hook(void* thisPtr, void*, const CollisionInfo* pCollisionInfo, u8* pPathBlock);


namespace Hooks
{
    Hook<decltype(&::sub_418930_hook), sub_418930_thiscall> sub_418930(Addrs().sub_418930());
    Hook<decltype(&::gdi_draw_hook)> gdi_draw(Funcs().gdi_draw.mAddr);
    Hook<decltype(&::anim_decode_hook), anim_decode_thiscall> anim_decode(Addrs().anim_decode());
    Hook<decltype(&::get_anim_frame_hook)> get_anim_frame(Addrs().get_anim_frame());
}

static StartDialog::StartMode gStartMode = StartDialog::eNormal;

int __fastcall set_first_camera_hook(void *thisPtr, void*, __int16 levelNumber, __int16 pathNumber, __int16 cameraNumber, __int16 screenChangeEffect, __int16 a6, __int16 a7);
ALIVE_FUNC_IMPLEX(0x443EE0, 0x00401415, set_first_camera_hook, true);

int __fastcall set_first_camera_hook(void *thisPtr, void* edx , __int16 levelNumber, __int16 pathNumber, __int16 cameraNumber, __int16 screenChangeEffect, __int16 a6, __int16 a7)
{
    TRACE_ENTRYEXIT;

    // AE Cheat screen, I think we have to fake cheat input or set a bool for this to work :(
    // cameraNumber = 31;

    // 5 = "flash" on
    // 4 = top to bottom
    // 8 = door effect/sound
    //screenChangeEffect = 5;

    switch (gStartMode)
    {
    case StartDialog::eNormal:
        break;
    case StartDialog::eStartFeeco:
        // Setting to Feco lets us go directly in game, with the side effect that pausing will crash
        // and some other nasties, still its good enough for debugging animations
        if (Utils::IsAe()) // This will just instantly crash for AO
        {
            levelNumber = 5;
            cameraNumber = 1;
        }
        break;
    case StartDialog::eStartMenuDirect:
        // Abe "hello" screen when levelNumber is left as the intro level
        cameraNumber = 1;
        break;
    }

    return set_first_camera_hook_.Ptr()(thisPtr, edx, levelNumber, pathNumber, cameraNumber, screenChangeEffect, a6, a7);
}

void DumpDeltas(anim_struct* thisPtr)
{
    static HalfFloat prevX = 0;
    static HalfFloat prevY = 0;
    static HalfFloat preVX = 0;
    static HalfFloat preVY = 0;

    BaseAnimatedWithPhysicsGameObject* pAbe = GameObjectList::HeroPtr();
    if (!pAbe) return;

    if (prevX != pAbe->xpos() ||
        prevY != pAbe->ypos() ||
        preVX != pAbe->velocity_x() ||
        preVY != pAbe->velocity_y())
    {

        printf("XD:%f YD:%f F:%d XV:%f YV:%f\n",
            (pAbe->xpos() - prevX).AsDouble(),
            (pAbe->ypos() - prevY).AsDouble(),
            thisPtr->mFrameNum,
            (pAbe->velocity_x() - preVX).AsDouble(),
            (pAbe->velocity_y() - preVY).AsDouble()
        );
        //system("PAUSE");
    }

    prevX = pAbe->xpos();
    prevY = pAbe->ypos();
    preVX = pAbe->velocity_x();
    preVY = pAbe->velocity_y();
}

void __fastcall anim_decode_hook(anim_struct* thisPtr, void*)
{
    static anim_struct* pTarget = nullptr;

    DumpDeltas(thisPtr);

    if (thisPtr->mAnimChunkPtrs)
    {

        DWORD* ptr = (DWORD*)*thisPtr->mAnimChunkPtrs;
        DWORD* id = ptr - 1;

        // Comment out pTarget checks to log anims for everything that comes in here
        // with pTarget its limited to abe anims only

        // 303 = dust
        //if (*id == 55 && pTarget == nullptr)
        {
            //pTarget = thisPtr;
        }

        // 55 index 5 = idle stand or walk
        //if (pTarget && thisPtr == pTarget)
        {
            // Force anim index 0
            // thisPtr->mFrameTableOffset = *(ptr + 1);

            // Get anim from cache or add to cache
            Oddlib::AnimSerializer* as = GetAnimLogger().AddAnim(
                *id, 
                reinterpret_cast<u8*>(ptr),
                *(ptr - 4) - (sizeof(DWORD) * 4));
           
            // Find out which index this anim is
            bool found = false;
            u32 idx = 0;
            for (const auto& anim : as->Animations())
            {
                if (anim->mOffset == thisPtr->mAnimationHeaderOffset)
                {
                    found = true;
                    break;
                }
                idx++;
            }

            if (found)
            {
                // Log the animation id/index against the parsed/cache animation
                GetAnimLogger().Add(
                    *id,
                    idx,
                    as);
            }
        }

    }
  
    Hooks::anim_decode.Real()(thisPtr);
}

int __fastcall get_anim_frame_hook(anim_struct* thisPtr, int a2, __int16 a3)
{
    // loop flag?
    // a3 = 0;
    int ret = Hooks::get_anim_frame.Real()(thisPtr, a2, a3);
    return ret;
}

struct SoundBlockInfo
{
    char* mVabHeaderName;
    char* mVabBodyName;
    unsigned int mVabID;
    void* mVabHead;
};

struct PathRoot
{
    struct PathBlyRecHeader* mBlyArrayPtr;
    void* mFmvArrayPtr;
    SoundBlockInfo* mMusicPtr;
    char* mBsqFileNamePtr; // E.g "FESEQ.BSQ"

    unsigned short int mReverb;
    unsigned short int mBgMusicId; // BG Music seq?
    char* mName; // E.g "FD"

    unsigned short int mNumPaths;
    unsigned short int mUnused;

    unsigned int mUnknown3;
    char* mLvlName; // E.g "\\FD.LVL;1" 
    unsigned int mUnknown4;
    char* mOvlName; // E.g "\\FD.OVL;1" 
    unsigned int mUnknown5;
    char* mMovName; // E.g "\\FD.MOV;1"
    char* mIdxName; // E.g "FD.IDX"
    char* mBndName; // E.g "FDPATH.BND"
};

struct PathData
{
    u16 mBLeft;	// for ao ??
    u16 mBRight; // for ao ??
    u16 mBTop;
    u16 mBBottom;

    u16 mGridWidth;
    u16 mGridHeight;

    u16 mWidth;
    u16 mHeight;

    u32 object_offset;
    u32 object_indextable_offset;

    // TODO: This isn't the full story, to get the true x,y start pos some other value is subtracted

    u16 mAbeXStartPos; // values offset for ao?
    u16 mAbeStartYPos; // part of the above for ao??

    void* (*mObjectFuncs[256])(void);
};

struct CollisionInfo;
CollisionInfo* CollisionInitFunc(const CollisionInfo *, u8 *);
using TCollisionFunc = decltype(&CollisionInitFunc);

struct CollisionInfo
{
    TCollisionFunc iFuncPtr;

    unsigned short int mBLeft;
    unsigned short int mBRight;
    unsigned short int mBTop;
    unsigned short int mBBottom;

    unsigned int mCollisionOffset;
    unsigned int mNumCollisionItems;
    unsigned int mGridWidth;
    unsigned int mGridHeight;
};

struct PathBlyRec
{
    char* mBlyName;
    PathData* mPathData;
    CollisionInfo* mCollisionData;
    unsigned int mUnused;
};

struct PathBlyRecHeader
{
    // Max number of paths is 99 due to the format string used in the cam loading
    struct PathBlyRec iBlyRecs[99];
};

const int kNumLvlsAe = 17;
const int kNumLevelsAo = 15;

inline s32 NumLevels()
{
    return Utils::IsAe() ? kNumLvlsAe : kNumLevelsAo;
}

struct PathRootData
{
    PathRoot iLvls[kNumLvlsAe];
};

std::unique_ptr<Oddlib::Path> gPath;

static int __fastcall sub_418930_hook(void* thisPtr, void*, const CollisionInfo* pCollisionInfo, u8* pPathBlock)
{
    LOG_INFO("Load collision data");

    for (s32 i = 0; i < NumLevels(); i++)
    {
        PathRoot& data = Vars().gPathData.Get()->iLvls[i];
        for (s32 j = 1; j < data.mNumPaths + 1; j++)
        {
            if (data.mBlyArrayPtr->iBlyRecs[j].mBlyName && data.mBlyArrayPtr->iBlyRecs[j].mCollisionData == pCollisionInfo)
            {
                std::cout << data.mBlyArrayPtr->iBlyRecs[j].mBlyName << std::endl;

                DWORD size = *reinterpret_cast<DWORD*>(pPathBlock - 16); // Back by the size of res header which contains the chunk size
                Oddlib::MemoryStream pathDataStream(std::vector<u8>(pPathBlock, pPathBlock + size));

                if (Utils::IsAe())
                {
                    PathData& pathData = *data.mBlyArrayPtr->iBlyRecs[j].mPathData;

                    gPath = std::make_unique<Oddlib::Path>(
                        "",
                        pathDataStream,
                        pCollisionInfo->mCollisionOffset + 16,
                        pathData.object_indextable_offset + 16,
                        pathData.object_offset + 16,
                        pathData.mBTop / pathData.mWidth,
                        pathData.mBBottom / pathData.mHeight,
                        false);
                }
                else
                {
                    // don't know where the info is in the AO data, so just look it up from the new engines
                    // resources instead
                    const std::string blyName = data.mBlyArrayPtr->iBlyRecs[j].mBlyName;

                    const std::string lvlName = blyName.substr(0, 2);
                    std::string pathNumber = blyName.substr(3);
                    pathNumber = pathNumber.substr(0, pathNumber.length() - 4);

                    const std::string resName = lvlName + "PATH_" + pathNumber;

                    const ResourceMapper::PathMapping*  mapping = GetAnimLogger().LoadPath(resName);

                    gPath = std::make_unique<Oddlib::Path>(
                        "",
                        pathDataStream,
                        mapping->mCollisionOffset,
                        mapping->mIndexTableOffset,
                        mapping->mObjectOffset,
                        mapping->mNumberOfScreensX,
                        mapping->mNumberOfScreensY,
                        false);
                }

                gDebugUi->SetActiveVab(data.mMusicPtr->mVabBodyName);

                return Hooks::sub_418930.Real()(thisPtr, pCollisionInfo, pPathBlock);
            }
        }
    }

    LOG_ERROR("Couldn't load path");
    return Hooks::sub_418930.Real()(thisPtr, pCollisionInfo, pPathBlock);
}


void GetPathArray()
{
    for (s32 i = 0; i < NumLevels(); i++)
    {
        PathRootData* ptr = Vars().gPathData.Get();
        PathRoot& data = ptr->iLvls[i];
        for (s32 j = 1; j < data.mNumPaths+1; j++)
        {
            if (data.mBlyArrayPtr->iBlyRecs[j].mBlyName && data.mBlyArrayPtr->iBlyRecs[j].mCollisionData->iFuncPtr)
            {
                std::cout << data.mBlyArrayPtr->iBlyRecs[j].mBlyName << std::endl;
            }
        }
    }
}

void GdiLoop(HDC hdc)
{
    BaseAnimatedWithPhysicsGameObject* hero = GameObjectList::HeroPtr();

    std::string text("Abe: X: " + 
        (hero ? std::to_string(hero->xpos().AsDouble()) : "??") +
        (hero ? " Y: " + std::to_string(hero->ypos().AsDouble()) : "??") + 
        (hero ? " VX: " + std::to_string(hero->velocity_x().AsDouble()) : "??") +
        (hero ? " VY: " + std::to_string(hero->velocity_y().AsDouble()) : "??") +
        " Grid: " + std::to_string(gGridEnabled) + 
        " Collisions: " + std::to_string(gCollisionsEnabled) + "(G/H)");

    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, 0);
    SetTextColor(hdc, RGB(255, 0, 0));
    TextOut(hdc, 0, 0, text.c_str(), text.length());

    XFORM xForm;
    xForm.eM11 = (FLOAT) 1.73913;
    xForm.eM12 = (FLOAT) 0.0;
    xForm.eM21 = (FLOAT) 0.0;
    xForm.eM22 = (FLOAT) 1.0;
    xForm.eDx = (FLOAT) 0.0;
    xForm.eDy = (FLOAT) (240.0 + 32.0); // Moves to background buffer

    SetGraphicsMode(hdc, GM_ADVANCED);
    //SetMapMode(hdc, MM_LOENGLISH);
    SetWorldTransform(hdc, &xForm);
    HPEN hPenOld;
    HPEN hLinePen;
    COLORREF qLineColor;
    qLineColor = RGB(255, 255, 255);
    hLinePen = CreatePen(PS_SOLID, 1, qLineColor);
    hPenOld = (HPEN)SelectObject(hdc, hLinePen);

    if (gGridEnabled)
    {
        for (int i = 0; i < (368 / 25) + 1; i++)
        {
            MoveToEx(hdc, i * 25, 0, NULL);
            LineTo(hdc, i * 25, 240);
        }

        for (int i = 0; i < (240 / 20) + 1; i++)
        {
            MoveToEx(hdc, 0, i * 20, NULL);
            LineTo(hdc, 368, i * 20);
        }
    }

    if (gCollisionsEnabled)
    {
        char * currentLevelName = Vars().gPathData.Get()->iLvls[Vars().currentLevelId.Get()].mName;
        char currentCamBuffer[24] = {};
        sprintf(currentCamBuffer, "%sP%02dC%02d.CAM", currentLevelName, Vars().currentPath.Get(), Vars().currentCam.Get());
        if (Vars().gPathData.Get())
        {
            int camX = 0;
            int camY = 0;

            for (auto x = 0u; x < gPath->XSize(); x++)
            {
                for (auto y = 0u; y < gPath->YSize(); y++)
                {
                    const Oddlib::Path::Camera screen = gPath->CameraByPosition(x, y);


                    if (screen.mName == std::string(currentCamBuffer))
                    {
                        camX = x;
                        camY = y;
                        goto camPosFound;
                    }
                }
            }
        camPosFound:

            int camRoomSizeX = Utils::IsAe() ? 375 : 1024;
            int camRoomSizeY = Utils::IsAe() ? 260 : 480;

            int renderOffsetX = camRoomSizeX * camX;
            int renderOffsetY = camRoomSizeY * camY;

            if (!Utils::IsAe())
            {
                renderOffsetX += 257;
                renderOffsetY += 114;
            }

            for (auto collision : gPath->CollisionItems())
            {
                int p1x = collision.mP1.mX - renderOffsetX;
                int p1y = collision.mP1.mY - renderOffsetY;
                int p2x = collision.mP2.mX - renderOffsetX;
                int p2y = collision.mP2.mY - renderOffsetY;

                if (p1x < 0 && p1y < 0 && p2x < 0 && p2y < 0 && p1x > 368 && p1y > 240 && p2x > 368 && p2y > 240)
                    continue;

                p1x = glm::clamp(p1x, 0, 368);
                p2x = glm::clamp(p2x, 0, 368);

                p1y = glm::clamp(p1y, 0, 240);
                p2y = glm::clamp(p2y, 0, 240);

                DeleteObject(hLinePen);

                const auto type = CollisionLine::ToType(collision.mType);
                const auto it = CollisionLine::mData.find(type);

                qLineColor = RGB(it->second.mColour.r, it->second.mColour.g, it->second.mColour.b);

                hLinePen = CreatePen(PS_SOLID, 1, qLineColor);
                hPenOld = (HPEN)SelectObject(hdc, hLinePen);

                MoveToEx(hdc, p1x, p1y, NULL);
                LineTo(hdc, p2x, p2y);
            }
        }
    }

    SelectObject(hdc, hPenOld);
    DeleteObject(hLinePen);
}

struct gVtbl_animation_2a_544290
{
    // Note: These actually take AnimationEx*'s but since this is hand made vtable it has to use the base type
    void (__thiscall* jAnimation_decode_frame_4039D1)(Animation2 *pthis);
    char (__thiscall* Animation_403F7B)(Animation2 *pthis, int a2, int a3, int a4, __int16 a5, int a6);
    signed __int16 (__thiscall* Animation_4029E1)(Animation2 *a1);
    __int16 (__thiscall* Animation_4032BF)(Animation2 *a1, int a2);
    char (__thiscall* Animation_4030FD)(Animation2 *pthis, int a1, int a2, int a3, int a4, int a5);
};

static void BeforeRender()
{

}

void __cdecl j_AnimateAllAnimations_40AC20_Hook(GameObjectList::Objs<Animation2*>* pAnims)
{
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

static int __cdecl gdi_draw_hook(DWORD * hdcPtr)
{
    HDC hdc = Funcs().ConvertAbeHdcHandle(hdcPtr);
    GdiLoop(hdc);
    Funcs().ConvertAbeHdcHandle2(hdcPtr, (int)hdc);
    
    return Hooks::gdi_draw.Real()(hdcPtr);
}

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

HMODULE gDllHandle = NULL;

void HookMain()
{
    TRACE_ENTRYEXIT;

    Hooks::SetWindowLong.Install(Hook_SetWindowLongA);
    Hooks::gdi_draw.Install(gdi_draw_hook);
    Hooks::anim_decode.Install(anim_decode_hook);
    Hooks::sub_418930.Install(sub_418930_hook);

    BaseFunction::HookAll();

    InstallSoundHooks();

    SubClassWindow();
    PatchWindowTitle();

    GetPathArray();

    Vars().ddCheatOn.Set(1);

    if (Utils::IsAe())
    {
        // Otherwise every other rendered frame is skipped which makes per frame comparison of
        // real engine vs alive rather tricky indeed.
        Vars().gb_ddNoSkip_5CA4D1.Set(1);
    }

    Vars().alwaysDrawDebugText.Set(1);
}

HRESULT __stdcall Stub_DirectSoundCreate_Hook(LPGUID lpGuid, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
std::set<class DirectSoundBuffer7Proxy*> gSoundBuffers;
class DirectSound7Proxy* gDSound;

static Hook<decltype(&Stub_DirectSoundCreate_Hook)> gStub_DirectSoundCreate_Hook(Addrs().Stub_DirectSoundCreate());

HRESULT __stdcall Stub_DirectSoundCreate_Hook(LPGUID lpGuid, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
{
    const HRESULT hr = gStub_DirectSoundCreate_Hook.Real()(lpGuid, ppDS, pUnkOuter);
    if (SUCCEEDED(hr))
    {
        auto pWrapper = new DirectSound7Proxy(**ppDS);
        *ppDS = pWrapper;
    }
    return hr;
}

int __cdecl end_Frame(char fps);
static Hook<decltype(&end_Frame)> gend_Frame_Hook(Addrs().end_Frame());

int __cdecl end_Frame(char fps)
{
    gDebugUi->OnFrameEnd();
    return gend_Frame_Hook.Real()(fps);
}

// Proxy DLL entry point
HRESULT WINAPI NewDirectDrawCreate(GUID* lpGUID, IDirectDraw** lplpDD, IUnknown* pUnkOuter)
{
    const HMODULE hDDrawDll = Utils::LoadRealDDrawDll();
    const Utils::TDirectDrawCreate pRealDirectDrawCreate = Utils::GetFunctionPointersToRealDDrawFunctions(hDDrawDll);
    const HRESULT ret = pRealDirectDrawCreate(lpGUID, lplpDD, pUnkOuter);
    if (SUCCEEDED(ret))
    {
        *lplpDD = new DirectDraw7Proxy(*lplpDD);
        
        gStub_DirectSoundCreate_Hook.Install(Stub_DirectSoundCreate_Hook);
        gend_Frame_Hook.Install(end_Frame);

        HookMain();

    }
    return ret;
}

static bool gConsoleDone = false;

extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/)
{
    gDllHandle = hModule;

    if (!gConsoleDone)
    {
        gConsoleDone = true;
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        SetConsoleTitle("ALIVE hook debug console");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);

        StartDialog startDlg;
        startDlg.Create(gDllHandle, MAKEINTRESOURCE(IDD_STARTUP), true);
        gStartMode = startDlg.GetStartMode();
    }

    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        gDllInstance = hModule;
    }
    return TRUE;
}
