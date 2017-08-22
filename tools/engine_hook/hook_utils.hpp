#pragma once

#include <windows.h>

struct IDirectDraw;

namespace Utils
{
    HRESULT WINAPI NewDirectDrawCreate(GUID* lpGUID, IDirectDraw** lplpDD, IUnknown* pUnkOuter);
    using TDirectDrawCreate = decltype(&NewDirectDrawCreate);

    HMODULE LoadRealDDrawDll();

    template<class T>
    inline T TGetProcAddress(HMODULE hDll, const char* func)
    {
#pragma warning(suppress: 4191)
        return reinterpret_cast<T>(::GetProcAddress(hDll, func));
    }

    TDirectDrawCreate GetFunctionPointersToRealDDrawFunctions(HMODULE hRealDll);

    void FatalExit(const char* msg);

    bool IsAe();
}
