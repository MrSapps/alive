#pragma once

#include <dsound.h>
#include <type_traits>
#include "addresses.hpp"
#include "game_objects.hpp"
#include <map>
#include <sstream>
#include <iostream>
#include "detours.h"
#include "hook_utils.hpp"

#define ALIVE_HOOK_FATAL(x) ::MessageBox(NULL, x, "Alive hook error.", MB_ICONERROR | MB_OK); __debugbreak(); abort();

class BaseFunction
{
public:
    virtual ~BaseFunction() { }
    virtual void Apply() = 0;
    static std::map<DWORD, BaseFunction*>& FunctionTable();
    static void HookAll();
};

enum CallingConvention
{
    eCDecl,
    eStdCall,
    eFastCall
};

template <DWORD kAoGameFunctionAddress, DWORD kAeGameFunctionAddress, void* kReplacementFunctionAddress, CallingConvention convention, class Signature, class ReturnType, class... Args>
class AliveFunctionImpl : public BaseFunction
{
public:
    using TFuncType = Signature*;

    static DWORD Address()
    {
        if (Utils::IsAe())
        {
            return kAeGameFunctionAddress;
        }
        return kAoGameFunctionAddress;
    }

    AliveFunctionImpl(const char* fnName, bool isImplemented)
        : mFnName(fnName), mImplemented(isImplemented)
    {
        auto it = FunctionTable().find(Address());
        if (it != std::end(FunctionTable()))
        {
            // duplicated function
            std::stringstream ss;
            ss << "Duplicated function 0x" << std::hex << Address() << " name " << fnName;
            ALIVE_HOOK_FATAL(ss.str().c_str());
        }
        else
        {
            FunctionTable().insert(std::make_pair(Address(), this));
        }
    }

    virtual ~AliveFunctionImpl()
    {
        // No need to clear from the map.. it might be gone at this point anyway
    }

    ReturnType operator()(Args ... args)
    {
        #pragma warning(push)
        #pragma warning(disable:4127) // conditional expression is constant
        if (kReplacementFunctionAddress)
        {
            // Redirection of mReal -> new
            return reinterpret_cast<TFuncType>(kReplacementFunctionAddress)(args...);
        }
        else
        {
            // No redirection at all
            return mRealFuncPtr(args...);
        }
        #pragma warning(pop)
    }

    Signature* Ptr() const
    {
        return mRealFuncPtr;
    }

protected:
    static ReturnType __cdecl Cdecl_Static_Hook_Impl(Args ... args)
    {
        auto it = FunctionTable().find(Address());
        if (it == std::end(FunctionTable()))
        {
            // Impossible situation
            ALIVE_HOOK_FATAL("No function table??");
        }

        auto baseFunc = it->second;
        return static_cast<AliveFunctionImpl*>(baseFunc)->operator()(args...);
    }

    static ReturnType __stdcall StdCall_Static_Hook_Impl(Args ... args)
    {
        auto it = FunctionTable().find(Address());
        if (it == std::end(FunctionTable()))
        {
            // Impossible situation
            ALIVE_HOOK_FATAL("No function table??");
        }

        auto baseFunc = it->second;
        return static_cast<AliveFunctionImpl*>(baseFunc)->operator()(args...);
    }

    static ReturnType __fastcall FastCall_Static_Hook_Impl(Args ... args)
    {
        auto it = FunctionTable().find(Address());
        if (it == std::end(FunctionTable()))
        {
            // Impossible situation
            ALIVE_HOOK_FATAL("No function table??");
        }

        auto baseFunc = it->second;
        return static_cast<AliveFunctionImpl*>(baseFunc)->operator()(args...);
    }

    virtual void Apply() override
    {
        #pragma warning(push)
        #pragma warning(disable:4127) // conditional expression is constant
        // Redirect internal game function to our reimpl
        if (mImplemented && Address())
        {
            ApplyImpl(reinterpret_cast<void*>(Address()), kReplacementFunctionAddress);
        }
        #pragma warning(pop)
    }

private:
    void ApplyImpl(void* funcToHook, void* replacement)
    {
        std::cout << "old addr " << funcToHook << " new addr " << replacement << std::endl;

        mRealFuncPtr = (TFuncType)funcToHook;

        LONG err = 0;
        #pragma warning(push)
        #pragma warning(disable:4127) // conditional expression is constant
        if (convention == eCDecl)
        {
            err = DetourAttach(&(PVOID&)mRealFuncPtr, Cdecl_Static_Hook_Impl);
        }
        else if (convention == eStdCall)
        {
            err = DetourAttach(&(PVOID&)mRealFuncPtr, StdCall_Static_Hook_Impl);
        }
        else if (convention == eFastCall)
        {
            err = DetourAttach(&(PVOID&)mRealFuncPtr, FastCall_Static_Hook_Impl);
        }
        else
        {
            ALIVE_HOOK_FATAL("Unknown calling convention");
        }
        #pragma warning(pop)

        if (err != NO_ERROR)
        {
            std::stringstream ss;
            ss << "DetourAttach failed with " << err << " for " << std::hex << " 0x" << funcToHook;
            ALIVE_HOOK_FATAL(ss.str().c_str());
        }
    }

    TFuncType mRealFuncPtr = nullptr;
    const char* mFnName = nullptr;
    bool mImplemented = false;
};

template<DWORD kAoGameFunctionAddress, DWORD kAeGameFunctionAddress, void* kReplacementFunctionAddress, class ReturnType>
class AliveFunction;

// __cdecl partial specialization
template<DWORD kAoGameFunctionAddress, DWORD kAeGameFunctionAddress, void* kReplacementFunctionAddress, class ReturnType, class... Args>
class AliveFunction    <kAoGameFunctionAddress, kAeGameFunctionAddress, kReplacementFunctionAddress,          ReturnType __cdecl(Args...) > : public
      AliveFunctionImpl<kAoGameFunctionAddress, kAeGameFunctionAddress, kReplacementFunctionAddress, eCDecl,  ReturnType __cdecl(Args...), ReturnType, Args...>
{
public:
    explicit AliveFunction(const char* name, bool isImplemented = true) : AliveFunctionImpl(name, isImplemented) { }
};

// __stdcall partial specialization
template<DWORD kAoGameFunctionAddress, DWORD kAeGameFunctionAddress, void* kReplacementFunctionAddress, class  ReturnType, class ... Args>
class AliveFunction    <kAoGameFunctionAddress, kAeGameFunctionAddress, kReplacementFunctionAddress,           ReturnType __stdcall(Args...) > : public
      AliveFunctionImpl<kAoGameFunctionAddress, kAeGameFunctionAddress, kReplacementFunctionAddress, eStdCall, ReturnType __stdcall(Args...), ReturnType, Args...>
{
public:
    explicit AliveFunction(const char* name, bool isImplemented = true) : AliveFunctionImpl(name, isImplemented) { }
};

// __fastcall partial specialization
template<DWORD kAoGameFunctionAddress, DWORD kAeGameFunctionAddress, void* kReplacementFunctionAddress,   class ReturnType, class ... Args>
class AliveFunction    <kAoGameFunctionAddress, kAeGameFunctionAddress, kReplacementFunctionAddress,            ReturnType __fastcall(Args...) > : public
      AliveFunctionImpl<kAoGameFunctionAddress, kAeGameFunctionAddress, kReplacementFunctionAddress, eFastCall, ReturnType __fastcall(Args...), ReturnType, Args...>
{
public:
    explicit AliveFunction(const char* name, bool isImplemented = true) : AliveFunctionImpl(name, isImplemented) { }
};

#define ALIVE_FUNC_NOT_IMPL(aoAddr, aeAddr, signature, name) AliveFunction<aoAddr, aeAddr, nullptr, signature> name(#name);
#define ALIVE_EXTERN_FUNC_NOT_IMPL(aoAddr, aeAddr, signature, name) extern AliveFunction<aoAddr, aeAddr, nullptr, signature> name;
#define ALIVE_FUNC_IMPLEX(aoAddr, aeAddr, funcName, isImplemented) AliveFunction<aoAddr, aeAddr, funcName, decltype(funcName)> funcName##_(#funcName, isImplemented);

#define ALIVE_VAR(AddrAo, AddrAe, TypeName, VarName) TypeName& VarName = *reinterpret_cast<TypeName*>(Utils::IsAe() ? AddrAe : AddrAo);
#define ALIVE_VAR_EXTERN(TypeName, VarName) extern TypeName& VarName;


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

ALIVE_EXTERN_FUNC_NOT_IMPL(0x0, 0x004EF970, IDirectSoundBuffer*__cdecl(int soundIndex, int a2), sub_4EF970);

struct GameFunctions
{
    AddressFunction<HWND __cdecl()> GetWindowHandle{ Addrs().GetWindowHandle() };
    AddressFunction<int __cdecl(const char* sourceFile, int errorCode, int notUsed, const char* message)> error_msgbox = { Addrs().error_msgbox() };


    AddressFunction<int(__cdecl)(DWORD* hdc)> gdi_draw = { Addrs().gdi_draw() };
    AddressFunction<HDC(__cdecl)(DWORD* hdc)> ConvertAbeHdcHandle = { Addrs().ConvertAbeHdcHandle() };

    AddressFunction<DWORD(__cdecl)(DWORD* hdc, int hdc2)> ConvertAbeHdcHandle2 = { Addrs().ConvertAbeHdcHandle2() };
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
