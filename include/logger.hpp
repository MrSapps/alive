#pragma once

#include <exception>

#define TRACE_ENTRYEXIT Logging::AutoLog __funcTrace("[ENTER] " __FUNCTION__, "[EXIT] " __FUNCTION__, "[EXIT_EXCEPTION] " __FUNCTION__)

#define LOG(level, levelStr, formatString, ...) LogLine(level, levelStr "[" __FUNCTION__ "]" " " ## formatString "\r\n", ##__VA_ARGS__)
#define LOG_NO_FUNC(level, levelStr, formatString, ...) LogLine(level, levelStr "" ## formatString "\r\n", ##__VA_ARGS__)

#define LOG_TRACE(formatString, ...) LOG(Logging::eTrace, "[T] ", formatString, __VA_ARGS__)
#define LOG_INFO(formatString, ...) LOG(Logging::eInfo, "[I] ", formatString, __VA_ARGS__)
#define LOG_WARNING(formatString, ...) LOG(Logging::eWarning, "[W] ", formatString, __VA_ARGS__)
#define LOG_ERROR(formatString, ...) LOG(Logging::eError, "[E] ", formatString, __VA_ARGS__)

inline void LogLine(int level, const char* formatStr, ...);

namespace Logging
{
    enum eLogLevels
    {
        eEnter,
        eExit,
        eTrace,
        eInfo,
        eWarning,
        eError
    };

    void LogLine(eLogLevels level, const char* formatStr, ...);

    class AutoLog
    {
    public:
        AutoLog(const AutoLog&) = delete;
        AutoLog& operator = (const AutoLog&) = delete;
        AutoLog(const char* enterStr, const char* exitStr, const char* exceptionExitStr)
          : mExitStr(exitStr),
            mExceptionExitStr(exceptionExitStr)
        {
            LOG_NO_FUNC(Logging::eEnter, "", "%s", enterStr);
        }

        ~AutoLog()
        {
            if (std::uncaught_exception())
            {
                LOG_NO_FUNC(Logging::eExit, "", "%s", mExceptionExitStr);
            }
            else
            {
                LOG_NO_FUNC(Logging::eExit, "", "%s", mExitStr);
            }
        }

    private:
        const char* mExitStr;
        const char* mExceptionExitStr;
    };
}



