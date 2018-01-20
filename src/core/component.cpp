//
// Created by Jonathan on 21-Jan-18.
//

#include "core/component.hpp"

void Component::Update()
{

}

void Component::Render(AbstractRenderer&) const
{

}

void Component::SetEntity(class Entity* entity)
{
    mEntity = entity;
}

void Component::SetId(ComponentIdentifier id)
{
    mId = id;
}

ComponentIdentifier Component::GetId() const
{
    return mId;
}