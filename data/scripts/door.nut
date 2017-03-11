class Door extends BaseMapObject
{
    static kAnimationResources = 
    [
        // AE door types
        "DoorClosed_Barracks",      // DoorToClose_Barracks have to play this in reverse to get DoorToOpen_Barracks
        "DOOR.BAN_2012_AePc_0",     // mines
        "SHDOOR.BAN_2012_AePc_0",   // wooden mesh door
        "TRAINDOR.BAN_2013_AePc_0", // feeco train door - not aligned!
        "BRDOOR.BAN_2012_AePc_0",   // brewer door, not aligned
        "BWDOOR.BAN_2012_AePc_0",   // bone werkz door
        "STDOOR.BAN_8001_AePc_0",   // blank? 
        "FDDOOR.BAN_2012_AePc_0",   // feeco normal door
        "SVZDOOR.BAN_2012_AePc_0"   // scrab vault door
    ];
    
    constructor(mapObj, rect, stream)
    {
        base.constructor(mapObj, "Door");

        local level = IStream.ReadU16(stream);
        local path = IStream.ReadU16(stream);
        local camera = IStream.ReadU16(stream);

        local scale = IStream.ReadU16(stream);
        if (scale == 1)
        {
            log_warning("Half scale not supported");
        }

        local doorNumber = IStream.ReadU16(stream);
        local id = IStream.ReadU16(stream);
        local targetDoorNumber = IStream.ReadU16(stream);
        local skin = IStream.ReadU16(stream);
        local startOpen = IStream.ReadU16(stream);

        local hubId1 = IStream.ReadU16(stream);
        local hubId2 = IStream.ReadU16(stream);
        local hubId3 = IStream.ReadU16(stream);
        local hubId4 = IStream.ReadU16(stream);
        local hubId5 = IStream.ReadU16(stream);
        local hubId6 = IStream.ReadU16(stream);
        local hubId7 = IStream.ReadU16(stream);
        local hubId8 = IStream.ReadU16(stream);

        local wipeEffect = IStream.ReadU16(stream);
        local xoffset = IStream.ReadU16(stream);
        local yoffset = IStream.ReadU16(stream);

        local wipeXStart = IStream.ReadU16(stream);
        local wipeYStart = IStream.ReadU16(stream);

        local abeFaceLeft = IStream.ReadU16(stream);
        local closeAfterUse = IStream.ReadU16(stream);

        local removeThrowables = IStream.ReadU16(stream);
  
        mBase.mXPos = rect.x + xoffset + 5;
        mBase.mYPos = rect.y + rect.h + yoffset;
    }

    function Update(actions)
    {
        base.SetAnimation("DoorClosed_Barracks");
        base.AnimUpdate();
    }
}
