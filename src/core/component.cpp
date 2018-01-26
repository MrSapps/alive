#include <cstring>

#include "core/component.hpp"

DEFINE_COMPONENT(Component);

Component::~Component() // NOLINT
{

}

void Component::Serialize(std::ostream&) const
{

}

void Component::Deserialize(std::istream&)
{
	Load();
}

void Component::Load()
{

}