#pragma once

#include <string>
#include <deque>

namespace string_util
{
    inline std::deque<std::string> split(const std::string& input, char delimiter)
    {
        std::deque<std::string> tokens;
        size_t start = 0;
        size_t end = 0;
        while ((end = input.find(delimiter, start)) != std::string::npos)
        {
            tokens.push_back(input.substr(start, end - start));
            start = end + 1;
        }
        tokens.push_back(input.substr(start));
        return tokens;
    }

    inline bool ends_with(std::string const& value, std::string const& ending)
    {
        if (ending.size() > value.size())
        {
            return false;
        }
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    inline bool contains(const std::string& haystack, const std::string& needle)
    {
        return (haystack.find(needle) != std::string::npos);
    }
}
