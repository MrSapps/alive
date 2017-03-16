#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4702)
#pragma warning(disable: 4244)
#pragma warning(disable: 4800)
#pragma warning(disable: 4458)
#endif

#ifndef SCRAT_USE_CXX11_OPTIMIZATIONS
#define SCRAT_USE_CXX11_OPTIMIZATIONS
#endif

#include <sqrat.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template<class T>
class InstanceBinder
{
public: 
    InstanceBinder(const std::string& varName, T* instancePtr)
        : mInstanceName(varName)
    {
        Sqrat::RootTable().SetValue(mInstanceName.c_str(), instancePtr);
    }

    ~InstanceBinder()
    {
        Sqrat::RootTable().SetValue(mInstanceName.c_str(), NULL);
    }

private:
    std::string mInstanceName;
};