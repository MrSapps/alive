
class Switch extends BaseMapObject
{
    mScale = 0;
    mId = 0;

    static kAnimationResources =
    [
        "SwitchIdle",
        "SwitchActivateLeft",
        "SwitchDeactivateLeft",
        "SwitchActivateRight",
        "SwitchDeactivateRight"
    ];

    static kSoundResources =
    [
        "FX_LEVER"
    ];

    constructor(mapObj, rect, stream)
    {
        base.constructor(mapObj, "Switch");

        mBase.mXPos = rect.x + 37;
        mBase.mYPos = rect.y + rect.h - 5;
  
        mScale = IStream.ReadU32(stream);
        if (mScale == 1)
        {
            log_warning("Half scale not supported")
        }
  
        local onSound = IStream.ReadU16(stream);
        local offSound = IStream.ReadU16(stream);
        local soundDirection = IStream.ReadU16(stream);

        // The ID of the object that this switch will apply "targetAction" to
        mId = IStream.ReadU16(stream)
    }

    function Activate(direction)
    {
        log_info("TODO: Activate");
        // TODO: Activate objects with self.mId
    }

    // TODO: Make this work
    function Update(actions)
    {
        base.SetAnimation("SwitchIdle");
        // SwitchActivateLeft
        // SwitchDeactivateLeft
        // SwitchActivateRight
        // SwitchDeactivateRight
        // PlaySoundEffect("FX_LEVER");
        base.AnimUpdate();
    }
}
