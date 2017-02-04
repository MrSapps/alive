#pragma once

template<class U, class T>
inline bool IsBitOn(U& data, T bitNumber)
{
    return (data & (1 << bitNumber)) > 0;
}

template<class U, class T>
inline void BitOn(U& data, T bitNumber)
{
    data |= (1 << bitNumber);
}

template<class U, class T>
inline void BitOff(U& data, T bitNumber)
{
    data &= ~(1 << bitNumber);
}

template<class U, class T>
inline void BitOnOrOff(U& data, T bitNumber, bool on)
{
    if (on)
    {
        BitOn(data, bitNumber);
    }
    else
    {
        BitOff(data, bitNumber);
    }
}
