#pragma once

#ifdef _MSC_VER
#pragma warning(push)
// warning C4265: 'std::_Pad' : class has virtual functions, but destructor is not virtual
#pragma warning(disable:4265)
#endif
#include <thread>
#include <mutex>
#include <functional>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
