#include "types.hpp"
#include "core/components/transformcomponent.hpp"

DEFINE_COMPONENT(TransformComponent);

void TransformComponent::Serialize(std::ostream &os) const
{
	static_assert(std::is_pod<decltype(mData)>::value, "TransformComponent::mData is not a POD type");
	os.write(static_cast<const char*>(static_cast<const void*>(&mData)), sizeof(decltype(mData)));
}

void TransformComponent::Deserialize(std::istream &is)
{
    static_assert(std::is_pod<decltype(mData)>::value, "TransformComponent::mData is not a POD type");
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
    //25x20 grid hack
    const auto posX = static_cast<s32>(mData.mXPos);
    const auto gridPos = (posX - 12) % 25;
    mData.mXPos = static_cast<float>((gridPos >= 13 ? posX - gridPos + 25 : posX - gridPos));
}