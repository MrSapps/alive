
class Mine extends BaseMapObject
{
    static kAnimationResources = 
    [
        "LANDMINE.BAN_1036_AePc_0"
    ];

    constructor(mapObj, map, rect, stream)
    {
        base.constructor(mapObj, map, "Mine");

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

    function Update(actions)
    {
        base.SetAnimation("LANDMINE.BAN_1036_AePc_0");
        base.AnimUpdate();
    }
}
