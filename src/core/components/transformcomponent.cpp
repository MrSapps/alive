#include <ostream>
#include <istream>

#include "types.hpp"
#include "core/entity.hpp"
#include "core/components/transformcomponent.hpp"

DEFINE_COMPONENT(TransformComponent);

void TransformComponent::Serialize(std::ostream& os) const
{
    static_assert(std::is_pod<decltype(mData)>::value, "TransformComponent::mData is not a POD type");
    os.write(static_cast<const char*>(static_cast<const void*>(&mData)), sizeof(decltype(mData)));
}

void TransformComponent::Deserialize(std::istream& is)
{
    static_assert(std::is_pod<decltype(mData)>::value, "TransformComponent::mData is not a POD type");
    is.read(static_cast<char*>(static_cast<void*>(&mData)), sizeof(decltype(mData)));
}

void TransformComponent::Set(float x, float y)
{
    mData.mX = x;
    mData.mY = y;
}

void TransformComponent::Set(float x, float y, float width, float height)
{
    mData.mX = x;
    mData.mY = y;
    mData.mWidth = width;
    mData.mHeight = height;
}

void TransformComponent::SetX(float x)
{
    mData.mX = x;
}

void TransformComponent::SetY(float y)
{
    mData.mY = y;
}

void TransformComponent::SetWidth(float width)
{
    mData.mWidth = width;
}

void TransformComponent::SetHeight(float height)
{
    mData.mHeight = height;
}

float TransformComponent::GetX() const
{
    return mData.mX;
}

float TransformComponent::GetY() const
{
    return mData.mY;
}

float TransformComponent::GetWidth() const
{
    return mData.mWidth;
}

float TransformComponent::GetHeight() const
{
    return mData.mHeight;
}

void TransformComponent::Add(float xAmount, float yAmount)
{
    mData.mX += xAmount;
    mData.mY += yAmount;
}

void TransformComponent::AddX(float xAmount)
{
    mData.mX += xAmount;
}

void TransformComponent::AddY(float yAmount)
{
    mData.mY += yAmount;
}

void TransformComponent::SnapXToGrid()
{
    //25x20 grid hack
    const auto posX = static_cast<s32>(mData.mX);
    const auto gridPos = (posX - 12) % 25;
    mData.mX = static_cast<float>((gridPos >= 13 ? posX - gridPos + 25 : posX - gridPos));
}
