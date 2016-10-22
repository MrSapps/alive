#pragma once

#include "types.hpp"
#include "physics.hpp"
#include "renderer.hpp"
#include <vector>
#include <memory>
#include <map>
#include <glm/vec2.hpp>

using CollisionLines = std::vector<std::unique_ptr<class CollisionLine>>;

class CollisionLine
{
public:

    glm::vec2 mP1;
    glm::vec2 mP2;

    enum eLineTypes : u32
    {
        eFloor = 0,
        eWallLeft = 1,
        eWallRight = 2,
        eCeiling = 3,
        eBackGroundFloor = 4,
        eBackGroundWallLeft = 5,
        eBackGroundWallRight = 6,
        eBackGroundCeiling = 7,
        eTrackLine = 8,
        eArt = 9,
        eBulletWall = 10,
        eMineCarFloor = 11,
        eMineCarWall = 12,
        eMineCarCeiling = 13,
        eFlyingSligCeiling = 17,
        eUnknown = 99
    };
    eLineTypes mType = eFloor;

    struct Link
    {
        CollisionLine* mPrevious = nullptr;
        CollisionLine* mNext = nullptr;
    };

    Link mLink;
    Link mOptionalLink; // TODO: Might not be required

    CollisionLine() = default;
    CollisionLine(glm::vec2 p1, glm::vec2 p2, eLineTypes type)
        : mP1(p1), mP2(p2), mType(type)
    {

    }

    static eLineTypes ToType(u16 type);
    static void Render(Renderer& rend, const CollisionLines& lines);

    template<u32 N>
    static bool RayCast(const CollisionLines& lines, const glm::vec2& line1p1, const glm::vec2& line1p2, u32 const (&collisionTypes)[N], Physics::raycast_collision* const collision)
    {
        const CollisionLine* nearestLine = nullptr;
        float nearestCollisionX = 0;
        float nearestCollisionY = 0;
        float nearestDistance = 0.0f;

        for (const std::unique_ptr<CollisionLine>& line : lines)
        {
            bool found = false;
            for (u32 type : collisionTypes)
            {
                if (type == line->mType)
                {
                    found = true;
                    break;
                }
            }

            if (!found) { continue; }

            const float line1p1x = line1p1.x;
            const float line1p1y = line1p1.y;
            const float line1p2x = line1p2.x;
            const float line1p2y = line1p2.y;

            const float line2p1x = line->mP1.x;
            const float line2p1y = line->mP1.y;
            const float line2p2x = line->mP2.x;
            const float line2p2y = line->mP2.y;

            // Get the segments' parameters.
            const float dx12 = line1p2x - line1p1x;
            const float dy12 = line1p2y - line1p1y;
            const float dx34 = line2p2x - line2p1x;
            const float dy34 = line2p2y - line2p1y;

            // Solve for t1 and t2
            const float denominator = (dy12 * dx34 - dx12 * dy34);
            if (denominator == 0.0f) { continue; }

            const float t1 = ((line1p1x - line2p1x) * dy34 + (line2p1y - line1p1y) * dx34) / denominator;
            const float t2 = ((line2p1x - line1p1x) * dy12 + (line1p1y - line2p1y) * dx12) / -denominator;

            // Find the point of intersection.
            const float intersectionX = line1p1x + dx12 * t1;
            const float intersectionY = line1p1y + dy12 * t1;

            // The segments intersect if t1 and t2 are between 0 and 1.
            const bool hasCollided = ((t1 >= 0) && (t1 <= 1) && (t2 >= 0) && (t2 <= 1));

            if (hasCollided)
            {
                const float distance = glm::distance(glm::vec2(line1p1x - intersectionX), glm::vec2(line1p1y - intersectionY));
                if (!nearestLine || distance < nearestDistance)
                {
                    nearestCollisionX = intersectionX;
                    nearestCollisionY = intersectionY;
                    nearestDistance = distance;
                    nearestLine = line.get();
                }
            }
        }

        if (nearestLine)
        {
            if (collision)
            {
                collision->intersection.x = nearestCollisionX;
                collision->intersection.y = nearestCollisionY;
            }
            return true;
        }

        return false;
    }

    struct LineData
    {
        std::string mName;
        ColourU8 mColour;
    };
    static const std::map<eLineTypes, LineData> mData;
};
