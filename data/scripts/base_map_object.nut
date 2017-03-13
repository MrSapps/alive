
class BaseMapObject
{
    mBase = "";
    mLastAnim = "";

    constructor(mapObj, name)
    {
        log_info("Constructing object: " + name);
        mBase = mapObj;
        mBase.mName = name;
    }

    function SetAnimation(anim)
    {
        if (mLastAnim != anim)
        {
            mBase.SetAnimation(anim);
            mLastAnim = anim;
        }
    }

    function AnimUpdate()
    {
        return mBase.AnimUpdate();
    }

    function LoadAnimation(name)
    {
        mBase.LoadAnimation(name);
    }

    function FacingRight()
    {
        mBase.FacingRight();
    }

    function FacingLeft()
    {
        mBase.FacingLeft();
    }

    function PlaySoundEffect(soundFx)
    {
        log_warning("TODO: PlaySoundEffect: " + soundFx);
    }

    function FrameNumber()
    {
        return mBase.FrameNumber();
    }

    function NumberOfFrames()
    {
        return mBase.NumberOfFrames();
    }

    function FrameCounter()
    {
        return mBase.FrameCounter();
    }

    function FlipXDirection()
    {
        mBase.FlipXDirection();
    }
}
