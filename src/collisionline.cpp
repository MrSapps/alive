#include "collisionline.hpp"
#include "proxy_nanovg.h"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "reverse_for.hpp"

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

bool CollisionLine::SetSelected(bool selected)
{
    mSelected = selected;
    return selected;
}

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

static glm::vec2 ArrowHeadP1(const Line& line, const glm::vec2& unitVec)
{
    return Line::PointOnLine(unitVec, 10.0f, 20.0f) + line.mP1;
}

static glm::vec2 ArrowHeadP2(const Line& line, const glm::vec2& unitVec)
{
    return Line::PointOnLine(unitVec, 10.0f, -20.0f) + line.mP1;
}

/*static*/ s32 CollisionLine::Pick(const CollisionLines& lines, const glm::vec2& pos, float lineScale)
{
    /*
    s32 idx = static_cast<s32>(lines.size());
    for (const std::unique_ptr<CollisionLine>& item : reverse_for(lines))
    {
        idx--;
        if (item->mLink.mNext)
        {
            // TODO: Check this actually works
            // Check collision with the connection point circle, if there is one
            if (Physics::IsPointInCircle(item->mLine.mP1, 5.0f * lineScale, pos))
            {
                return idx;
            }
        }
    }
    */

    s32 idx = static_cast<s32>(lines.size());
    for (const std::unique_ptr<CollisionLine>& item : reverse_for(lines))
    {
        idx--;

        /*
        // Check collision with the arrow head triangle, if there is one
        if (!item->mLink.mNext)
        {
            const Line unitVec = item->mLine.UnitVector();
            const glm::vec2 arrowP1 = ArrowHeadP1(item->mLine, unitVec.mP2);
            const glm::vec2 arrowP2 = ArrowHeadP2(item->mLine, unitVec.mP2);
            // TODO: This isn't correct as the tri points don't take the line width into account
            if (Physics::IsPointInTriangle(item->mLine.mP1, arrowP1, arrowP2, pos))
            {
                return idx;
            }
        }
        */

        // TODO: Adjust P1 depending on if there is an arrow head or not
        // Check collision with the main line segment
        if (Physics::IsPointInThickLine(item->mLine.mP1, item->mLine.mP2, pos, 10.0f * lineScale))
        {
            return idx;
        }

        /*
        if (Physics::IsPointInCircle(item->mLine.mP2, 5.0f * lineScale, pos))
        {
           // return idx;
        }

        if (Physics::IsPointInCircle(item->mLine.mP1, 6.0f * lineScale, pos))
        {
          //  return idx;
        }
        */
    }
    return -1;
}

static void RenderCircle(Renderer& rend, const glm::vec2& pos, const ColourF32& colour, f32 radius)
{
    rend.strokeWidth(4.0f);
    rend.strokeColor(colour);
    rend.beginPath();
    rend.circle(pos.x, pos.y, radius);
    rend.stroke();
}

/*static*/ void CollisionLine::Render(Renderer& rend, const CollisionLines& lines)
{
    for (const std::unique_ptr<CollisionLine>& item : lines)
    {
        RenderLine(rend, *item);
    }

    // Render would-be connection points - always on top of lines
    for (const std::unique_ptr<CollisionLine>& item : lines)
    {
        if (item->mLink.mNext)
        {
            const glm::vec2 p1 = rend.WorldToScreen(item->mLine.mP1);
            RenderCircle(rend, p1, ColourF32{ 0, 0, 0, 1 }, 5.0f);
            RenderCircle(rend, p1, ColourF32{ 1, 0, 1, 1 }, 2.0f);
        }
    }
}

static void RenderLineAndOptionalArrowHead(Renderer& rend, const Line& line, f32 width, const ColourF32& colour, bool arrowHead)
{
    rend.lineCap(NVG_ROUND);
    rend.LineJoin(NVG_ROUND);

    rend.strokeColor(colour);
    rend.strokeWidth(width);
    rend.beginPath();

    rend.moveTo(line.mP1.x, line.mP1.y);
    rend.lineTo(line.mP2.x, line.mP2.y);

    if (arrowHead)
    {
        const Line unitVec = line.UnitVector();
        const glm::vec2 arrowP1 = ArrowHeadP1(line, unitVec.mP2);
        const glm::vec2 arrowP2 = ArrowHeadP2(line, unitVec.mP2);

        rend.moveTo(line.mP1.x, line.mP1.y);
        rend.lineTo(arrowP1.x, arrowP1.y);
        rend.moveTo(line.mP1.x, line.mP1.y);
        rend.lineTo(arrowP2.x, arrowP2.y);
    }

    rend.stroke();
}

/*static*/ void CollisionLine::RenderLine(Renderer& rend, const CollisionLine& item)
{
    const Line line(rend.WorldToScreen(item.mLine.mP1), rend.WorldToScreen(item.mLine.mP2));

    // Draw the big outline
    RenderLineAndOptionalArrowHead(
        rend, 
        line, 
        10.0f, 
        (item.mSelected ? ColourF32{ 1, 0, 0, 1 } : ColourF32{ 0, 0, 0, 1 }), 
        !item.mLink.mNext);

    // Over draw the middle with the "fill" colour
    const auto it = mData.find(item.mType);
    assert(it != std::end(mData));
    RenderLineAndOptionalArrowHead(
        rend,
        line,
        4.0f,
        it->second.mColour.ToColourF32(),
        !item.mLink.mNext);
}

f32 Line::Length() const
{
    return glm::distance(mP1, mP2);
}
