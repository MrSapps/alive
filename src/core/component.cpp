#include "core/component.hpp"

DEFINE_ROOT_COMPONENT(Component);

Component::~Component() // NOLINT
{

}

void Component::OnLoad()
{
    OnResolveDependencies();
}

void Component::OnResolveDependencies()
{

}

void Component::Serialize(std::ostream&) const
{

}

void Component::Deserialize(std::istream&)
{

}

#undef DEFINE_ROOT_COMPONENT