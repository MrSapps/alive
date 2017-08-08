#pragma once

HRESULT WINAPI NewDirectDrawCreate(GUID* lpGUID, IDirectDraw** lplpDD, IUnknown* pUnkOuter);
using TDirectDrawCreate = decltype(&NewDirectDrawCreate);

inline HMODULE LoadRealDDrawDll()
{
    char infoBuf[MAX_PATH] = {};
    ::GetSystemDirectory(infoBuf, MAX_PATH);

    strcat_s(infoBuf, "\\ddraw.dll");

    const HMODULE hRealDll = ::LoadLibrary(infoBuf);
    if (!hRealDll)
    {
        FatalExit("Can't load or find real DDraw.dll");
    }
    return hRealDll;
}

template<class T>
inline T TGetProcAddress(HMODULE hDll, const char* func)
{
#pragma warning(suppress: 4191)
    return reinterpret_cast<T>(::GetProcAddress(hDll, func));
}

inline TDirectDrawCreate GetFunctionPointersToRealDDrawFunctions(HMODULE hRealDll)
{
    auto ptr = TGetProcAddress<TDirectDrawCreate>(hRealDll, "DirectDrawCreate");
    if (!ptr)
    {
        FatalExit("Can't find DirectDrawCreate function in real dll");
    }
    return ptr;
}

