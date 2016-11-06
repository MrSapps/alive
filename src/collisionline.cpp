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

/*static*/ CollisionLine* CollisionLine::Pick(const CollisionLines& lines, const glm::vec2& pos)
{
    LOG_INFO("Check for line at " << pos.x << "," << pos.y);
    LOG_ERROR("TODO");

    for (const std::unique_ptr<CollisionLine>& item : lines)
    {
        // Check collision with the arrow head triangle, if there is one
        //Physics::IsPointInTriangle(item->mLine.mP1, ? , ? , pos);

        // Check collision with the connection point circle, if there is one
        //Physics::IsPointInCircle(centre, radius, pos);

        // TODO: Adjust P1 depending on if there is an arrow head or not
        // Check collision with the main line segment
        if (Physics::IsPointInThickLine(item->mLine.mP1, item->mLine.mP2, pos, 4.0f))
        {
            LOG_INFO("Item selected");
            return item.get();
        }
    }

    LOG_INFO("Nothing selected");
    return nullptr;
}

/*static*/ void CollisionLine::Render(Renderer& rend, const CollisionLines& lines)
{
    for (const std::unique_ptr<CollisionLine>& item : lines)
    {
        const Line line(rend.WorldToScreen(item->mLine.mP1), rend.WorldToScreen(item->mLine.mP2));

        rend.lineCap(NVG_ROUND);
        rend.LineJoin(NVG_ROUND);
        rend.strokeColor(ColourF32{ 0, 0, 0, 1 });
        rend.strokeWidth(10.0f);
        rend.beginPath();

        rend.moveTo(line.mP1.x, line.mP1.y);
        rend.lineTo(line.mP2.x, line.mP2.y);

        Line unitVec = line.UnitVector();

        if (!item->mLink.mNext)
        {
            // Arrow head
            glm::vec2 pos = unitVec.mP2 * 10.0f;
            pos += glm::rotate(pos, 20.0f);
            pos += line.mP1;

            rend.moveTo(line.mP1.x, line.mP1.y);
            rend.lineTo(pos.x, pos.y);

            glm::vec2 pos2 = unitVec.mP2 * 10.0f;
            pos2 += glm::rotate(pos2, -20.0f);
            pos2 += line.mP1;

            rend.moveTo(line.mP1.x, line.mP1.y);
            rend.lineTo(pos2.x, pos2.y);
        }

        rend.stroke();

        const auto it = mData.find(item->mType);
        assert(it != std::end(mData));

        rend.strokeColor(it->second.mColour.ToColourF32());
        rend.lineCap(NVG_ROUND);
        rend.LineJoin(NVG_ROUND);
        rend.strokeWidth(4.0f);
        rend.beginPath();

        rend.moveTo(line.mP1.x, line.mP1.y);
        rend.lineTo(line.mP2.x, line.mP2.y);

        // TODO: Don't duplicate with above code
        if (!item->mLink.mNext)
        {
            // Arrow head
            glm::vec2 pos = unitVec.mP2 * 10.0f;
            pos += glm::rotate(pos, 20.0f);
            pos += line.mP1;

            rend.moveTo(line.mP1.x, line.mP1.y);
            rend.lineTo(pos.x, pos.y);

            glm::vec2 pos2 = unitVec.mP2 * 10.0f;
            pos2 += glm::rotate(pos2, -20.0f);
            pos2 += line.mP1;

            rend.moveTo(line.mP1.x, line.mP1.y);
            rend.lineTo(pos2.x, pos2.y);
        }
        rend.stroke();
        
        //rend.text(p1.x, p1.y, std::string(it->second.mName).c_str());
    }
    
    // Render would-be connection points
    for (const std::unique_ptr<CollisionLine>& item : lines)
    {
        if (item->mLink.mNext)
        {
            const glm::vec2 p1 = rend.WorldToScreen(item->mLine.mP1);
            //const glm::vec2 p2 = rend.WorldToScreen(item->mP2);

            rend.strokeWidth(4.0f);
            rend.strokeColor(ColourF32{ 0, 0, 0, 1 });
            rend.beginPath();
            rend.circle(p1.x, p1.y, 5.0f);
            rend.stroke();

            rend.strokeWidth(4.0f);
            rend.strokeColor(ColourF32{ 1, 0, 1, 1 });
            rend.beginPath();
            rend.circle(p1.x, p1.y, 2.0f);
            rend.stroke();

        }
    }
}

