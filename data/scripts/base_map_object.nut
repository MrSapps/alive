
class BaseMapObject
{
    mBase = 0;
    mLastAnim = 0;
    mMap = 0;

    constructor(mapObj, map, name)
    {
        log_info("Constructing object: " + name);
        mBase = mapObj;
        mBase.mName = name;
        mMap = map;
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
        return mBase.CellingCollision(mMap, dx, dy);
    }

    function WallCollision(dx, dy)
    {
        return mBase.WallCollision(mMap, dx, dy);
    }

    function FloorCollision()
    {
        return mBase.FloorCollision(mMap);
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
