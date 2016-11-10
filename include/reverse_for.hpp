#pragma once

#include <type_traits>

template<typename It>
class Range
{
    It b, e;
public:
    Range(It b, It e) : b(b), e(e) {}
    It begin() const { return b; }
    It end() const { return e; }
};

template<typename ORange, typename OIt = decltype(std::begin(std::declval<ORange>())), typename It = std::reverse_iterator<OIt>>
inline Range<It> reverse_for(ORange&& originalRange)
{
    return Range<It>(It(std::end(originalRange)), It(std::begin(originalRange)));
}
