class SlamDoor extends BaseMapObject
{
    mClosed = 0;
    mScale = 0;
    mId = 0;
    mInverted = 0;
    mDelete = 0;

    static kAnimationResources = 
    [
        "SLAM.BAN_2020_AePc_1"
    ];

    function constructor(mapObj, map, rect, stream)
    {
        base.constructor(mapObj, map, "SlamDoor");

        mBase.mXPos = rect.x + 13;
        mBase.mYPos = rect.y + 18;

        mClosed = IStream.ReadU16(stream);
        mScale = IStream.ReadU16(stream);
        mId = IStream.ReadU16(stream);
        mInverted = IStream.ReadU16(stream);
        mDelete = IStream.ReadU32(stream);
    }

    function Update(actions)
    {
        if (mClosed)
        {
            base.SetAnimation("SLAM.BAN_2020_AePc_1");
            base.AnimUpdate();
        }
        else
        {
            base.SetAnimation("");
        }
    }
}
