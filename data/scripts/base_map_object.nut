
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
        mBase.AnimUpdate();
    }

    function LoadAnimation(name)
    {
        mBase.LoadAnimation(name);
    }
}
