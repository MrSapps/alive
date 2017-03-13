
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
        return mBase.FacingRight();
    }

    function FacingLeft()
    {
        return mBase.FacingLeft();
    }

    function PlaySoundEffect(soundFx)
    {
        log_warning("TODO: PlaySoundEffect: " + soundFx);
    }

    function FrameNumber()
    {
        log_info("FrameNumber");
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

    function SnapXToGrid()
    {
        mBase.SnapXToGrid();
    }

    function CellingCollision(dx, dy)
    {
        return mBase.CellingCollision(dx, dy);
    }

    function WallCollision(dx, dy)
    {
        return mBase.WallCollision(dx, dy);
    }

    function FloorCollision()
    {
        return mBase.FloorCollision();
    }

    function SetAnimationFrame(frame)
    {
        mBase.SetAnimationFrame(frame);
    }

    function AnimationComplete()
    {
        return mBase.AnimationComplete();
    }
}
