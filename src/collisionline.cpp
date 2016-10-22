#include "collisionline.hpp"
#include "proxy_nanovg.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>

/*static*/ const std::map<CollisionLine::eLineTypes, CollisionLine::LineData> CollisionLine::mData =
{
    { eFloor,               { "Floor",                  { 255, 0, 0, 255 } } },
    { eWallLeft,            { "Wall left",              { 0, 0, 255, 255 } } },
    { eWallRight,           { "Wall right",             { 0, 100, 255, 255 } } },
    { eCeiling,             { "Ceiling",                { 255, 100, 0, 255 } } },
    { eBackGroundFloor,     { "Bg floor",               { 255, 100, 0, 255 } } },
    { eBackGroundWallLeft,  { "Bg wall left",           { 100, 100, 255, 255 } } },
    { eBackGroundWallRight, { "Bg wall right",          { 0, 255, 255, 255 } } },
    { eBackGroundCeiling,   { "Bg ceiling",             { 255, 100, 0, 255 } } },
    { eTrackLine,           { "Track line",             { 255, 255, 0, 255 } } },
    { eArt,                 { "Art line",               { 255, 255, 255, 255 } } },
    { eBulletWall,          { "Bullet wall",            { 255, 255, 0, 255 } } },
    { eMineCarFloor,        { "Minecar floor",          { 255, 255, 255, 255 } } },
    { eMineCarWall,         { "Minecar wall",           { 255, 0,   255, 255 } } },
    { eMineCarCeiling,      { "Minecar ceiling",        { 255, 0, 255, 255 } } },
    { eFlyingSligCeiling,   { "Flying slig ceiling",    { 255, 0, 255, 255 } } },
    { eUnknown,             { "Unknown",                { 255, 0, 255, 255 } } }
};

/*static*/ CollisionLine::eLineTypes CollisionLine::ToType(u16 type)
{
    switch (type)
    {
    case eFloor: return eFloor;
    case eWallLeft: return eWallLeft;
    case eWallRight: return eWallRight;
    case eCeiling: return eCeiling;
    case eBackGroundFloor: return eBackGroundFloor;
    case eBackGroundWallLeft: return eBackGroundWallLeft;
    case eBackGroundWallRight: return eBackGroundWallRight;
    case eBackGroundCeiling: return eBackGroundCeiling;
    case eTrackLine: return eTrackLine;
    case eArt: return eArt;
    case eBulletWall: return eBulletWall;
    case eMineCarFloor: return eMineCarFloor;
    case eMineCarWall: return eMineCarWall;
    case eMineCarCeiling: return eMineCarCeiling;
    case eFlyingSligCeiling: return eFlyingSligCeiling;
    }
    LOG_ERROR("Unknown collision type: " << type);
    return eUnknown;
}

glm::vec2 normalize(const glm::vec2& vec)
{
    if (vec.x != 0.0f || vec.y != 0.0f)
    {
        return glm::normalize(vec);
    }
    return vec;
}

/*static*/ void CollisionLine::Render(Renderer& rend, const CollisionLines& lines)
{
    for (const std::unique_ptr<CollisionLine>& item : lines)
    {
        const glm::vec2 p1 = rend.WorldToScreen(item->mP1);
        const glm::vec2 p2 = rend.WorldToScreen(item->mP2);

        {
            rend.lineCap(NVG_ROUND);
            rend.LineJoin(NVG_ROUND);
            rend.strokeColor(ColourF32{ 0, 0, 0, 1 });
            rend.strokeWidth(10.0f);
            rend.beginPath();
            rend.moveTo(p1.x, p1.y);
            rend.lineTo(p2.x, p2.y);

            if (!item->mLink.mNext)
            {
                // Arrow head   
                glm::vec2 vx = p2 - p1;
                vx = glm::normalize(vx);

                glm::vec2 pos
                {   p1.x + (vx.x * 10.0f),
                    p1.y + (vx.y * 10.0f) 
                };
                pos += glm::rotate(pos - p1, 20.0f);

                rend.moveTo(p1.x, p1.y);
                rend.lineTo(pos.x, pos.y);

                glm::vec2 pos2
                {   p1.x + (vx.x * 10.0f),
                    p1.y + (vx.y * 10.0f) 
                };
                pos2 += glm::rotate(pos2 - p1, -20.0f);
                rend.moveTo(p1.x, p1.y);
                rend.lineTo(pos2.x, pos2.y);
            }

            rend.stroke();
        }

        const auto it = mData.find(item->mType);
        assert(it != std::end(mData));

        rend.strokeColor(it->second.mColour.ToColourF32());
        rend.lineCap(NVG_ROUND);
        rend.LineJoin(NVG_ROUND);
        rend.strokeWidth(4.0f);
        rend.beginPath();
        rend.moveTo(p1.x, p1.y);
        rend.lineTo(p2.x, p2.y);

        if (!item->mLink.mNext)
        {
            // Arrow head   
            glm::vec2 vx = p2 - p1;
            vx = glm::normalize(vx);

            glm::vec2 pos
            {
                p1.x + (vx.x * 10.0f),
                p1.y + (vx.y * 10.0f)
            };
            pos += glm::rotate(pos - p1, 20.0f);

            rend.moveTo(p1.x, p1.y);
            rend.lineTo(pos.x, pos.y);

            glm::vec2 pos2
            {
                p1.x + (vx.x * 10.0f),
                p1.y + (vx.y * 10.0f)
            };
            pos2 += glm::rotate(pos2 - p1, -20.0f);
            rend.moveTo(p1.x, p1.y);
            rend.lineTo(pos2.x, pos2.y);
        }
        rend.stroke();

        //rend.text(p1.x, p1.y, std::string(it->second.mName).c_str());
    }
    /*
    // Render would-be connection points
    for (const std::unique_ptr<CollisionLine>& item : lines)
    {
    const glm::vec2 p1 = rend.WorldToScreen(item->mP1);
    const glm::vec2 p2 = rend.WorldToScreen(item->mP2);


    rend.strokeColor(ColourF32{ 0, 0, 0, 1 });
    rend.strokeWidth(10.0f + 4.0f);

    rend.beginPath();
    rend.circle(p1.x, p1.y, 1.0f);
    rend.stroke();

    const auto it = mData.find(item->mType);
    assert(it != std::end(mData));

    rend.strokeColor(it->second.mColour.ToColourF32());

    rend.strokeWidth(4.0f+4.0f);

    rend.beginPath();
    rend.circle(p1.x, p1.y, 1.0f);
    rend.stroke();
    }*/
}
