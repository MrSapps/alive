
class ElectricWall
{
    mScale = 0;
    mBase = 0;
    mId = 0;
    mEnabled = 0;
    mLastAnim = "";

    static kAnimationResources = 
    [
        "ELECWALL.BAN_6000_AePc_0"
    ];

    constructor(mapObj, rect, stream)
    {
        mBase = mapObj;

        mBase.mName = "ElectricWall";

        log_info("ElectricWall ctor");

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
        SetAnimation("ELECWALL.BAN_6000_AePc_0");
        mBase.AnimUpdate();
    }
}
