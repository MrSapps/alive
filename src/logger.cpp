#include "logger.hpp"
#include <iostream>
#include <memory>
#include <stdarg.h>

#ifdef _MSC_VER
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

namespace Logging
{
    const static int kIdentSize = 2;
    THREAD_LOCAL size_t perThreadIndent = 0;

    void LogLine(eLogLevels level, const char* formatStr, ...)
    {
        if (level == eExit)
        {
            perThreadIndent -= kIdentSize;
        }

        va_list params;
        va_start(params, formatStr);

        const int bufsize = _vscprintf(formatStr, params) + 1;
        std::unique_ptr<char[]> buffer(new (std::nothrow) char[bufsize]);
        if (buffer.get())
        {
            memset(buffer.get(), 0, bufsize);
            if (_vsnprintf_s(buffer.get(), bufsize, bufsize - 1, formatStr, params) > 0)
            {
                for (size_t i = 0; i < perThreadIndent; i++)
                {
                    if (i % kIdentSize == 0)
                    {
                        std::cout << "|";
                    }
                    else
                    {
                        std::cout << " ";
                    }
                }
                std::cout << buffer.get();
            }
        }

        va_end(params);

        if (level == eEnter)
        {
            perThreadIndent += kIdentSize;
        }
    }
}
