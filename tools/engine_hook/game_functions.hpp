#pragma once

#include <dsound.h>
#include <type_traits>
#include "addresses.hpp"
#include "game_objects.hpp"

template<class AddressType>
struct Address
{
    DWORD mAddr;

    AddressType Get()
    {
        return *reinterpret_cast<AddressType*>(mAddr);
    }

    void Set(AddressType value)
    {
        *reinterpret_cast<AddressType*>(mAddr) = value;
    }
};

template<class AddressType>
struct Address<AddressType*>
{
    DWORD mAddr;

    AddressType* Get()
    {
        return reinterpret_cast<AddressType*>(mAddr);
    }

    void Set(AddressType* value)
    {
        reinterpret_cast<AddressType*>(mAddr) = value;
    }
};

template<class AddressType>
struct AddressFunction
{
    DWORD mAddr;

    template<typename... Params>
    auto operator()(Params&&... args)
    {
        auto typedFuncPtr = reinterpret_cast<AddressType*>(mAddr);
        return typedFuncPtr(std::forward<Params>(args)...);
    }

    using Type = AddressType;
};

struct GameFunctions
{
    AddressFunction<HWND __cdecl()> GetWindowHandle{ Addrs().GetWindowHandle() };
    AddressFunction<int __cdecl(const char* sourceFile, int errorCode, int notUsed, const char* message)> error_msgbox = { Addrs().error_msgbox() };
    AddressFunction<IDirectSoundBuffer*__cdecl(int soundIndex, int a2)> sub_4EF970 = { Addrs().sub_4EF970() };

    AddressFunction<int(__cdecl)(DWORD* hdc)> gdi_draw = { Addrs().gdi_draw() };
    AddressFunction<HDC(__cdecl)(DWORD* hdc)> ConvertAbeHdcHandle = { Addrs().ConvertAbeHdcHandle() };

    AddressFunction<DWORD(__cdecl)(DWORD* hdc, int hdc2)> ConvertAbeHdcHandle2 = { Addrs().ConvertAbeHdcHandle2() };
    
    AddressFunction<void(__cdecl)(GameObjectList::Objs<Animation2*>* pAnims)> j_AnimateAllAnimations_40AC20 = { 0x40AC20 };

};

struct GameVars
{
    Address<IDirectSound8*> g_lPDSound_dword_BBC344 = { Addrs().g_lPDSound_dword_BBC344() };
    Address<DWORD> dword_575158 = { Addrs().dword_575158() };
    Address<DWORD> g_snd_time_dword_BBC33C = { Addrs().g_snd_time_dword_BBC33C() };
    Address<DWORD*> dword_BBBD38 = { Addrs().dword_BBBD38() };

    Address<DWORD> ddCheatOn = { Addrs().ddCheatOn() };

    // AE only
    Address<DWORD> gb_ddNoSkip_5CA4D1 = { 0x5CA4D1 };

    Address<DWORD> alwaysDrawDebugText = { Addrs().alwaysDrawDebugText() };

    Address<struct PathRootData*> gPathData = { Addrs().gPathData() };

    Address<char> currentLevelId = { Addrs().currentLevelId() };
    Address<char> currentPath = { Addrs().currentPath() };
    Address<char> currentCam = { Addrs().currentCam() };
    Address<DWORD> gnFrame = { Addrs().gnFrame() };
};

GameFunctions& Funcs();
GameVars& Vars();
