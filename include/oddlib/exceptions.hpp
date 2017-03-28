#pragma once

#include <stdexcept>
#include <string>

namespace Oddlib
{
    class Exception : public std::exception
    {
    public:
        explicit Exception(const char* msg) : mMsg(msg  ? msg : "") { }
        explicit Exception(std::string msg) : mMsg(msg) { }
        const char* what() const throw () override { return mMsg.c_str(); }
    private:
        std::string mMsg;
    };
}
