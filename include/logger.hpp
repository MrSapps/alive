#pragma once

#include <exception>
#include <iostream>

#ifdef _MSVC_VER
#define FNAME __FUNCSIG__
#else
#define FNAME __PRETTY_FUNCTION__
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
    std::cout << function;
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
constexpr LogData<std::pair<Begin&&, Value&&>> operator<<(LogData<Begin>&& begin,
                                                          Value&& value) noexcept
{
    return {{ std::forward<Begin>(begin.list), std::forward<Value>(value) }};
}

template<typename Begin, size_t n>
constexpr LogData<std::pair<Begin&&, const char*>> operator<<(LogData<Begin>&& begin,
                                                              const char (&value)[n]) noexcept
{
    return {{ std::forward<Begin>(begin.list), value }};
}

typedef std::ostream& (*PfnManipulator)(std::ostream&);

template<typename Begin>
constexpr LogData<std::pair<Begin&&, PfnManipulator>> operator<<(LogData<Begin>&& begin,
                                                                 PfnManipulator value) noexcept
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
#define LOG_TRACE(msg) (Log(FNAME, LogData<None>() << " [T] " << msg))
#define LOG_INFO(msg) (Log(FNAME, LogData<None>() << " [I] " << msg))
#define LOG_WARNING(msg) (Log(FNAME, LogData<None>() << " [W] " << msg))
#define LOG_ERROR(msg) (Log(FNAME, LogData<None>() << " [E] " << msg))
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
            LOG("[ENTER] " << mFuncName);
        }

        ~AutoLog()
        {
            if (std::uncaught_exception())
            {
                LOG("[EXIT_EXCEPTION] " << mFuncName);
            }
            else
            {
                LOG("[EXIT]  " << mFuncName);
            }
        }

    private:
        const char* mFuncName;
    };
}
