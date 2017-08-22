#pragma once

#include "types.hpp"

// First 16bits is the whole number, last 16bits is the remainder (??)
class HalfFloat
{
public:
    HalfFloat() = default;
    HalfFloat(s32 value) : mValue(value) { }
    f64 AsDouble() const { return static_cast<double>(mValue) / 65536.0; }
    HalfFloat& operator = (int value) { mValue = value; return *this; }
    bool operator != (const HalfFloat& r) const { return mValue != r.mValue; }
private:
    friend HalfFloat operator -(const HalfFloat& l, const HalfFloat& r);
    s32 mValue;
};

inline HalfFloat operator - (const HalfFloat& l, const HalfFloat& r) { return HalfFloat(l.mValue - r.mValue); }
