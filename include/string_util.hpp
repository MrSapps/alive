#pragma once

#include <string>
#include <deque>
#include <algorithm>
#include <ctype.h>

namespace string_util
{
    inline std::string trim(const std::string& s)
    {
        auto wsfront = std::find_if_not(s.begin(), s.end(), [](int c){return ::isspace(c) || c == '\n' || c=='\r'; });
        auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](int c){return ::isspace(c) || c == '\n' || c == '\r'; }).base();
        return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
    }

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

    inline bool ends_with(std::string const& value, std::string const& ending, bool ignoreCase = false)
    {

        if (ending.size() > value.size())
        {
            return false;
        }

        if (ignoreCase)
        {
            std::string valueLower = value;
            std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(), ::tolower);

            std::string endingLower = ending;
            std::transform(endingLower.begin(), endingLower.end(), endingLower.begin(), ::tolower);

            return std::equal(endingLower.rbegin(), endingLower.rend(), valueLower.rbegin());
        }
        else
        {
            return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
        }
    }

    inline bool contains(const std::string& haystack, const std::string& needle)
    {
        return (haystack.find(needle) != std::string::npos);
    }
}
