#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244) // return conversion from big to small possible loss of data
#pragma warning(disable:4555) // expression has no effect - in template generated code
#pragma warning(disable:4582) // constructor is not implicitly called
#pragma warning(disable:4583) // destructor is not implicitly called
#pragma warning(disable:5027) // move assignment operator was implicitly defined as deleted

#endif
#include "sol/sol.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
