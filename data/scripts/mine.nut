
class Mine
{
    mBase = 0;
    mLastAnim = "";

    static kAnimationResources = 
    [
        "LANDMINE.BAN_1036_AePc_0"
    ];

    constructor(mapObj, rect, stream)
    {
        log_info("Mine ctor");

        mBase = mapObj;
        mBase.mName = "Mine";

        mBase.mXPos = rect.x + 10;
        mBase.mYPos = rect.y + 22;
  
        IStream.ReadU32(stream); // Skip unused "num patterns"
        IStream.ReadU32(stream); // Skip unused "patterns"
  
        local scale = IStream.ReadU16(stream);
        if (scale == 1)
        {
            log_warning("Mine background scale not implemented");
        }
    }

    function SetAnimation(anim)
    {
        if (mLastAnim != anim)
        {
            mBase.SetAnimation(anim);
            mLastAnim = anim;
        }
    }

    function Update(actions)
    {
        SetAnimation("LANDMINE.BAN_1036_AePc_0");
        mBase.AnimUpdate();
    }
}
