
class ElectricWall extends BaseMapObject
{
    mScale = 0;
    mId = 0;
    mEnabled = 0;

    static kAnimationResources = 
    [
        "ELECWALL.BAN_6000_AePc_0"
    ];

    constructor(mapObj, map, rect, stream)
    {
        base.constructor(mapObj, map, "ElectricWall");

        mBase.mXPos = rect.x;
        mBase.mYPos = rect.y;

        mScale = IStream.ReadU16(stream);
        if (mScale == 1)
        {
            log_warning("ElectricWall scale not implemented");
        }
        
        mId = IStream.ReadU16(stream);
        mEnabled = IStream.ReadU16(stream);
   
        log_info("WALL ID IS " + mId + " START ON IS " + mEnabled + " SCALE IS " + mScale);
    }

    function Update(actions)
    {
        base.SetAnimation("ELECWALL.BAN_6000_AePc_0");
        base.AnimUpdate();
    }
}
