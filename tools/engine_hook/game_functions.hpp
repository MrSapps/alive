#pragma once

#include <dsound.h>
#include <type_traits>

extern bool gIsAe;

template<class AddressType>
struct Address
{
    DWORD mAe;
    DWORD mAo;
    AddressType Get()
    {
        return *reinterpret_cast<AddressType*>(mAe);
    }

    void Set(AddressType value)
    {
        *reinterpret_cast<AddressType*>(mAe) = value;
    }
};

template<class AddressType>
struct AddressFunction
{
    DWORD mAe;
    DWORD mAo;

    DWORD Address() const
    {
        return gIsAe ? mAe : mAo;
    }

    template<typename... Params>
    auto operator()(Params&&... args)
    {
        auto typedFuncPtr = reinterpret_cast<AddressType*>(mAe);
        return typedFuncPtr(std::forward<Params>(args)...);
    }

    using Type = AddressType;
};

struct GameFunctions
{
    AddressFunction<HWND __cdecl()> GetWindowHandle = { 0x004F2C70, 0x0 };
    AddressFunction<int __cdecl(const char* sourceFile, int errorCode, int notUsed, const char* message)> error_msgbox = { 0x004F2920, 0x0 };
    AddressFunction<IDirectSoundBuffer*__cdecl(int soundIndex, int a2)> sub_4EF970 = { 0x004EF970, 0x0 };

    AddressFunction<int(__cdecl)(DWORD* hdc)> gdi_draw = { 0x004F21F0, 0x0 };
    AddressFunction<HDC(__cdecl)(DWORD* hdc)> ConvertAbeHdcHandle = { 0x4F2150, 0x0 };

    AddressFunction<DWORD(__cdecl)(DWORD* hdc, int hdc2)> ConvertAbeHdcHandle2 = { 0x4F21A0, 0x0 };
};

struct GameVars
{
    Address<IDirectSound8*> g_lPDSound_dword_BBC344 = { 0x00BBC344, 0x0 };
    Address<DWORD> dword_575158 = { 0x575158 , 0x0 };
    Address<DWORD> g_snd_time_dword_BBC33C = { 0xBBC33C , 0x0 };
    Address<DWORD*> dword_BBBD38 = { 0xBBBD38 , 0x0 };

    Address<DWORD> ddCheatOn = { 0x005CA4B5 , 0x0 };
    Address<DWORD> alwaysDrawDebugText = { 0x005BC000 , 0x0 };

    Address<struct PathRootData*> gPathData = { 0x00559660, 0x0 };

    Address<char> currentLevelId = { 0x5C3030, 0x0 };
    Address<char> currentPath = { 0x5C3032, 0x0 };
    Address<char> currentCam = { 0x5C3034, 0x0 };
};


extern GameFunctions gFuncs;
extern GameVars gVars;
