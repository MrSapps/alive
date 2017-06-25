#include "collisionline.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "reverse_for.hpp"

/*static*/ const std::map<CollisionLine::eLineTypes, CollisionLine::LineData> CollisionLine::mData =
{
    { eFloor,               { "Floor",                  { 222, 237, 211, 255 } } },
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

/*
static glm::vec2 ArrowHeadP1(const Line& line, const glm::vec2& unitVec)
{
    return Line::PointOnLine(unitVec, 10.0f, 20.0f) + line.mP1;
}

static glm::vec2 ArrowHeadP2(const Line& line, const glm::vec2& unitVec)
{
    return Line::PointOnLine(unitVec, 10.0f, -20.0f) + line.mP1;
}
*/

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

static void RenderCircle(AbstractRenderer& rend, const glm::vec2& pos, const ColourU8& colour, f32 radius)
{
    rend.CircleFilled(colour, pos.x, pos.y, radius, 12, AbstractRenderer::eLayers::eEditor + 1, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
}

/*static*/ void CollisionLine::Render(AbstractRenderer& rend, const CollisionLines& lines)
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
            RenderCircle(rend, p1, ColourU8{ 0, 0, 0, 255 }, 5.0f);
            RenderCircle(rend, p1, ColourU8{ 255, 0, 255, 255 }, 2.0f);
        }
    }
}

static void RenderLineAndOptionalArrowHead(AbstractRenderer& rend, const Line& line, f32 width, const ColourU8& fillColour, const ColourU8& outlineColour, bool /*arrowHead*/)
{
    static float thickness = 4.0f;
    static float arrowWidth = 12.0f;
    static float arrowHeight = 18.0f;
    float lineWidth = width;// 4.0f;

    ImVec2 lineP1 = { line.mP1.x, line.mP1.y };
    ImVec2 lineP2 = { line.mP2.x, line.mP2.y };

    glm::vec2 points[7] =
    {
        { lineP1.x - thickness, lineP1.y + arrowHeight },
        { lineP1.x - arrowWidth, lineP1.y + arrowHeight },
        { lineP1.x, lineP1.y },
        { lineP1.x + arrowWidth, lineP1.y + arrowHeight },
        { lineP1.x + thickness, lineP1.y + arrowHeight },
        { lineP2.x + thickness, lineP2.y },
        { lineP2.x - thickness, lineP2.y }
    };

    const f32 angleRads = line.Angle() + glm::radians(90.0f);

    for (u32 i = 0; i < 7 - 2; i++)
    {
        glm::vec2 tmp = points[i];
        tmp.x -= lineP1.x;
        tmp.y -= lineP1.y;
        points[i] = glm::rotate(tmp, angleRads);
        points[i].x += lineP1.x;
        points[i].y += lineP1.y;
    }

    for (int i = 5; i < 7; i++)
    {
        glm::vec2 tmp = points[i];
        tmp.x -= lineP2.x;
        tmp.y -= lineP2.y;
        points[i] = glm::rotate(tmp, angleRads);
        points[i].x += lineP2.x;
        points[i].y += lineP2.y;
    }

    rend.PathBegin();
    for (u32 i = 0; i < 7; i++)
    {
        rend.PathLineTo(points[i].x, points[i].y);
    }
    rend.PathFill(fillColour, AbstractRenderer::eLayers::eEditor, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
    
    rend.PathBegin();
    for (u32 i = 0; i < 7; i++)
    {
        rend.PathLineTo(points[i].x, points[i].y);
    }
    rend.PathStroke(outlineColour, lineWidth, AbstractRenderer::eLayers::eEditor, AbstractRenderer::eNormal, AbstractRenderer::eScreen);
}

/*static*/ void CollisionLine::RenderLine(AbstractRenderer& rend, const CollisionLine& item)
{
    const Line line(rend.WorldToScreen(item.mLine.mP1), rend.WorldToScreen(item.mLine.mP2));
    const auto it = mData.find(item.mType);
    assert(it != std::end(mData));
    RenderLineAndOptionalArrowHead(
        rend,
        line,
        4.0f,
        it->second.mColour,
        (item.mSelected ? ColourU8{ 255, 0, 0, 255 } : ColourU8{ 0, 0, 0, 255 }),
        !item.mLink.mNext);
}

f32 Line::Length() const
{
    return glm::distance(mP1, mP2);
}
