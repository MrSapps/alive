#include "hook_utils.hpp"
#include "string_util.hpp"

namespace Utils
{
    HMODULE LoadRealDDrawDll()
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

    TDirectDrawCreate GetFunctionPointersToRealDDrawFunctions(HMODULE hRealDll)
    {
        auto ptr = TGetProcAddress<TDirectDrawCreate>(hRealDll, "DirectDrawCreate");
        if (!ptr)
        {
            FatalExit("Can't find DirectDrawCreate function in real dll");
        }
        return ptr;
    }

    void FatalExit(const char* msg)
    {
        ::MessageBox(NULL, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
        exit(-1);
    }

    bool IsAe()
    {
        wchar_t buffer[MAX_PATH + 1] = {};
        ::GetModuleFileNameW(GetModuleHandle(NULL), buffer, _countof(buffer));

        const auto exeName = string_util::split(std::wstring(buffer), L'\\').back();
        return _wcsicmp(exeName.c_str(), L"Exoddus.exe") == 0;
    }
}
