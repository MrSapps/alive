#include <cstring>

#include "core/component.hpp"

DEFINE_COMPONENT(Component);

Component::~Component() // NOLINT
{

}

void Component::OnLoad()
{
    OnResolveDependencies();
}

void Component::Serialize(std::ostream&) const
{

}

void Component::Deserialize(std::istream&)
{

}

void Component::OnResolveDependencies()
{

}