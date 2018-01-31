#include <type_traits>

#include "mapobject.hpp"
#include "core/components/transformcomponent.hpp"

DEFINE_COMPONENT(TransformComponent)

void TransformComponent::Serialize(std::ostream &os) const
{
	// static_assert(std::is_pod<decltype(mData)>::value);
	os.write(static_cast<const char*>(static_cast<const void*>(&mData)), sizeof(decltype(mData)));
}

void TransformComponent::Deserialize(std::istream &is)
{
    // static_assert(std::is_pod<decltype(mData)>::value);
	is.read(static_cast<char*>(static_cast<void*>(&mData)), sizeof(decltype(mData)));
}

void TransformComponent::Set(float xPos, float yPos)
{
    mData.mXPos = xPos;
    mData.mYPos = yPos;
}

void TransformComponent::SetX(float xPos)
{
    mData.mXPos = xPos;
}

void TransformComponent::SetY(float yPos)
{
    mData.mYPos = yPos;
}

float TransformComponent::GetX() const
{
    return mData.mXPos;
}

float TransformComponent::GetY() const
{
    return mData.mYPos;
}

void TransformComponent::Add(float xAmount, float yAmount)
{
    mData.mXPos += xAmount;
    mData.mYPos += yAmount;
}

void TransformComponent::AddX(float xAmount)
{
    mData.mXPos += xAmount;
}

void TransformComponent::AddY(float yAmount)
{
    mData.mYPos += yAmount;
}

void TransformComponent::SnapXToGrid()
{
    mData.mXPos = ::SnapXToGrid(mData.mXPos);
}