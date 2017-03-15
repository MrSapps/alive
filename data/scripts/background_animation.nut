class BackgroundAnimation extends BaseMapObject
{
    constructor(mapObj, map, rect, stream)
    {
        base.constructor(mapObj, map, "BackgroundAnimation");

        local animId = IStream.ReadU32(stream);
        local animName = "";
        switch(animId)
        {
        case 1201:
            animName = "BAP01C06.CAM_1201_AePc_0";
            mBase.mXPos = rect.x + 5;
            mBase.mYPos = rect.y + 3;
            break;

        case 1202:
            animName = "FARTFAN.BAN_1202_AePc_0";
            break;

        default:
            animName = "AbeStandSpeak1";
        }

        log_info("BackgroundAnimation ctor id is " + animId + " animation is " + animName);
        base.LoadAnimation(animName);
        base.SetAnimation(animName);
    }

    function Update(actions)
    {
        base.AnimUpdate();
    }
}
