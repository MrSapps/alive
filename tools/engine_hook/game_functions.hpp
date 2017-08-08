#pragma once

#include <dsound.h>
#include <type_traits>

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

    template<typename... Params>
    auto operator()(Params&&... args)
    {
        auto typedFuncPtr = reinterpret_cast<AddressType*>(mAe);
        return typedFuncPtr(std::forward<Params>(args)...);
    }
};

struct GameFunctions
{
    AddressFunction<HWND __cdecl()> GetWindowHandle = { 0x004F2C70, 0x0 };
    AddressFunction<int __cdecl(const char* sourceFile, int errorCode, int notUsed, const char* message)> error_msgbox = { 0x004F2920, 0x0 };
    AddressFunction<IDirectSoundBuffer*__cdecl(int soundIndex, int a2)> sub_4EF970 = { 0x004EF970, 0x0 };
};

struct GameVars
{
    Address<IDirectSound8*> g_lPDSound_dword_BBC344 = { 0x00BBC344, 0x0 };
    Address<DWORD> dword_575158 = { 0x575158 , 0x0 };
    Address<DWORD> g_snd_time_dword_BBC33C = { 0xBBC33C , 0x0 };
    Address<DWORD*> dword_BBBD38 = { 0xBBBD38 , 0x0 };

    Address<DWORD> ddCheatOn = { 0x005CA4B5 , 0x0 };
    Address<DWORD> alwaysDrawDebugText = { 0x005BC000 , 0x0 };
};

extern bool gIsAe;
extern GameFunctions gFuncs;
extern GameVars gVars;
