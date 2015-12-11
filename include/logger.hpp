#pragma once

#include <exception>
#include <iostream>

#if _MSC_VER
#define FNAME __FUNCTION__
#else
#define FNAME __PRETTY_FUNCTION__
#endif

// For VS2013 and younger constxpr/noexcept isn't implemented
#if _MSC_VER < 1900
#define CONSTXPR 
#define NOEXEPT 
#else
#define CONSTXPR constexpr
#define NOEXEPT noexcept
#endif

enum colour { DARKBLUE = 1, DARKGREEN, DARKTEAL, DARKRED, DARKPINK, DARKYELLOW, GRAY, DARKGRAY, BLUE, GREEN, TEAL, RED, PINK, YELLOW, WHITE };

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
static void* gConsoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);

struct setcolour
{
    colour _c;
    void* _console_handle;

    setcolour(colour c)
        : _c(c), _console_handle(0)
    {
        _console_handle = gConsoleHandle;
    }
};


static std::basic_ostream<char> &operator<<(std::basic_ostream<char> &s, const setcolour &ref)
{
    ::SetConsoleTextAttribute(ref._console_handle, static_cast<WORD>(ref._c));
    return s;
}
#else
struct setcolour
{
    colour _c;
    setcolour(colour c)
        : _c(c)
    {
    }
};

static std::basic_ostream<char> &operator<<(std::basic_ostream<char>& s, const setcolour& /*ref*/)
{
    // Not supported on other platforms yet
    return s;
}
#endif

struct None
{

};

template<typename List>
struct LogData
{
    List list;
};

template<typename List>
void Log(const char* function, LogData<List>&& data)
{
    std::cout << setcolour(PINK) << function << std::flush;
    output(std::cout, std::move(data.list));
    std::cout << std::endl;
}

template<typename List>
void Log(LogData<List>&& data)
{
    output(std::cout, std::move(data.list));
    std::cout << std::endl;
}

template<typename Begin, typename Value>
CONSTXPR LogData<std::pair<Begin&&, Value&&>> operator<<(LogData<Begin>&& begin,
    Value&& value) NOEXEPT
{
    return {{ std::forward<Begin>(begin.list), std::forward<Value>(value) }};
}

template<typename Begin, size_t n>
CONSTXPR LogData<std::pair<Begin&&, const char*>> operator<<(LogData<Begin>&& begin,
    const char(&value)[n]) NOEXEPT
{
    return {{ std::forward<Begin>(begin.list), value }};
}

typedef std::ostream& (*PfnManipulator)(std::ostream&);

template<typename Begin>
CONSTXPR LogData<std::pair<Begin&&, PfnManipulator>> operator<<(LogData<Begin>&& begin,
    PfnManipulator value) NOEXEPT
{
    return {{ std::forward<Begin>(begin.list), value }};
}

template <typename Begin, typename Last>
void output(std::ostream& os, std::pair<Begin, Last>&& data)
{
    output(os, std::move(data.first));
    os << data.second;
}

inline void output(std::ostream& /*os*/, None)
{

}

#define TRACE_ENTRYEXIT Logging::AutoLog __funcTrace(FNAME)
#define LOG_TRACE(msg) (Log(FNAME, LogData<None>() << setcolour(WHITE) << " [T] " << msg << std::flush))
#define LOG_INFO(msg) (Log(FNAME, LogData<None>() << setcolour(GREEN) << " [I] " << msg << std::flush))
#define LOG_WARNING(msg) (Log(FNAME, LogData<None>() << setcolour(YELLOW) << " [W] " << msg << std::flush))
#define LOG_ERROR(msg) (Log(FNAME, LogData<None>() << setcolour(RED) << " [E] " << msg << std::flush))
#define LOG(msg) (Log(LogData<None>() << msg))

namespace Logging
{
    class AutoLog
    {
    public:
        AutoLog(const AutoLog&) = delete;
        AutoLog& operator = (const AutoLog&) = delete;
        AutoLog(const char* funcName)
          : mFuncName(funcName)
        {
            LOG(setcolour(WHITE) << "[ENTER] " << mFuncName << std::flush);
        }

        ~AutoLog()
        {
            if (std::uncaught_exception())
            {
                LOG(setcolour(WHITE) << "[EXIT_EXCEPTION] " << mFuncName << std::flush);
            }
            else
            {
                LOG(setcolour(WHITE) << "[EXIT]  " << mFuncName << std::flush);
            }
        }

    private:
        const char* mFuncName;
    };
}
