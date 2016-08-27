#pragma once

#include "oddlib/stream.hpp"
#include "string_util.hpp"
#include <string>
#include <vector>
#include <deque>

// Very basic SRT/subrip parser. Will only handle 1 line per subtitle
class SubTitle
{
public:
    SubTitle()
    {

    }

    SubTitle(std::deque<std::string>& lines)
    {
        ParseSequnceNumber(TakeLine(lines));
        ParseStartEndTime(TakeLine(lines));

        // Text continues till we hit a blank line or EOF
        while (!lines.empty() && !IsBlankLine(lines.front()))
        {
            ParseText(TakeLine(lines));
        }
    }

    Uint64 SequnceNumber() const { return mSequnceNumber; }
    Uint64 StartTimeStampMsec() const { return mStartTimeStamp; }
    Uint64 EndTimeStampMsec() const { return mEndTimeStamp; }
    const std::string& Text() const { return mSubTitleText; }

protected:
    bool IsBlankLine(const std::string& line)
    {
        return string_util::trim(line).empty();
    }

    void ParseSequnceNumber(const std::string& line)
    {
        // 1, 2, 3 etc
        mSequnceNumber = std::stoll(line);
    }

    void ParseStartEndTime(const std::string& line)
    {
        // "00:00:10,500 --> 00:00:13,000"
        const std::string startTime = line.substr(0, 12);
        mStartTimeStamp = ParseTime(startTime);
        const std::string divider = line.substr(12, 5);
        if (divider != " --> ")
        {
            throw Oddlib::Exception("Invalid time stamp");
        }
        const std::string endTime = line.substr(17);
        mEndTimeStamp = ParseTime(endTime);
    }

    Uint64 ParseTime(const std::string& timeStr)
    {
        const Uint64 hh = std::stol(timeStr.substr(0, 2)) * 60 * 60 * 1000; // to mins, to seconds, to msec
        const Uint64 mm = std::stol(timeStr.substr(3, 2)) * 60 * 1000; // to seconds to msec
        const Uint64 ss = std::stol(timeStr.substr(6, 2)) * 1000; // to msec
        const Uint64 ms = std::stol(timeStr.substr(9, 3)); // already msec
        return hh + mm + ss + ms;
    }

    void ParseText(const std::string& text)
    {
        if (mSubTitleText.empty())
        {
            mSubTitleText = text;
        }
        else
        {
            mSubTitleText += "\n" + text;
        }
    }

    std::string TakeLine(std::deque<std::string>& lines)
    {
        std::string ret;
        do
        {
            if (lines.empty())
            {
                throw Oddlib::Exception("Premature end of file");
            }

            ret = lines.front();
            ret = string_util::trim(ret);
            lines.pop_front();
        } while (ret.empty());

        return ret;
    }

private:
    Uint64 mSequnceNumber = 0;
    Uint64 mStartTimeStamp = 0;
    Uint64 mEndTimeStamp = 0;
    std::string mSubTitleText;
};

class SubTitleParser
{
public:
    SubTitleParser(std::unique_ptr<Oddlib::IStream> stream)
    {
        Parse(stream->LoadAllToString());
    }

    SubTitleParser(const std::string& input)
    {
        Parse(input);
    }

    std::vector<const SubTitle*> Find(Uint64 curTime)
    {
        // TODO: Performance - might be slow if 100,000s of sub titles since this is a brute force
        // range search.
        std::vector<const SubTitle*> ret;
        for (const SubTitle& sub : mSubTitles)
        {
            if (sub.StartTimeStampMsec() <= curTime)
            {
                if (sub.EndTimeStampMsec() >= curTime)
                {
                    ret.push_back(&sub);
                }
            }
        }
        return ret;
    }

protected:
    void Parse(const std::string& input)
    {
        std::deque<std::string> lines = string_util::split(input, L'\n');
        while (!lines.empty() && lines.size() >= 3)
        {
            Parse(lines);
        }

        // Sort by starting time
        std::sort(std::begin(mSubTitles), std::end(mSubTitles), [](const SubTitle& lhs, const SubTitle& rhs)
        {
            return lhs.StartTimeStampMsec() < rhs.StartTimeStampMsec();
        });
    }

    void Parse(std::deque<std::string>& lines)
    {
        mSubTitles.emplace_back(lines);
    }

    std::vector<SubTitle> mSubTitles;
};
