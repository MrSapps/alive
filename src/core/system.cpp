#include "core/system.hpp"

DEFINE_ROOT_SYSTEM(System);

System::~System() // NOLINT
{

}

void System::OnLoad()
{
    OnResolveDependencies();
}

void System::OnResolveDependencies()
{

}

#undef DEFINE_ROOT_SYSTEM