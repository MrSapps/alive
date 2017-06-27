#pragma once

#include <stdlib.h>

enum eDataSetType
{
    eAoPc,
    eAoPcDemo,
    eAoPsx,
    eAoPsxDemo,
    eAePc,
    eAePcDemo,
    eAePsxCd1,
    eAePsxCd2,
    eAePsxDemo
};

inline bool IsPsx(eDataSetType type)
{
    switch (type)
    {
    case eAoPc:
    case eAoPcDemo:
    case eAePc:
    case eAePcDemo:
        return false;

    case eAoPsx:
    case eAoPsxDemo:
    case eAePsxCd1:
    case eAePsxCd2:
    case eAePsxDemo:
        return true;
    }
    abort();
}

inline bool IsAo(eDataSetType type)
{
    switch (type)
    {
    case eAoPc:
    case eAoPcDemo:
    case eAoPsx:
    case eAoPsxDemo:
        return true;

    case eAePsxCd1:
    case eAePsxCd2:
    case eAePsxDemo:
    case eAePc:
    case eAePcDemo:
        return false;
    }
    abort();
}

inline const char* ToString(eDataSetType type)
{
    switch (type)
    {
    case eAoPc:
        return "AoPc";

    case eAoPcDemo:
        return "AoPcDemo";

    case eAoPsx:
        return "AoPsx";

    case eAoPsxDemo:
        return "AoPsxDemo";

    case eAePc:
        return "AePc";

    case eAePcDemo:
        return "AePcDemo";

    case eAePsxCd1:
        return "AePsxCd1";

    case eAePsxCd2:
        return "AePsxCd2";

    case eAePsxDemo:
        return "AePsxDemo";
    }
    abort();
}
