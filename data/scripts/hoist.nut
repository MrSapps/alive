class Hoist extends BaseMapObject
{
    mHoistType = 0;
    mEdgeType = 0;
    mId = 0;
    mScale = 0;

    function constructor(mapObj, rect, stream)
    {
        base.constructor(mapObj, "Hoist");

        mBase.mXPos = rect.x;
        mBase.mYPos = rect.y;

        mHoistType = IStream.ReadU16(stream);
        mEdgeType = IStream.ReadU16(stream);
        mId = IStream.ReadU16(stream);
        mScale = IStream.ReadU16(stream);
    }

    function Update(actions)
    {

    }
}
