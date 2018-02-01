#include <algorithm>

#include "core/systems/collisionsystem.hpp"

DEFINE_SYSTEM(CollisionSystem);

void CollisionSystem::Update()
{

}

static CollisionLine* GetCollisionIndexByIndex(CollisionLines& lines, s16 index)
{
    const s32 count = static_cast<s32>(lines.size());
    if (index > 0)
    {
        if (index < count)
        {
            return lines[index].get();
        }
        else
        {
            LOG_ERROR("Link index is out of bounds: " << index);
        }
    }
    return nullptr;
}

static void ConvertLink(CollisionLines& lines, const Oddlib::Path::Links& oldLink, CollisionLine::Link& newLink)
{
    newLink.mPrevious = GetCollisionIndexByIndex(lines, oldLink.mPrevious);
    newLink.mNext = GetCollisionIndexByIndex(lines, oldLink.mNext);
}

void CollisionSystem::ConvertCollisionItems(const std::vector<Oddlib::Path::CollisionItem>& items)
{
    const auto count = static_cast<s32>(items.size());
    mCollisionLines.resize(count);

    // First pass to create/convert from original/"raw" path format
    for (auto i = 0; i < count; i++)
    {
        mCollisionLines[i] = std::make_unique<CollisionLine>();
        mCollisionLines[i]->mLine.mP1.x = items[i].mP1.mX;
        mCollisionLines[i]->mLine.mP1.y = items[i].mP1.mY;

        mCollisionLines[i]->mLine.mP2.x = items[i].mP2.mX;
        mCollisionLines[i]->mLine.mP2.y = items[i].mP2.mY;

        mCollisionLines[i]->mType = CollisionLine::ToType(items[i].mType);
    }

    // Second pass to set up raw pointers to existing lines for connected segments of 
    // collision lines
    for (auto i = 0; i < count; i++)
    {
        // TODO: Check if optional link is ever used in conjunction with link
        ConvertLink(mCollisionLines, items[i].mLinks[0], mCollisionLines[i]->mLink);
        ConvertLink(mCollisionLines, items[i].mLinks[1], mCollisionLines[i]->mOptionalLink);
    }

    // Now we can re-order collision items without breaking prev/next links, thus we want to ensure
    // that anything that either has no links, or only a single prev/next links is placed first
    // so that we can render connected segments from the start or end.
    std::sort(std::begin(mCollisionLines), std::end(mCollisionLines), [](std::unique_ptr<CollisionLine>& a, std::unique_ptr<CollisionLine>& b)
    {
        return std::tie(a->mLink.mNext, a->mLink.mPrevious) < std::tie(b->mLink.mNext, b->mLink.mPrevious);
    });

    // Ensure that lines link together physically
    for (auto i = 0; i < count; i++)
    {
        // Some walls have next links, overlapping the walls will break them
        if (mCollisionLines[i]->mLink.mNext && mCollisionLines[i]->mType == CollisionLine::eTrackLine)
        {
            mCollisionLines[i]->mLine.mP2 = mCollisionLines[i]->mLink.mNext->mLine.mP1;
        }
    }
}

void CollisionSystem::Clear()
{
    mCollisionLines.clear();
}

CollisionRaycast CollisionSystem::Raycast(glm::vec2 origin, glm::vec2 direction, std::initializer_list<CollisionLine::eLineTypes> lineMask) const
{
    const CollisionLine* nearestLine = nullptr;
    auto nearestDistance = 0.0f;
    glm::vec2 nearestCollision;

    for (const auto& line : mCollisionLines)
    {
        auto found = false;
        for (auto type : lineMask)
        {
            if (type == line->mType)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            continue;
        }

        const auto line1p1x = origin.x;
        const auto line1p1y = origin.y;
        const auto line1p2x = direction.x;
        const auto line1p2y = direction.y;

        const auto line2p1x = line->mLine.mP1.x;
        const auto line2p1y = line->mLine.mP1.y;
        const auto line2p2x = line->mLine.mP2.x;
        const auto line2p2y = line->mLine.mP2.y;

        // Get the segments' parameters.
        const auto dx12 = line1p2x - line1p1x;
        const auto dy12 = line1p2y - line1p1y;
        const auto dx34 = line2p2x - line2p1x;
        const auto dy34 = line2p2y - line2p1y;

        // Solve for t1 and t2
        const float denominator = (dy12 * dx34 - dx12 * dy34);
        if (denominator == 0.0f)
        {
            continue;
        }

        const auto t1 = ((line1p1x - line2p1x) * dy34 + (line2p1y - line1p1y) * dx34) / denominator;
        const auto t2 = ((line2p1x - line1p1x) * dy12 + (line1p1y - line2p1y) * dx12) / -denominator;

        // Find the point of intersection.
        const auto intersectionX = line1p1x + dx12 * t1;
        const auto intersectionY = line1p1y + dy12 * t1;

        // The segments intersect if t1 and t2 are between 0 and 1.
        const auto hasCollided = ((t1 >= 0) && (t1 <= 1) && (t2 >= 0) && (t2 <= 1));

        if (hasCollided)
        {
            const auto distance = glm::distance(glm::vec2(line1p1x - intersectionX), glm::vec2(line1p1y - intersectionY));
            if (!nearestLine || distance < nearestDistance)
            {
                nearestCollision.x = intersectionX;
                nearestCollision.y = intersectionY;
                nearestDistance = distance;
                nearestLine = line.get();
            }
        }
    }

    if (nearestLine != nullptr)
    {
        return { nearestDistance, nearestCollision, origin };
    }
    return {};
}

CollisionRaycast::CollisionRaycast(float mDistance, const glm::vec2& mPoint, const glm::vec2& mOrigin) : mDistance(mDistance), mPoint(mPoint), mOrigin(mOrigin)
{

}

CollisionRaycast::operator bool() const
{
    return mDistance >= 0;
}