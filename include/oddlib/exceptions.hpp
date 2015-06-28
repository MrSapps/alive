#pragma once

#include <stdexcept>

namespace Oddlib
{
    class Exception : public std::exception
    {
    public:
        explicit Exception(const char* msg)
            : std::exception(msg)
        {

        }
    };
}
