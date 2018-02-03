class Abe extends BaseMapObject
{
    static kAnimationResources =
    [
        "AbeWalkToStand",
        "AbeWalkToStandMidGrid",
        "AbeWalkingToRunning",
        "AbeWalkingToRunningMidGrid",
        "AbeWalkingToSneaking",
        "AbeWalkingToSneakingMidGrid",
        "AbeStandToRun",
        "AbeRunningToSkidTurn",
        "AbeRunningTurnAround",
        "AbeRunningTurnAroundToWalk",
        "AbeRunningToRoll",
        "AbeRunningToJump",
        "AbeRunningJumpInAir",
        "AbeLandToRunning",
        "AbeLandToWalking",
        "AbeFallingToLand",
        "RunToSkidStop",
        "AbeRunningSkidStop",
        "AbeRunningToWalk",
        "AbeRunningToWalkingMidGrid",
        "AbeStandToSneak",
        "AbeSneakToStand",
        "AbeSneakToStandMidGrid",
        "AbeSneakingToWalking",
        "AbeSneakingToWalkingMidGrid",
        "AbeStandPushWall",
        "AbeHitGroundToStand",
        "AbeStandToWalk",
        "AbeStandToCrouch",
        "AbeCrouchToStand",
        "AbeStandTurnAround",
        "AbeStandTurnAroundToRunning",
        "AbeCrouchTurnAround",
        "AbeCrouchToRoll",
        "AbeStandSpeak1",
        "AbeStandSpeak2",
        "AbeStandSpeak3",
        "AbeStandingSpeak4",
        "AbeStandSpeak5",
        "AbeCrouchSpeak1",
        "AbeCrouchSpeak2",
        "AbeStandIdle",
        "AbeCrouchIdle",
        "AbeStandToHop",
        "AbeHopping",
        "AbeHoppingToStand",
        "AbeHoistDangling",
        "AbeHoistPullSelfUp",
        "AbeStandToJump",
        "AbeJumpUpFalling",
        "AbeHitGroundToStand",
        "AbeWalking",
        "AbeRunning",
        "AbeSneaking",
        "AbeStandToFallingFromTrapDoor",
        "AbeHoistDropDown",
        "AbeRolling"
    ];

    function InputNotSameAsDirection()
    {
        return (Actions.Left(mInput.IsHeld)  && base.FacingRight())
            || (Actions.Right(mInput.IsHeld) && base.FacingLeft());
    }

    function InputSameAsDirection()
    {
        return (Actions.Left(mInput.IsHeld)  && base.FacingLeft())
            || (Actions.Right(mInput.IsHeld) && base.FacingRight());
    }

    function SetAnimationFrame(frame)
    {
        log_info("Force animation frame to " + frame);
        base.SetAnimationFrame(frame);
    }

    function FlipXDirection() { base.FlipXDirection(); }
    function FrameIs(frame) { return base.FrameNumber() == frame && base.FrameCounter() == 0; }
    function SetXSpeed(speed) { mXSpeed = speed; }
    function SetYSpeed(speed) { mYSpeed = speed; }
    function SetXVelocity(velocity) { mXVelocity = velocity; }
    function SetYVelocity(velocity) { mYVelocity = velocity; }

    function SnapXToGrid()
    {
        // TODO: This breaks sometimes, stand idle, press inverse direction and take 1 step to repro issue
        base.SnapXToGrid();
    }

    function ToStandCommon(anim)
    {
        SetXSpeed(2.777771);
        SetXVelocity(0);

        PlayAnimation(anim,
        {
            onFrame = function(obj)
            {
                if (obj.FrameIs(2))
                {
                    obj.PlaySoundEffect("MOVEMENT_MUD_STEP");
                }
            }
        });
        //SnapXToGrid();
        return GoTo(Stand);
    }

    function ToStand() { return ToStandCommon("AbeWalkToStand"); }
    function ToStand2() { return ToStandCommon("AbeWalkToStandMidGrid"); }

    function WalkToRunCommon(anim)
    {
        PlayAnimation(anim);
        return GoTo(Run);
    }

    function WalkToRun() { return WalkToRunCommon("AbeWalkingToRunning"); }
    function WalkToRun2() { return WalkToRunCommon("AbeWalkingToRunningMidGrid"); }

    function WalkToSneakCommon(anim)
    {
        SetXSpeed(2.777771);
        SetXVelocity(0);
        PlayAnimation(anim);
        return GoTo(Sneak);
    }

    function WalkToSneak() { return WalkToSneakCommon("AbeWalkingToSneaking"); }
    function WalkToSneak2() { return WalkToSneakCommon("AbeWalkingToSneakingMidGrid"); }

    function StandToRun()
    {
        SetXSpeed(6.25);
        SetXVelocity(0);
        PlayAnimation("AbeStandToRun");
        return GoTo(Run);
    }

    function RunToSkidTurnAround()
    {
        SetXSpeed(6.25);
        SetXVelocity(0.375);
        PlayAnimation("AbeRunningToSkidTurn",
        {
            endFrame = 15,
            onFrame = function(obj)
            {
                if (obj.FrameIs(14))
                {
                    log_trace("Handle last frame of AbeRunningToSkidTurn");
                    obj.SetXVelocity(0);
                    obj.SnapXToGrid();
                }
            }
        });

        if (Actions.Run(mInput.IsHeld))
        {
            SetXSpeed(6.25);
            SetXVelocity(0);
            // TODO: Probably better to have a FlipSpriteX and FlipDirectionX instead?
            mInvertX = true;
            PlayAnimation("AbeRunningTurnAround");
            mInvertX = false;
            FlipXDirection();
            //SnapXToGrid();

            return GoTo(Run);
        }
        else
        {
            SetXSpeed(2.777771);
            SetXVelocity(0);
            mInvertX = true;
            PlayAnimation("AbeRunningTurnAroundToWalk");
            mInvertX = false;
            FlipXDirection();
            return GoTo(Walk);
        }

    }

    function RunToRoll()
    {
        SetXVelocity(0);
        SetXSpeed(6.25);
        PlayAnimation("AbeRunningToRoll");
        return GoTo(Roll);
    }

    function RunToJump()
    {
        PlayAnimation("AbeRunningToJump",
        {
            onFrame = function(obj)
            {
                if (obj.FrameIs(2))
                {
                    obj.SetYVelocity(9.6);
                }
            }
        });

        SetYVelocity(-1.8);
        SetXSpeed(7.6);
        PlayAnimation("AbeRunningJumpInAir",
        {
            onFrame = function(obj)
            {
                if (obj.FrameIs(10))
                {
                    obj.SetYVelocity(1.8);
                    obj.SetXSpeed(4.9);
                }
            }
        });

        SetYSpeed(0);
        SetYVelocity(0);
        SnapXToGrid();

        if (InputSameAsDirection())
        {
            if (Actions.Run(mInput.IsHeld))
            {
                SetXSpeed(6.250000);
                PlayAnimation("AbeLandToRunning");
                return GoTo(Abe.Run);
            }
            else
            {
                SetXSpeed(2.777771);
                PlayAnimation("AbeLandToWalking");
                return GoTo(Abe.Walk);
            }
        }
        else
        {
            PlayAnimation("AbeFallingToLand");
            return RunToSkidStop();
        }
    }

    function Run()
    {
        if (FrameIs(0) && base.AnimationComplete() == false)
        {
            // SetXSpeed(12.5);
            // the real game uses 12.5 sometimes depending on the previous state, but 6.25
            // seems to always look correct..
            SetXSpeed(6.25);
        }
        else
        {
            SetXSpeed(6.25);
        }

        if (FrameIs(0+1) || FrameIs(8+1))
        {
            SnapXToGrid();
            if (InputSameAsDirection() && Actions.Run(mInput.IsHeld) && Actions.Jump(mInput.IsHeld))
            {
                return RunToJump();
            }
        }

        if (FrameIs(4+1) || FrameIs(12+1))
        {
            SnapXToGrid();
            if (InputNotSameAsDirection())
            {
                return RunToSkidTurnAround();
            }
            else if (InputSameAsDirection())
            {
                base.PlaySoundEffect("MOVEMENT_MUD_STEP"); // TODO: Always play fx?
                if (Actions.Run(mInput.IsHeld) == false)
                {
                    if (FrameIs(4+1))
                    {
                        return RunToWalk();
                    }
                    else
                    {
                        return RunToWalk2();
                    }
                }
                else if (Actions.Jump(mInput.IsHeld))
                {
                    return RunToJump();
                }
                else if (Actions.RollOrFart(mInput.IsHeld))
                {
                    return RunToRoll();
                }
            }
            else
            {
                return RunToSkidStop();
            }
        }
    }

    function RunToSkidStop()
    {
        SetXVelocity(0.375);
        PlayAnimation("AbeRunningSkidStop",
        {
            endFrame = 15,
            onFrame = function(obj)
            {
                if (obj.FrameIs(14))
                {
                    log_trace("Handle last frame of AbeRunningSkidStop");
                    obj.SetXVelocity(0);
                    obj.SnapXToGrid();
                }
            }
        });

        return GoTo(Stand);
    }

    function RunToWalkCommon(anim)
    {
        SetXSpeed(2.777771);
        SetXVelocity(0);
        PlayAnimation(anim);
        return GoTo(Walk);
    }

    function RunToWalk() { return RunToWalkCommon("AbeRunningToWalk"); }
    function RunToWalk2() { return RunToWalkCommon("AbeRunningToWalkingMidGrid"); }

    function StandToSneak()
    {
        SetXSpeed(2.5);
        PlayAnimation("AbeStandToSneak");
        return GoTo(Sneak);
    }

    function Sneak()
    {
        if (InputSameAsDirection())
        {
            if (FrameIs(6+1) || FrameIs(16+1))
            {
                SnapXToGrid();

                local collision = WillStepIntoWall();
                if (collision) { return collision; }

                base.PlaySoundEffect("MOVEMENT_MUD_STEP");
                if (Actions.Sneak(mInput.IsHeld) == false)
                {
                    if (FrameIs(6+1)) { return SneakToWalk(); } else { return SneakToWalk2(); }
                }
            }
        }
        else
        {
            if (FrameIs(3+1) || FrameIs(13+1))
            {
                if (FrameIs(3+1)) { return AbeSneakToStand(); } else { return AbeSneakToStand2(); }
            }
        }
    }

    function AbeSneakToStandCommon(anim)
    {
        PlayAnimation(anim);
        return GoTo(Stand);
    }

    function AbeSneakToStand() { return AbeSneakToStandCommon("AbeSneakToStand"); }
    function AbeSneakToStand2() { return AbeSneakToStandCommon("AbeSneakToStandMidGrid"); }

    function SneakToWalkCommon(anim)
    {
        PlayAnimation(anim);
        return GoTo(Walk);
    }

    function SneakToWalk() { return SneakToWalkCommon("AbeSneakingToWalking"); }
    function SneakToWalk2() { return SneakToWalkCommon("AbeSneakingToWalkingMidGrid"); }

    function CalculateYSpeed()
    {
        local newYSpeed = mYSpeed - mYVelocity;
        if (newYSpeed > 20)
        {
            newYSpeed = 20;
        }
        return newYSpeed;
    }

    function ApplyMovement()
    {
        if (mXSpeed > 0)
        {
            mXSpeed = mXSpeed - mXVelocity;
            if (base.FacingLeft())
            {
                if (mInvertX)
                {
                    mBase.mXPos = mBase.mXPos + mXSpeed;
                }
                else
                {
                    mBase.mXPos = mBase.mXPos - mXSpeed;
                }
            }
            else
            {
                if (mInvertX)
                {
                    mBase.mXPos = mBase.mXPos - mXSpeed;
                }
                else
                {
                    mBase.mXPos = mBase.mXPos + mXSpeed;
                }
            }
        }

        //if (mYSpeed < 0)
        //{
            //log_error("YSpeed from " + mYSpeed + " to " + mYSpeed - mYVelocity);
            mYSpeed = CalculateYSpeed();
            mBase.mYPos += mYSpeed;
        //}

    }

    function WillStepIntoWall()
    {
        // Blocked by wall at head height?
        if (base.WallCollision(25, -50))
        {
            SetXSpeed(0);
            SetXVelocity(0);

            // Blocked at knee height?
            if (base.WallCollision(25, -20))
            {
                // Way to get through
                PlayAnimation("AbeStandPushWall");
                return GoTo(Stand);
            }
            else
            {
                // Goto crouch so we can roll through
                return StandToCrouch();
            }
        }
    }

    function CollisionWithFloor()
    {
        return base.FloorCollision();
    }

    // TODO Merge with StandFalling
    function StandFalling2()
    {
        log_info("StandFalling2");

        // TODO: Fix XVelocity, only correct for walking?
        local ret = CollisionWithFloor();

        if (ret.collision == false || (ret.collision == true && ret.y > mBase.mYPos))
        {
            log_info("Not touching floor or floor is far away " + ret.y + " VS " + mBase.mYPos + " distance " + ret.distance);
            local ySpeed = CalculateYSpeed();
            local expectedYPos = mBase.mYPos + ySpeed;
            if (ret.collision == true && expectedYPos > ret.y)
            {
                log_info("going to pass through the floor, glue to it!");
                mBase.mYPos = ret.y;
                SetXSpeed(0);
                SetXVelocity(0);
                SetYSpeed(0);
                SetYVelocity(0);
                SnapXToGrid();
                PlayAnimation("AbeHitGroundToStand");
                return GoTo(Stand);
            }
        }
        else
        {
            log_info("hit floor or gone through it");
            SetXSpeed(0);
            SetXVelocity(0);
            SetYSpeed(0);
            SetYVelocity(0);
            SnapXToGrid();
            PlayAnimation("AbeHitGroundToStand");
            return GoTo(Stand);
        }
    }

    // TODO
    function StandFalling()
    {
        // TODO: Fix XVelocity, only correct for walking?
        local ret = CollisionWithFloor();
        if (ret.collision == false || (ret.collision == true && ret.y > mBase.mYPos))
        {
            log_info("Not touching floor or floor is far away " + ret.y + " VS " + mBase.mYPos + " distance " + ret.distance);
            local ySpeed = CalculateYSpeed();
            local expectedYPos = mBase.mYPos + ySpeed;
            if (ret.collision == true && expectedYPos > ret.y)
            {
                log_info("going to pass through the floor, glue to it!");
                mBase.mYPos = ret.y;
                SetXSpeed(0);
                SetXVelocity(0);
                SetYSpeed(0);
                SetYVelocity(0);
                SnapXToGrid();
                PlayAnimation("AbeHitGroundToStand");
                return GoTo(Stand);
            }
        }
        else
        {
            log_info("hit floor or gone through it");
            SetXSpeed(0);
            SetXVelocity(0);
            SetYSpeed(0);
            SetYVelocity(0);
            SnapXToGrid();
            PlayAnimation("AbeHitGroundToStand");
            return GoTo(Stand);
        }
    }

    function Walk()
    {
        local ret = CollisionWithFloor();
        if (ret.collision == false || (ret.collision == true && ret.y > mBase.mYPos))
        {
            log_info("Not on the floor " + ret.y + " VS " + mBase.mYPos);
            return GoTo(StandFalling);
        }

        if (FrameIs(5+1) || FrameIs(14+1))
        {
            base.PlaySoundEffect("MOVEMENT_MUD_STEP");
            SnapXToGrid();

            local collision = WillStepIntoWall();
            if (collision) { return collision; }

            if (InputSameAsDirection() == true)
            {
                if (Actions.Run(mInput.IsHeld))
                {
                    if (FrameIs(5+1)) { return WalkToRun(); } else { return WalkToRun2(); }
                }
                else if (Actions.Sneak(mInput.IsHeld))
                {
                    if (FrameIs(5+1)) { return WalkToSneak(); } else { return WalkToSneak2(); }
                }
            }
        }
        else if (FrameIs(2+1) || FrameIs(11+1))
        {
            if (InputSameAsDirection() == false)
            {
                if (FrameIs(2+1)) { return ToStand(); } else { return ToStand2(); }
            }
        }
    }

    function PlayAnimation(name, params = null)
    {
        base.SetAnimation(name);
        if ("startFrame" in params)
        {
            log_info("Start animation at frame " + params.startFrame);
            SetAnimationFrame(params.startFrame-1); // first update will frame+1
        }
        else
        {
            log_info("Start animation at beginning");
        }

        local stop = false;
        while (true)
        {

            local frameChanged = base.AnimUpdate();

            base.FrameNumber();

            if (frameChanged)
            {
                if (("endFrame" in params && base.FrameNumber() == params.endFrame+1) || (base.FrameNumber() == base.NumberOfFrames()-1))
                {
                    log_info("Wait for complete done at frame " + base.FrameNumber());
                    stop = true;
                }

                if ("onFrame" in params)
                {
                    if (params.onFrame(this))
                    {
                        log_info("Request to stop at frame " + base.FrameNumber());
                        stop = true;
                    }
                }
                ApplyMovement();
            }
            else
            {
                log_info("No change frame no:" + base.FrameNumber() + " num frames: " + base.NumberOfFrames());
            }

            ::suspend();

            if (stop)
            {
                log_info("-PlayAnimation (callback): " + name);
                return;
            }
        }
    }

    function GoTo(func)
    {
        log_info("Goto " + func);

        local data = mAnims[func];
        mData = { mFunc = func, Animation = data.name };
        if ("xspeed" in data)
        {
            SetXSpeed(data.xspeed);
        }
        SetXVelocity(data.xvel);

        if ("yvel" in data)
        {
            SetYVelocity(data.yvel);
        }

        if (mData.Animation == null)
        {
            log_error("An animation mapping is missing!");
            mData.Animation = mLastAnimationName;
        }

        log_info("Goto sets " + mData.Animation);

        return true;
    }

    function StandToWalk()
    {
        SetXSpeed(2.777771);
        SetXVelocity(0);
        PlayAnimation("AbeStandToWalk");
        return GoTo(Walk);
    }

    function StandToCrouch()
    {
        PlayAnimation("AbeStandToCrouch");
        return GoTo(Crouch);
    }

    function CrouchToStand()
    {
        PlayAnimation("AbeCrouchToStand");
        return GoTo(Stand);
    }

    function StandTurnAround()
    {
        log_info("StandTurnAround");

        base.PlaySoundEffect("GRAVEL_SMALL"); // TODO: Add to json
        local toRun = false;

        // stop at frame 3 if we want to go to running
        PlayAnimation("AbeStandTurnAround",
        {
            onFrame = function(obj)
            {
                if (obj.FrameIs(3) && Actions.Run(obj.mInput.IsHeld))
                {
                    toRun = true;
                    return true;
                }
            }
        });

        FlipXDirection()
        if (toRun)
        {
            SetXSpeed(6.25);
            SetXVelocity(0);
            PlayAnimation("AbeStandTurnAroundToRunning");
            return GoTo(Run);
        }
        return GoTo(Stand);
    }

    function CrouchTurnAround()
    {
        PlayAnimation("AbeCrouchTurnAround");
        FlipXDirection();
        return GoTo(Crouch);
    }

    function CrouchToRoll()
    {
        SetXSpeed(6.25);
        SetXVelocity(0);
        PlayAnimation("AbeCrouchToRoll");
        return GoTo(Roll);
    }

    function Roll()
    {
        if (FrameIs(0+1) || FrameIs(6+1))
        {
            base.PlaySoundEffect("MOVEMENT_MUD_STEP");
        }

        if (FrameIs(0+1) || FrameIs(4+1) || FrameIs(8+1))
        {
            SnapXToGrid();
            if (InputSameAsDirection() == false)
            {
                return GoTo(Crouch);
            }
        }
        else if (FrameIs(1+1) || FrameIs(5+1) || FrameIs(9+1))
        {
            if (InputSameAsDirection())
            {
                if (Actions.Run(mInput.IsHeld))
                {
                    // TODO: Fix InputRunPressed and the likes, will be missed if pressed between frames
                    return StandToRun();
                }
            }
        }
    }

    static game_speak =
    {
         GameSpeak1 = { anims = [ "AbeStandSpeak2",    "AbeCrouchSpeak1" ], sound = "GAMESPEAK_MUD_HELLO"},
         GameSpeak2 = { anims = [ "AbeStandSpeak3",    "AbeCrouchSpeak1" ], sound = "GAMESPEAK_MUD_FOLLOWME"},
         GameSpeak3 = { anims = [ "AbeStandingSpeak4", "AbeCrouchSpeak2" ], sound = "GAMESPEAK_MUD_WAIT"},
         GameSpeak4 = { anims = [ "AbeStandingSpeak4", "AbeCrouchSpeak1" ], sound = "GAMESPEAK_MUD_ANGRY"},
         GameSpeak5 = { anims = [ "AbeStandingSpeak4", "AbeCrouchSpeak2" ], sound = "GAMESPEAK_MUD_WORK"},
         GameSpeak6 = { anims = [ "AbeStandSpeak2",    "AbeCrouchSpeak2" ], sound = "GAMESPEAK_MUD_ALLYA"},
         GameSpeak7 = { anims = [ "AbeStandSpeak5",    "AbeCrouchSpeak1" ], sound = "GAMESPEAK_MUD_SORRY"},
         GameSpeak8 = { anims = [ "AbeStandSpeak3",    "AbeCrouchSpeak2" ], sound = "GAMESPEAK_MUD_NO_SAD"},    // TODO: actually "Stop it"
        // TODO: Laugh, whistle1/2 for AO
    };

    function HandleGameSpeak(standing)
    {
        local numGameSpeaks = game_speak.len();
        foreach(key, item in game_speak)
        {
            if (Actions[key](mInput.IsPressed))
            {
                if (standing == 1)
                {
                    GameSpeakStanding(item.anims[0], item.sound);
                }
                else
                {
                    GameSpeakCrouching(item.anims[1], item.sound);
                }
                return true;
            }
        }
        return false;
    }

    function GameSpeakStanding(anim, soundEffect)
    {
        PlayAnimation("AbeStandSpeak1");
        base.PlaySoundEffect(soundEffect);
        PlayAnimation(anim);
        SetAnimation("AbeStandIdle");
    }

    function GameSpeakFartStanding()
    {
        base.PlaySoundEffect("GAMESPEAK_MUD_FART");
        PlayAnimation("AbeStandSpeak5");
        SetAnimation("AbeStandIdle");
    }

    function GameSpeakCrouching(anim, soundEffect)
    {
        base.PlaySoundEffect(soundEffect);
        PlayAnimation(anim);
        SetAnimation("AbeCrouchIdle");
    }

    function GameSpeakFartCrouching()
    {
        GameSpeakCrouching("AbeCrouchSpeak1", "GAMESPEAK_MUD_FART");
        SetAnimation("AbeCrouchIdle");
    }

    function IsCellingAbove()
    {
        // If we move up will we smack the celling?
        if (base.CellingCollision(0, -60)) { return true; }
        return false;
    }

    function Crouch()
    {
        if (Actions.Up(mInput.IsHeld))
        {
            if (IsCellingAbove() == false)
            {
                return CrouchToStand();
            }
            else
            {
                log_info("Can't stand due to celling");
            }
        }
        else if (InputSameAsDirection())
        {
            return CrouchToRoll();
        }
        else if (InputNotSameAsDirection())
        {
            return CrouchTurnAround();
        }
        else if (HandleGameSpeak(0))
        {
            // stay in this state
        }
        else if (Actions.RollOrFart(mInput.IsHeld))
        {
            GameSpeakFartCrouching();
        }
        // TODO: Crouching object pick up
    }

    function StandToHop()
    {
        PlayAnimation("AbeStandToHop",
        {
            startFrame = 9,
            endFrame = 11,
            onFrame = function(obj)
            {
                if (obj.FrameNumber() == 9)
                {
                    obj.SetXSpeed(17);
                }
                else
                {
                    obj.SetXSpeed(13.569992);
                    obj.SetYSpeed(-2.7);
                }
            }
        });

        SetYVelocity(-1.8);
        PlayAnimation("AbeHopping",
        {
            endFrame = 3,
            onFrame = function(obj)
            {
                if (obj.FrameIs(3))
                {
                    obj.SetXSpeed(0);
                    obj.SetYSpeed(0);
                    obj.SetYVelocity(0);
                    obj.SnapXToGrid();
                }
            }
        });

        PlayAnimation("AbeHoppingToStand");

        return GoTo(Stand);
    }

    function Stand()
    {
        if (InputNotSameAsDirection())
        {
            log_info("StandTurnAround");
            return StandTurnAround();
        }
        else if (InputSameAsDirection())
        {
            if (Actions.Run(mInput.IsHeld))
            {
                return StandToRun();
            }
            else if (Actions.Sneak(mInput.IsHeld))
            {
                local collision = WillStepIntoWall();
                if (collision) { return collision };
                return StandToSneak();
            }
            else
            {
                local collision = WillStepIntoWall();
                if (collision) { return collision; }
                return StandToWalk();
            }
        }
        else if (Actions.Down(mInput.IsHeld))
        {
            return StandToCrouch();
        }
        else if (HandleGameSpeak(1))
        {
            // stay in this state
        }
        else if (Actions.RollOrFart(mInput.IsHeld))
        {
            GameSpeakFartStanding();
        }
        else if (Actions.Jump(mInput.IsHeld))
        {
            return StandToHop();
        }
        else if (Actions.Up(mInput.IsHeld))
        {
            return JumpUp();
        }
    }

    function HoistHang()
    {
        while(true)
        {
            // loop the animation until the user breaks out of it
            local toDown = false;
            local toUp = false;
            PlayAnimation("AbeHoistDangling",
            {
                onFrame = function(obj)
                {
                    if (Actions.Down(obj.mInput.IsHeld)) { toDown = true; }
                    if (Actions.Up(obj.mInput.IsHeld)) { toUp = true; }
                    if (toDown || toUp)
                    {
                        return true;
                    }
                }
            });

            if (toDown)
            {
                // TODO: Use the line ypos!
                mBase.mYPos = mBase.mYPos + 78;
                return GoTo(StandFalling2);
            }

            if (toUp)
            {
                PlayAnimation("AbeHoistPullSelfUp");
                return GoTo(Stand);
            }
        }
    }

    function JumpUp()
    {
        local oldY = mBase.mYPos;

        PlayAnimation("AbeStandToJump",
        {
            onFrame = function(obj)
            {
                if (obj.FrameNumber() == 9)
                {
                    obj.SetYSpeed(-8);
                }
            }
        });

        // Look for a hoist at the head pos
        local hoist = mMap.GetMapObject(mBase.mXPos, mBase.mYPos-50, "Hoist");

        SetYVelocity(-1.8);
        PlayAnimation("AbeJumpUpFalling",
        {
            onFrame = function(obj)
            {
                if (obj.FrameNumber() >= 3)
                {
                    if (hoist)
                    {
                        // TODO: Calculate pos to collision line, we can't reach
                        // the hoist if its higher than the pos in this frame as its when
                        // we start to fall back to the ground
                        log_info("TODO: Hoist check");
                        return true;
                    }
                }
            }
        });

        SetYSpeed(0);
        SetYVelocity(0);

        if (hoist)
        {
            // TODO: Use the next line or hoist ypos!
            mBase.mYPos = mBase.mYPos - 78;
            return GoTo(HoistHang);
        }
        else
        {
            log_info("TODO: Check for door, rope, well etc");

            mBase.mYPos = oldY;
            PlayAnimation("AbeHitGroundToStand");
            return GoTo(Stand);
        }
    }

    mData = {};

    function Exec()
    {
        while (true)
        {
            while (true)
            {
                // TODO Keep "pressed" flags for buttons that we can check mid animation
                // TODO Clear pressed flags before we change states
                // TODO: Change pressed checking to use stored flags/pressed states
                SetAnimation(mData.Animation);
                local frameChanged = base.AnimUpdate();

                //log_info("call mFunc..");
                if (mData.mFunc.bindenv(this)())
                {
                    //ApplyMovement();
                    break;
                }

                if (frameChanged)
                {
                    ApplyMovement();
                }

                //log_info("suspending..");
                ::suspend();
                //log_info("resumed");
            }
        }
    }

    function CoRoutineProc(thisPtr)
    {
        log_info("set first state..");
        thisPtr.GoTo.bindenv(thisPtr)(Abe.Stand);

        log_info("inf exec..");
        thisPtr.Exec.bindenv(thisPtr)();
    }

    oldX = 0;
    oldY = 0;
    function DebugPrintPosDeltas()
    {
        if (oldX != mXPos || oldY != mBase.mYPos)
        {
            local deltaX = mBase.mXPos - oldX;
            local deltaY = mBase.mYPos - oldY;
            if (deltaX != mBase.mXPos || deltaY != mBase.mYPos)
            {
                log_trace(
                    "XD:"   + string.format("%.6f", deltaX) +
                    " YD:" + string.format("%.6f", deltaY) +
                    " F:"   + base.FrameNumber() +
                    //" X:" + string.format("%.2f", mBase.mXPos) +
                    //" Y:" + string.format("%.2f", mBase.mYPos) +
                    " XS:" + string.format("%.2f", mXSpeed) +
                    " YS:" + string.format("%.2f", mYSpeed) +
                    " XV:" + string.format("%.2f", mXVelocity) +
                    " YV:" + string.format("%.2f", mYVelocity));
            }
        }
        oldX = mBase.mXPos;
        oldY = mBase.mYPos;
    }

    mFirst = true;
    function Update(actions)
    {
        //log_info("+Update");

        mInput = actions;

        if (mFirst)
        {
            mThread.call(this);
            mFirst = false;
        }
        else
        {
            mThread.wakeup(this);
        }
        //log_info("-Update");

        // TODO: Fix me
        //DebugPrintPosDeltas();
    }

    mAnims = {};
    mInvertX = false;
    mXSpeed = 0;
    mXVelocity = 0;
    mYSpeed = 0;
    mYVelocity = 0;
    mNextFunction = "";
    mThread = "";
    mInput = 0;

    function constructor(mapObj, map)
    {
        base.constructor(mapObj, map, "Abe");

        mAnims[Abe.Stand] <-          { name = "AbeStandIdle",                xspeed = 0,               xvel = 0 };
        mAnims[Abe.Walk] <-           { name = "AbeWalking",                  xspeed = 2.777771,        xvel = 0 };
        mAnims[Abe.Run] <-            { name = "AbeRunning",                  xspeed = 6.25,            xvel = 0 };
        mAnims[Abe.Crouch] <-         { name = "AbeCrouchIdle",               xspeed = 0,               xvel = 0 };
        mAnims[Abe.Sneak] <-          { name = "AbeSneaking",                 xspeed = 2.5,             xvel = 0 };
        mAnims[Abe.Roll] <-           { name = "AbeRolling",                  xspeed = 6.25,            xvel = 0 };
        mAnims[Abe.StandFalling] <-   { name = "AbeStandToFallingFromTrapDoor",                         xvel = 0.3,  yvel = -1.8 };
        mAnims[Abe.StandFalling2] <-  { name = "AbeHoistDropDown",                                      xvel = 0,    yvel = -1.8 };
        mAnims[Abe.HoistHang] <-      { name = "AbeHoistDangling",                                      xvel = 0.0,  yvel = 0 };
        mNextFunction = Abe.Stand;
        mThread = ::newthread(CoRoutineProc);
    }

}
