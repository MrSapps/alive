class TestObj extends BaseMapObject
{
    function constructor(mapObj, map, x, y)
    {
        base.constructor(mapObj, map, "Child test");
        mBase.mXPos = x + 30;
        mBase.mYPos = y;
        base.LoadAnimation("SLAM.BAN_2020_AePc_1");
        base.SetAnimation("SLAM.BAN_2020_AePc_1");
    }

    function Update(actions)
    {

    }
}

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

        local nativeObj = base.AddChildObject();
        local scriptInst = TestObj(nativeObj, map, mBase.mXPos, mBase.mYPos);
        nativeObj.SetScriptInstance(scriptInst);
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

        local childCount = base.ChildCount();
        for (local i=0; i<childCount; i++)
        {
            local child = base.ChildAt(i);
            child.Update(actions);
        }
    }
}
