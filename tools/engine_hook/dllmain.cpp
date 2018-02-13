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
#include "demo_hooks.hpp"

#define private public
#include "gridmap.hpp"
#undef private

/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_Primary;
/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_BackBuffer;
/*static*/ DirectSurface7Proxy* DirectSurface7Proxy::g_FakePrimary;


HINSTANCE gDllInstance = NULL;

static int __cdecl gdi_draw_hook(DWORD * hdc);

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
    BYTE** mAnimChunkPtrs;
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
std::string gStartDemoPath;

int __fastcall set_first_camera_hook(void *thisPtr, void*, __int16 levelNumber, __int16 pathNumber, __int16 cameraNumber, __int16 screenChangeEffect, __int16 a6, __int16 a7);
ALIVE_FUNC_IMPLEX(0x443EE0, 0x00401415, set_first_camera_hook, true);

void __cdecl GameLoop_467230();
ALIVE_FUNC_IMPLEX(0x0, 0x00467230, GameLoop_467230, true);


ALIVE_FUNC_NOT_IMPL(0x0, 0x403274, void** __cdecl(const char *pBanName, int a2), LoadResource_403274);
ALIVE_FUNC_NOT_IMPL(0x0, 0x401AC8, void** __cdecl(unsigned int type, int id, __int16 a3, __int16 a4) , jGetLoadedResource_401AC8);
ALIVE_FUNC_NOT_IMPL(0x0, 0x4014AB, signed __int16 __cdecl(void* a1), sub_4014AB);
ALIVE_FUNC_NOT_IMPL(0x0, 0x4C9170, signed __int16 __cdecl(void* a1), j_LoadOrCreateSave_4C9170);

ALIVE_VAR_EXTERN(WORD, word_5C1BA0);


static bool sbCreateDemo = false;
bool gForceDemo = false;
__int16 __cdecl sub_40390E(int a1);
ALIVE_FUNC_IMPLEX(0x0, 0x40390E, sub_40390E, true);
__int16 __cdecl sub_40390E(int a1)
{
    auto ret = sub_40390E_.Ptr()(a1);

    if (sbCreateDemo)
    {
        Demo_Reset(); // Kill any existing demo
        Demo_ctor_type_98_4D6990(0, 0, 0, 99);
        sbCreateDemo = false;
    }

    return ret;
}

void HandleDemoLoad()
{
    // Check for external SAV/JOY pair
    bool loadedExternalDemo = false;
    try
    {
        Oddlib::FileStream savFile(gStartDemoPath + ".SAV", Oddlib::IStream::ReadMode::ReadOnly);
        Oddlib::FileStream joyFile(gStartDemoPath + ".JOY", Oddlib::IStream::ReadMode::ReadOnly);

        gDemoData.mSavData = Oddlib::IStream::ReadAll(savFile);
        gDemoData.mJoyData = Oddlib::IStream::ReadAll(joyFile);

        // Erase resource headers
        DWORD* savData = reinterpret_cast<DWORD*>(gDemoData.mSavData.data());
        // Check there is a res header in the save as user created demo saves don't have it
        if (savData[2] == 'PtxN')
        {
            gDemoData.mSavData.erase(gDemoData.mSavData.begin(), gDemoData.mSavData.begin() + 0x10);
        }

        gDemoData.mJoyData.erase(gDemoData.mJoyData.begin(), gDemoData.mJoyData.begin() + 0x10);

        // For reversing purposes also allow omitting of the Demo header
        DWORD* joyData = reinterpret_cast<DWORD*>(gDemoData.mJoyData.data());
        if (joyData[2] == 'omeD')
        {
            gDemoData.mSavData.erase(gDemoData.mSavData.begin(), gDemoData.mSavData.begin() + 0x10);
        }

        loadedExternalDemo = true;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("Failed to load external demo: " << ex.what() << " falling back to internal demo");
    }

    gForceDemo = false;

    word_5C1BA0 = 1; // Demo ctor will do nothing if this is not set
    BYTE* gSaveBuffer_unk_BAF7F8 = reinterpret_cast<BYTE*>(0xBAF7F8);

    if (!loadedExternalDemo)
    {
        if (!gStartDemoPath.empty())
        {
            LoadResource_403274(gStartDemoPath.c_str(), 0); // Will crash here if .SAV is not in the .LVL
            void** pLoadedSaveBlock = jGetLoadedResource_401AC8('PtxN', 0, 1, 0);

            memcpy(gSaveBuffer_unk_BAF7F8, *pLoadedSaveBlock, 8192u);
            sub_4014AB(pLoadedSaveBlock); // Frees block ?

            j_LoadOrCreateSave_4C9170(pLoadedSaveBlock);

            gDemoData.mJoyData.clear(); // Make sure demo ctor won't try to use it

            sbCreateDemo = true;
        }
    }
    else
    {
        memcpy(gSaveBuffer_unk_BAF7F8, gDemoData.mSavData.data(), 8192u);
        j_LoadOrCreateSave_4C9170(gDemoData.mSavData.data());

        // If the demo was created for a "real" map there will not be a demo spawn
        // point in it. Therefore create the demo manually.
        sbCreateDemo = true;

    }
   
    LOG_INFO("Demo booted");
}

void __cdecl GameLoop_467230()
{
    static bool firstCall = true;
    if (firstCall)
    {
        if (gStartMode == StartDialog::eStartBootToDemo )
        {
            HandleDemoLoad();
        }
        firstCall = false;
    }
    GameLoop_467230_.Ptr()();
}

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

    case StartDialog::eStartBootToDemo:
        LOG_INFO("Boot to demo will be invoked later");
        cameraNumber = 1; // Go directly to main menu
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

    u16 mAbeStartXPos; // values offset for ao?
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
                        pathDataStream,
                        pCollisionInfo->mCollisionOffset + 16,
                        pathData.object_indextable_offset + 16,
                        pathData.object_offset + 16,
                        pathData.mBTop / pathData.mWidth,
                        pathData.mBBottom / pathData.mHeight,
                        -1, -1,
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

                    const auto mapping = GetAnimLogger().LoadPath(resName);

                    gPath = std::make_unique<Oddlib::Path>(
                        pathDataStream,
                        mapping->mCollisionOffset,
                        mapping->mIndexTableOffset,
                        mapping->mObjectOffset,
                        mapping->mNumberOfScreensX,
                        mapping->mNumberOfScreensY,
                        mapping->mSpawnXPos,
                        mapping->mSpawnYPos,
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

class EditablePathsJson : public PathsJson
{
public:
    using PathsJson::PathsJson;
    using PathsJson::mPathMaps;
    using PathsJson::mPathThemes;
};

struct PerLvlData
{
    const char* field_0_display_name;
    WORD field_4_level;
    WORD field_6_path;
    WORD field_8_camera;
    WORD field_A_scale;
    WORD field_C_abe_x_off;
    WORD field_E_abe_y_off;
};

const static PerLvlData gMovieMenuInfos_561540[28] =
{
    { "GT Logo", 0, 65535, 65535, 3u, 65535, 65535 },
    { "Oddworld Intro", 0, 65535, 65535, 1u, 65535, 65535 },
    { "Abe's Exoddus", 0, 65535, 65535, 5u, 65535, 65535 },
    { "Backstory", 0, 65535, 65535, 4u, 65535, 65535 },
    { "Prophecy", 1, 65535, 65535, 1u, 65535, 65535 },
    { "Vision", 1, 65535, 65535, 24u, 65535, 65535 },
    { "Game Opening", 1, 65535, 65535, 2u, 65535, 65535 },
    { "Brew", 1, 65535, 65535, 26u, 65535, 65535 },
    { "Brew Transition", 1, 65535, 65535, 31u, 65535, 65535 },
    { "Escape", 1, 65535, 65535, 25u, 65535, 65535 },
    { "Reward", 2, 65535, 65535, 22u, 65535, 65535 },
    { "FeeCo", 5, 65535, 65535, 4u, 65535, 65535 },
    { "Information Booth", 5, 65535, 65535, 3u, 65535, 65535 },
    { "Train 1", 6, 65535, 65535, 5u, 65535, 65535 },
    { "Train 2", 9, 65535, 65535, 15u, 65535, 65535 },
    { "Train 3", 8, 65535, 65535, 6u, 65535, 65535 },
    { "Aslik Info", 5, 65535, 65535, 2u, 65535, 65535 },
    { "Aslik Explodes", 5, 65535, 65535, 1u, 65535, 65535 },
    { "Dripek Info", 6, 65535, 65535, 4u, 65535, 65535 },
    { "Dripek Explodes", 6, 65535, 65535, 3u, 65535, 65535 },
    { "Phleg Info", 8, 65535, 65535, 4u, 65535, 65535 },
    { "Phleg Explodes", 8, 65535, 65535, 5u, 65535, 65535 },
    { "Soulstorm Info", 9, 65535, 65535, 14u, 65535, 65535 },
    { "Ingredient", 9, 65535, 65535, 16u, 65535, 65535 },
    { "Conference", 9, 65535, 65535, 13u, 65535, 65535 },
    { "Happy Ending", 9, 65535, 65535, 17u, 65535, 65535 },
    { "Sad Ending", 9, 65535, 65535, 18u, 65535, 65535 },
    { "Credits", 16, 65535, 65535, 65535u, 65535, 65535 }
};


const static PerLvlData gDemoData_off_5617F0[23] =
{
    { "Mudokons 1", 1, 8, 5, 0u, 0, 0 },            // MI P8
    { "Mudokons 2", 1, 8, 32, 1u, 0, 0 },           // MI P8
    { "Mudokons 3", 1, 8, 21, 2u, 0, 0 },           // MI P8
    { "Flying Slig", 1, 9, 18, 4u, 0, 0 },          // MI P9
    { "Blind Mudokons 1", 1, 11, 27, 5u, 0, 0 },    // MI P11
    { "Blind Mudokons 2", 1, 11, 22, 3u, 0, 0 },    // MI P11
    { "Minecar", 1, 12, 2, 6u, 0, 0 },              // MI P12
    { "Fleeches", 2, 7, 1, 7u, 0, 0 },              // NE P7
    { "Paramite Chase", 3, 2, 13, 8u, 0, 0 },       // PV P2
    { "Paramites Talk", 3, 6, 8, 27u, 0, 0 },       // PV P6
    { "Scrab and Fleeches", 4, 12, 2, 9u, 0, 0 },   // SV P12
    { "Invisibility", 4, 13, 5, 10u, 0, 0 },        // SV P13
    { "Farts-a-poppin'", 5, 6, 3, 12u, 0, 0 },      // FD P6
    { "Flying Sligs 2", 5, 12, 1, 14u, 0, 0 },      // FD P12
    { "Baggage Claim", 12, 13, 1, 15u, 0, 0 },      // FD ENDER P13
    { "Shrykull", 6, 3, 10, 16u, 0, 0 },            // BA P3
    { "Crawling Sligs", 6, 4, 6, 17u, 0, 0 },       // BA P4
    { "Slogs Attack", 8, 11, 7, 18u, 0, 0 },        // BW P11
    { "Glukkon", 14, 13, 9, 19u, 0, 0 },            // BW ENDER P13
    { "Angry Mudokons", 9, 13, 10, 22u, 0, 0 },     // BR P13
    { "Sligs", 9, 26, 4, 23u, 0, 0 },               // BR P26
    { "Tortured Mudokons", 9, 27, 7, 24u, 0, 0 },   // BR P27
    { "Greeters Go Boom", 9, 28, 4, 25u, 0, 0 }     // BR P28
};

// Used by the level skip cheat/ui/menu
const static PerLvlData gPerLvlData_561700[17] =
{
    { "Mines", 1, 1, 4, 65535u, 2712, 1300 },
    { "Mines Ender", 1, 6, 10, 65535u, 2935, 2525 },
    { "Necrum", 2, 2, 1, 65535u, 2885, 1388 },
    { "Mudomo Vault", 3, 1, 1, 65535u, 110, 917 },
    { "Mudomo Vault Ender", 11, 13, 1, 65535u, 437, 454 },
    { "Mudanchee Vault", 4, 6, 3, 65535u, 836, 873 },
    { "Mudanchee Vault Ender", 7, 9, 4, 65534u, 1600, 550 },
    { "FeeCo Depot", 5, 1, 1, 65535u, 4563, 972 },
    { "FeeCo Depot Ender", 12, 11, 5, 65535u, 1965, 1650 },
    { "Barracks", 6, 1, 4, 65535u, 1562, 1651 },
    { "Barracks Ender", 13, 11, 5, 65535u, 961, 1132 },
    { "Bonewerkz", 8, 1, 1, 65535u, 813, 451 },
    { "Bonewerkz Ender", 14, 14, 10, 65535u, 810, 710 },
    { "Brewery", 9, 16, 6, 65535u, 1962, 1232 },
    { "Game Ender", 10, 1, 1, 65535u, 460, 968 },
    { "Credits", 16, 1, 1, 65535u, 0, 0 },
    { "Menu", 0, 1, 1, 65535u, 0, 0 }
};

const static std::map<int, std::string> gPathThemeNames =
{
    { 0, "Menu" },
    { 1, "Mines" },
    { 2, "Necrum" },
    { 3, "ParamiteVaults" },
    { 4, "ScrabVaults" },
    { 5, "FecoDepot" },
    { 6 , "Barracks" },
    { 8 , "BoneWerkz" },
    { 9 , "Brewery" },
    { 10 , "Brewery_Ender" }, // BM

    { 7 , "ScrabVaults_Ender" },
    { 11 , "ParamiteVaults_Ender" },
    { 12 , "FecoDepot_Ender" },
    { 13 , "Barracks_Ender" },
    { 14 , "BoneWerkz_Ender" },

    { 15 , "TL" },
    { 16 , "Credits" },
};

class AePcSaveFile
{
public:
    const static u32 kObjectsOffset = 0x55C;
    const static int OffsetPATHWorldState = 0x244;
    // TODO: Other types as ripped by mlg
    /*
    2,16
    9,12
    11,60
    25,8
    26,128
    30,20
    45,60
    50,180
    54,172
    55,16
    57,16
    60,4
    61,24
    64,80
    65,60
    67,144
    69,216
    78,28
    81,136
    84,60
    89,104
    96,120
    99,8
    102,4
    104,40
    105,56
    112,160
    113,16
    122,8
    125,164
    126,120
    129,44
    136,16
    142,12
    143,24
    148,16
    */
    enum ObjectTypes : u16
    {
        eLandMine = 0x8F,
        eMud = 0x51,
        eAbe = 0x45,
        eSlamDoor = 0x7A,
        eWheel = 0x94,
        ePlatform = 0x4E, // trap door ??
        eFlyingSlig = 0x36,
        eParamite = 0x60,
        eBirdPortal = 0x63
    };

    void DeSerialize(Oddlib::IStream& stream)
    {
        stream.Seek(OffsetPATHWorldState + 4);

        mLevel = ReadU16(stream);
        mPath = ReadU16(stream);
        mCamera = ReadU16(stream);

        
        mAbeXPos = ReadU16(stream);
        mAbeXPos = ReadU16(stream);
        mAbeYPos = ReadU16(stream);
        
        /*

        const int OffsetPATHAbeState = 0x284;
        stream.Seek(0);
        stream.Seek(OffsetPATHAbeState + 2); // 2 object id, 

       // HalfFloat xpos(ReadU32(stream));
       // HalfFloat ypos(ReadU32(stream));

       // mAbeXPos = (s32)xpos.AsDouble();
       // mAbeYPos = (s32)ypos.AsDouble();
        mAbeXPos = ReadU32(stream);
        mAbeYPos = ReadU32(stream);
        */

        /*
        const u16 objectType = ReadU16(stream);
        switch (objectType)
        {
        case ObjectTypes::eLandMine:
            stream.Seek(stream.Pos() + 24);
            break;
        case ObjectTypes::eMud:
            stream.Seek(stream.Pos() + 136);
            break;
        case ObjectTypes::eAbe:
            stream.Seek(stream.Pos() + 216);
            break;
        case ObjectTypes::eSlamDoor:
            stream.Seek(stream.Pos() + 8);
            break;
        case ObjectTypes::eWheel:
            stream.Seek(stream.Pos() + 16);
            break;
        case ObjectTypes::ePlatform:
            stream.Seek(stream.Pos() + 28);
            break;
        case ObjectTypes::eFlyingSlig:
            stream.Seek(stream.Pos() + 184);
            break;
        case ObjectTypes::eParamite:
            stream.Seek(stream.Pos() + 128);
            break;
        case ObjectTypes::eBirdPortal:
            stream.Seek(stream.Pos() + 8);
            break;
        default:
            LOG_ERROR("Unknown type " << objectType);
            abort();
        }*/
    }


    s32 AbeXPos() const
    {
        return mAbeXPos;
    }

    s32 AbeYPos() const
    {
        return mAbeYPos;
    }

    int Level() const
    {
        return mLevel;
    }

    int Path() const
    {
        return mPath;
    }

    int Camera() const
    {
        return mCamera;
    }
private:
    int mLevel = 0;
    int mPath = 0;
    int mCamera = 0;

    s32 mAbeXPos = 0;
    s32 mAbeYPos = 0;
};

struct ScrapedPathData
{
    int level;
    int path;
    bool isDemo;
    bool isEnder;
};

int ToLevelId(const std::string& id, bool& valid)
{
    valid = true;
    if (id == "ST")
    {
        return 0;
    }
    else if (id == "MI")
    {
        return 1;
    }
    else if (id == "NE")
    {
        return 2;
    }
    else if (id == "PV")
    {
        return 3;
    }
    else if (id == "SV")
    {
        return 4;
    }
    else if (id == "FD")
    {
        return 5;
    }
    else if (id == "BA")
    {
        return 6;
    }
    else if (id == "BW")
    {
        return 8;
    }
    else if (id == "BR")
    {
        return 9;
    }
    else if (id == "BM")
    {
        return 10;
    }
    else if (id == "CR")
    {
        return 16;
    }

    valid = false;
    return 0;
}

static bool IsEnder(int lvl)
{
    switch (lvl)
    {
    case 11: return true; // PV
    case 7:  return true; // SV
    case 12: return true; // FD
    case 13: return true; // BA
    case 14: return true; // BW
    }
    return false;
}

static int DeEnder(int lvl)
{
    switch (lvl)
    {
    case 11: return 3; // PV
    case 7:  return 4; // SV
    case 12: return 5; // FD
    case 13: return 6; // BA
    case 14: return 8; // BW
    }
    return lvl;
}

static int ToEnder(int lvl)
{
    switch (lvl)
    {
    case 3: return 11; // PV
    case 4:  return 7; // SV
    case 5: return 12; // FD
    case 6: return 13; // BA
    case 8: return 14; // BW
    }
    return lvl;
}

struct LevelAndPathCamera
{
    int level;
    int path;
    int camera;

    int abex;
    int abey;
};

static bool operator < (const LevelAndPathCamera& lhs, const LevelAndPathCamera& rhs)
{
    return std::tie(lhs.level, lhs.path, lhs.camera) < std::tie(rhs.level, rhs.path, rhs.camera);
}

static std::map<LevelAndPathCamera, std::string> gSaveMap;

void UpdatePathsJson()
{
    const std::vector<std::string> lvls =
    { 
        { "ba.lvl" },
        { "bm.lvl" },
        { "br.lvl" },
        { "bw.lvl" },
        { "cr.lvl" },
        { "fd.lvl" },
        { "mi.lvl" },
        { "ne.lvl" },
        { "pv.lvl" },
        { "st.lvl" },
        { "sv.lvl" }
    };

    for (const auto& lvlName : lvls)
    {
        Oddlib::LvlArchive lvlArchive(lvlName);
        for (u32 i = 0; i < lvlArchive.FileCount(); i++)
        {
            const std::string fileName = lvlArchive.FileByIndex(i)->FileName();
            auto chunk = lvlArchive.FileByIndex(i)->ChunkByType(Oddlib::MakeType("NxtP"));
            if (chunk)
            {
                if (fileName.substr(0, 2) == "NX")
                {
                    auto chunkStream = chunk->Stream();
                    AePcSaveFile saveFile;
                    saveFile.DeSerialize(*chunkStream);

                    LevelAndPathCamera levelPath = { saveFile.Level(), saveFile.Path(), saveFile.Camera(), saveFile.AbeXPos(), saveFile.AbeYPos() };
                    auto it = gSaveMap.find(levelPath);
                    if (it != std::end(gSaveMap))
                    {
                        //abort();
                    }
                    gSaveMap[levelPath] = fileName;
                }
            }
        }
    }

    std::vector<ScrapedPathData> scrapedData;

    for (const auto& lvlSkip : gDemoData_off_5617F0)
    {
        scrapedData.push_back({ DeEnder(lvlSkip.field_4_level), lvlSkip.field_6_path, true, IsEnder(lvlSkip.field_4_level) });
    }

    for (const auto& lvlSkip : gPerLvlData_561700)
    {
        scrapedData.push_back({ DeEnder(lvlSkip.field_4_level), lvlSkip.field_6_path, false, IsEnder(lvlSkip.field_4_level) });
    }

    // SV enders
    scrapedData.push_back({ DeEnder(7), 9, false, true });
    scrapedData.push_back({ DeEnder(7), 10, false, true });
    scrapedData.push_back({ DeEnder(7), 11, false, true });
    scrapedData.push_back({ DeEnder(7), 14, false, true });

    // BW enders
    scrapedData.push_back({ DeEnder(8), 14, false, true });
    scrapedData.push_back({ DeEnder(8), 9, false, true });

    // PV enders
    scrapedData.push_back({ DeEnder(11), 13, false, true });

    // FD enders
    scrapedData.push_back({ DeEnder(12), 11, false, true });
    scrapedData.push_back({ DeEnder(12), 14, false, true });

    // BA enders
    scrapedData.push_back({ DeEnder(13), 11, false, true });
    scrapedData.push_back({ DeEnder(13), 16, false, true });

    // BW enders
    scrapedData.push_back({ DeEnder(14), 14, false, true });
    scrapedData.push_back({ DeEnder(14), 9, false, true });

    Oddlib::FileStream jsonFile("../../data/paths.json", Oddlib::IStream::ReadMode::ReadOnly);
    EditablePathsJson paths;
    paths.DeSerialize(jsonFile.LoadAllToString());
   // paths.Serialize("../../data/paths_new.json");

    // Create a theme for each LVL entry
    PathsJson::PathTheme dummyTheme;
    dummyTheme.mDoorSkin = "Mines";
    dummyTheme.mGlukkonSkin = "GreenSuit";
    dummyTheme.mLiftSkin = "Mines";
    dummyTheme.mMusicTheme = "Barracks";
    dummyTheme.mSlamDoorSkin = "Mines";

    // Add the themes to the map
    for (const auto& pathData : gPathThemeNames)
    {
        dummyTheme.mName = pathData.second;
        paths.mPathThemes[pathData.second] = std::make_unique<PathsJson::PathTheme>(dummyTheme);
    }

    for (auto& path : paths.mPathMaps)
    {
        const std::string id = path.first.substr(0, 2);
        bool valid = false;
        int levelId = ToLevelId(id, valid);
        if (!valid)
        {
            LOG_WARNING("SKIP: " << id);
            continue;
        }

        const ScrapedPathData* pSd = nullptr;
        for (const auto& sd : scrapedData)
        {
            if (sd.level == levelId && sd.path == (int)path.second.mId)
            {
                if (sd.isEnder)
                {
                    levelId = DeEnder(sd.level);
                }
                pSd = &sd;
                break;
            }
        }

        // Find spawn offset constant
        const PerLvlData* pOffSet = nullptr;
        for (const auto& lvlSkip : gPerLvlData_561700)
        {
            // Look for an exact match first
            if (lvlSkip.field_4_level == levelId && lvlSkip.field_6_path == (int)path.second.mId)
            {
                pOffSet = &lvlSkip;
                break;
            }

            if (lvlSkip.field_4_level == levelId)
            {
                pOffSet = &lvlSkip;
                break;
            }
        }

        if (!pOffSet)
        {
            abort();
        }

        // Find per path spawn offset
        PathRootData* ptr = Vars().gPathData.Get();
        PathRoot& data = ptr->iLvls[(pSd && pSd->isEnder) ? ToEnder(levelId) : levelId];
        PathBlyRec& bly = data.mBlyArrayPtr->iBlyRecs[path.second.mId];

        const LevelAndPathCamera* pSavData = nullptr;
        for (const auto& savRec : gSaveMap)
        {
            if (savRec.first.level == levelId || (savRec.first.level == (pSd && pSd->isEnder) ? ToEnder(levelId) : levelId))
            {
                if ((u32)savRec.first.path == path.second.mId)
                {
                    pSavData = &savRec.first;
                    break;
                }
            }
        }
        
        if (pSavData)
        {
            path.second.mSpawnXPos = pSavData->abex;
            path.second.mSpawnYPos = pSavData->abey;
        }
        else
        {
            path.second.mSpawnXPos = pOffSet->field_C_abe_x_off - bly.mPathData->mAbeStartXPos;
            path.second.mSpawnYPos = pOffSet->field_E_abe_y_off - bly.mPathData->mAbeStartYPos;
        }
        auto themeIt = gPathThemeNames.find((pSd && pSd->isEnder) ? ToEnder(levelId) : levelId);
        auto themePtrIt = paths.mPathThemes.find(themeIt->second);
        //if (themePtrIt != std::end(paths.mPathThemes))
        {
            path.second.mTheme = themePtrIt->second.get();
        }
    }

    /*
    for (s32 i = 0; i < NumLevels(); i++)
    {
        PathRootData* ptr = Vars().gPathData.Get();
        PathRoot& data = ptr->iLvls[i];

        if (std::string(data.mName) == "TL")
        {
            continue;
        }

        //Oddlib::LvlArchive lvl(std::string(data.mName) + ".LVL");
        
        for (s32 j = 1; j < data.mNumPaths + 1; j++)
        {
            // Find matching PathsJson entry for this Bly
            std::string generatedName = std::string(data.mName) + "PATH_" + std::to_string(j);  // e.g BAPATH_1
            auto pathIt = paths.mPathMaps.find(generatedName);
            if (pathIt == std::end(paths.mPathMaps))
            {
                LOG_INFO("MISSING PATH: " << generatedName);
                // These missing paths seem to have been removed from the real game, for instance
                // BAPATH_6 is in the table but there is no resource for it..

                continue;
            }

            pathIt->second.mSpawnXPos = data.mBlyArrayPtr->iBlyRecs[j].mPathData->mAbeStartXPos;
            pathIt->second.mSpawnYPos = data.mBlyArrayPtr->iBlyRecs[j].mPathData->mAbeStartYPos;

            auto themeIt = gPathThemeNames.find(i);
            auto themePtrIt = paths.mPathThemes.find(themeIt->second);
            if (themePtrIt != std::end(paths.mPathThemes))
            {
                pathIt->second.mTheme = themePtrIt->second.get();
            }

        }
    }
    */

    paths.Serialize("../../data/paths_new.json");
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
    //UpdatePathsJson();
    //abort();
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


static int __cdecl gdi_draw_hook(DWORD * hdcPtr)
{
    HDC hdc = Funcs().ConvertAbeHdcHandle(hdcPtr);
    GdiLoop(hdc);
    Funcs().ConvertAbeHdcHandle2(hdcPtr, (int)hdc);
    
    return Hooks::gdi_draw.Real()(hdcPtr);
}

HMODULE gDllHandle = NULL;

void HookMain()
{
    TRACE_ENTRYEXIT;

    DemoHooksForceLink();

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

        //Vars().gb_ddNoSkip_5CA4D1.Set(1);
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
        gStartDemoPath = startDlg.DemoPath();
    }

    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        gDllInstance = hModule;
    }
    return TRUE;
}
