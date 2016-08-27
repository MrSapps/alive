#pragma once

#include <string>
#include <codecvt>
#include <deque>
#include <algorithm>
#include <ctype.h>

namespace string_util
{
    inline void replace_all(std::string& input, char find, const char replace)
    {
        size_t pos = 0;
        while ((pos = input.find(find, pos)) != std::string::npos)
        {
            input.replace(pos, 1, 1, replace);
            pos += 1;
        }
    }

    inline void replace_all(std::string& input, const std::string& find, const std::string& replace)
    {
        size_t pos = 0;
        while ((pos = input.find(find, pos)) != std::string::npos)
        {
            input.replace(pos, find.length(), replace);
            pos += replace.length();
        }
    }

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

    inline char c_tolower(int c)
    {
        return static_cast<char>(::tolower(c));
    }

    inline bool iequals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size())
        {
            return false;
        }

        for (auto i = 0u; i < a.size(); ++i)
        {
            if (c_tolower(a[i]) != c_tolower(b[i]))
            {
                return false;
            }
        }

        return true;
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
            std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(), c_tolower);

            std::string endingLower = ending;
            std::transform(endingLower.begin(), endingLower.end(), endingLower.begin(), c_tolower);

            return std::equal(endingLower.rbegin(), endingLower.rend(), valueLower.rbegin());
        }
        else
        {
            return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
        }
    }

    inline bool starts_with(const std::string& toCheck, const std::string& prefix, bool ignoreCase = false)
    {
        if (prefix.size() > toCheck.size())
        {
            return false;
        }

        if (ignoreCase)
        {
            std::string ltoCheck = toCheck;
            std::transform(ltoCheck.begin(), ltoCheck.end(), ltoCheck.begin(), c_tolower);

            std::string lprefix = prefix;
            std::transform(lprefix.begin(), lprefix.end(), lprefix.begin(), c_tolower);

            return std::equal(lprefix.begin(), lprefix.end(), ltoCheck.begin());
        }
        return std::equal(prefix.begin(), prefix.end(), toCheck.begin());
    }

    inline bool contains(const std::string& haystack, const std::string& needle)
    {
        return (haystack.find(needle) != std::string::npos);
    }

    inline std::string wstring_to_utf8(const std::wstring& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.to_bytes(str);
    }
}
