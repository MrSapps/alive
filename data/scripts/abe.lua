local Abe = {}
Abe.__index = Abe

function Abe:InputNotSameAsDirection()
    return (Actions.Left(self.mInput.IsHeld)  and self.mApi:FacingRight()) 
        or (Actions.Right(self.mInput.IsHeld) and self.mApi:FacingLeft())
end

function Abe:InputSameAsDirection()
    return (Actions.Left(self.mInput.IsHeld)  and self.mApi:FacingLeft()) 
        or (Actions.Right(self.mInput.IsHeld) and self.mApi:FacingRight())
end

function Abe:SetAnimationFrame(frame)
  print("Force animation frame to " .. frame)
  self.mApi:SetAnimationFrame(frame)
end

function Abe:SetAnimation(anim)
  if anim ~= self.mLastAnimationName then
    print("SetAnimation: " .. anim)
    self.mApi:SetAnimation(anim)
    self.mLastAnimationName = anim
  end  
end

function Abe:FlipXDirection() self.mApi:FlipXDirection() end
function Abe:FrameIs(frame) return self.mApi:FrameNumber() == frame and self.mApi:FrameCounter() == 0 end
function Abe:SetXSpeed(speed) self.mXSpeed = speed end
function Abe:SetYSpeed(speed) self.mYSpeed = speed end
function Abe:SetXVelocity(velocity) self.mXVelocity = velocity end
function Abe:SetYVelocity(velocity) self.mYVelocity = velocity end

function Abe:SnapXToGrid() 
  -- TODO: This breaks sometimes, stand idle, press inverse direction and take 1 step to repro issue
  self.mApi:SnapXToGrid() 
end


function Abe:ToStandCommon(anim) 
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:PlayAnimation{anim, onFrame = function() 
    if self:FrameIs(2) then PlaySoundEffect("MOVEMENT_MUD_STEP") end
  end} 
  --self:SnapXToGrid()
  return self:GoTo(self.Stand)
end

function Abe:ToStand() return self:ToStandCommon("AbeWalkToStand") end
function Abe:ToStand2() return self:ToStandCommon("AbeWalkToStandMidGrid") end

function Abe:WalkToRunCommon(anim)
  self:PlayAnimation{anim}
  return self:GoTo(self.Run)
end

function Abe:WalkToRun() return self:WalkToRunCommon("AbeWalkingToRunning") end
function Abe:WalkToRun2() return self:WalkToRunCommon("AbeWalkingToRunningMidGrid") end

function Abe:WalkToSneakCommon(anim)
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:PlayAnimation{anim} 
  return self:GoTo(self.Sneak)
end

function Abe:WalkToSneak() return self:WalkToSneakCommon("AbeWalkingToSneaking") end
function Abe:WalkToSneak2() return self:WalkToSneakCommon("AbeWalkingToSneakingMidGrid") end

function Abe:StandToRun()
  self:SetXSpeed(6.25)
  self:SetXVelocity(0)
  self:PlayAnimation{"AbeStandToRun"}
  return self:GoTo(self.Run)
end

function Abe:RunToSkidTurnAround()
  self:SetXSpeed(6.25)
  self:SetXVelocity(0.375)
  self:PlayAnimation{'AbeRunningToSkidTurn', endFrame = 15, onFrame = function()
    if self:FrameIs(14) then
      print("Handle last frame of AbeRunningToSkidTurn")
      self:SetXVelocity(0)
      self:SnapXToGrid()
    end
  end}
  
  if Actions.Run(self.mInput.IsHeld) then
    self:SetXSpeed(6.25)
    self:SetXVelocity(0)
    -- TODO: Probably better to have a FlipSpriteX and FlipDirectionX instead?
    self.mInvertX = true
    self:PlayAnimation{"AbeRunningTurnAround"}
    self.mInvertX = false
    self:FlipXDirection()
    --self:SnapXToGrid()
   
    return self:GoTo(self.Run)
  else
    self:SetXSpeed(2.777771)
    self:SetXVelocity(0)
    self.mInvertX = true
    self:PlayAnimation{"AbeRunningTurnAroundToWalk"}
    self.mInvertX = false
    self:FlipXDirection()
   return self:GoTo(self.Walk)
  end

end

function Abe:RunToRoll()
  self:SetXVelocity(0)
  self:SetXSpeed(6.25)
  self:PlayAnimation{"AbeRunningToRoll"}
  return self:GoTo(self.Roll)
end

function Abe:RunToJump()
  print("TODO: RunToJump")
end

function Abe:Run()
  if self:FrameIs(0) and self.mApi:AnimationComplete() == false then
    --self:SetXSpeed(12.5)
    -- the real game uses 12.5 sometimes depending on the previous state, but 6.25
    -- seems to always look correct..
    self:SetXSpeed(6.25)
  else
    self:SetXSpeed(6.25)
  end
  
  if self:FrameIs(0+1) or self:FrameIs(8+1) then
    self:SnapXToGrid()
    if self:InputSameAsDirection() and Actions.Run(self.mInput.IsHeld) and Actions.Jump(self.mInput.IsHeld) then
      return self:RunToJump()
    end
  end

  if self:FrameIs(4+1) or self:FrameIs(12+1) then
    self:SnapXToGrid()
    if self:InputNotSameAsDirection() then 
      return self:RunToSkidTurnAround()
    elseif self:InputSameAsDirection() then
      PlaySoundEffect("MOVEMENT_MUD_STEP") -- TODO: Always play fx?
      if Actions.Run(self.mInput.IsHeld) == false then 
        if self:FrameIs(4+1) then return self:RunToWalk() else return self:RunToWalk2() end
      elseif Actions.Jump(self.mInput.IsHeld) then
        return self:RunToJump()
      elseif Actions.RollOrFart(self.mInput.IsHeld) then
        return self:RunToRoll()
      end
    else
      return self:RunToSkidStop()
    end
  end
end

function Abe:RunToSkidStop()
  print("RunToSkidStop")
  self:SetXVelocity(0.375)
  self:PlayAnimation{'AbeRunningSkidStop', endFrame = 15, onFrame = function()
    if self:FrameIs(14) then
      print("Handle last frame of AbeRunningSkidStop")
      self:SetXVelocity(0)
      self:SnapXToGrid()
    end
  end}

  return self:GoTo(self.Stand)
end

function Abe:RunToWalkCommon(anim)
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:PlayAnimation{anim}
  return self:GoTo(self.Walk)
end

function Abe:RunToWalk() return self:RunToWalkCommon("AbeRunningToWalk") end
function Abe:RunToWalk2() return self:RunToWalkCommon("AbeRunningToWalkingMidGrid") end

function Abe:StandToSneak() 
  self:SetXSpeed(2.5)
  self:PlayAnimation{"AbeStandToSneak"}
  return self:GoTo(self.Sneak)
end

function Abe:Sneak()
  if (self:InputSameAsDirection()) then
    if self:FrameIs(6+1) or self:FrameIs(16+1) then
      self:SnapXToGrid()
      PlaySoundEffect("MOVEMENT_MUD_STEP") 
      if Actions.Sneak(self.mInput.IsHeld) == false then
        if self:FrameIs(6+1) then return self:SneakToWalk() else return self:SneakToWalk2() end
      end
    end
  else
    if self:FrameIs(3+1) or self:FrameIs(13+1) then
      if self:FrameIs(3+1) then return self:AbeSneakToStand() else return self:AbeSneakToStand2() end
    end
  end
end

function Abe:AbeSneakToStandCommon(anim)
    self:PlayAnimation{anim}
    return self:GoTo(self.Stand)
end

function Abe:AbeSneakToStand() return self:AbeSneakToStandCommon("AbeSneakToStand") end
function Abe:AbeSneakToStand2() return self:AbeSneakToStandCommon("AbeSneakToStandMidGrid") end

function Abe:SneakToWalkCommon(anim)
  self:PlayAnimation{anim}
  return self:GoTo(self.Walk)
end

function Abe:SneakToWalk() return self:SneakToWalkCommon("AbeSneakingToWalking") end
function Abe:SneakToWalk2() return self:SneakToWalkCommon("AbeSneakingToWalkingMidGrid") end

function Abe:ApplyMovement()
  if self.mXSpeed > 0 then
    self.mXSpeed = self.mXSpeed - self.mXVelocity
    if self.mApi:FacingLeft() then
      if self.mInvertX then
        self.mApi.mXPos = self.mApi.mXPos + self.mXSpeed
      else
        self.mApi.mXPos = self.mApi.mXPos - self.mXSpeed
      end
    else
      if self.mInvertX then
        self.mApi.mXPos = self.mApi.mXPos - self.mXSpeed
      else
        self.mApi.mXPos = self.mApi.mXPos + self.mXSpeed
      end
    end
  end
  
  --if self.mYSpeed < 0 then
    self.mYSpeed = self.mYSpeed - self.mYVelocity
    self.mApi.mYPos = self.mApi.mYPos + self.mYSpeed
  --end
  
end

function Abe:Walk()
  if self:FrameIs(5+1) or self:FrameIs(14+1) then 
    PlaySoundEffect("MOVEMENT_MUD_STEP") 
    self:SnapXToGrid()
    if (self:InputSameAsDirection() == true) then
      if (Actions.Run(self.mInput.IsHeld)) then
        if self:FrameIs(5+1) then return self:WalkToRun() else return self:WalkToRun2() end
      elseif (Actions.Sneak(self.mInput.IsHeld)) then 
        if self:FrameIs(5+1) then return self:WalkToSneak() else return self:WalkToSneak2() end
      end
    end
  elseif self:FrameIs(2+1) or self:FrameIs(11+1) then
    if (self:InputSameAsDirection() == false) then 
      if self:FrameIs(2+1) then return self:ToStand() else return self:ToStand2() end
    end
  end
end

function Abe:PlayAnimation(params)
  self:SetAnimation(params[1])
  if params.startFrame then
    print("Start animation at frame " .. params.startFrame)
    self:SetAnimationFrame(params.startFrame)
  else
    print("Start animation at beginning")
  end

  local stop = false
  local frameChanged = true
  while true do
    if frameChanged then
      if params.endFrame and self.mApi:FrameNumber()+1==params.endFrame 
      or self.mApi:FrameNumber() == self.mApi:NumberOfFrames()-1 then
        print("Wait for complete done at frame " .. self.mApi:FrameNumber())
        return
      else
        self:ApplyMovement()
      end
    end
    frameChanged = self.mApi:AnimUpdate()
    
    if frameChanged then
      if params.onFrame then 
        if (params.onFrame()) then
          print("Request to stop at frame " .. self.mApi:FrameNumber())
          stop = true
        end
      end
    end
    coroutine.yield()
    if stop then return end
  end
end

function Abe:GoTo(func)
  local data = self.mAnims[func]
  self.mData = { mFunc = func, Animation = data.name }
  print(self.mData.Animation)
  print(self.mData.mFunc)
  self:SetXSpeed(data.xspeed)
  self:SetXVelocity(data.xvel)
  if self.mData.Animation == nil then
    print("ERROR: An animation mapping is missing!")
  end
  return true
end

function Abe:StandToWalk()
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:PlayAnimation{"AbeStandToWalk"}
  return self:GoTo(self.Walk)
end

function Abe:StandToCrouch()
  self:PlayAnimation{"AbeStandToCrouch"}
  return self:GoTo(self.Crouch)
end

function Abe:CrouchToStand()
  self:PlayAnimation{"AbeCrouchToStand"}
  return self:GoTo(self.Stand)
end

function Abe:StandTurnAround() 
  PlaySoundEffect("GRAVEL_SMALL") -- TODO: Add to json
  local toRun = false
  
  -- stop at frame 3 if we want to go to running
  self:PlayAnimation{'AbeStandTurnAround', onFrame = function()
    if self:FrameIs(3) and Actions.Run(self.mInput.IsHeld) then
      toRun = true
      return true
    end
  end}

  self:FlipXDirection()
  if toRun then
    self:SetXSpeed(6.25)
    self:SetXVelocity(0)
    self:PlayAnimation{"AbeStandTurnAroundToRunning"}
    return self:GoTo(self.Run)
  end
  return self:GoTo(self.Stand) 
end

function Abe:CrouchStand() 
  self:PlayAnimation{"AbeCrouchTurnAround"} 
  self:FlipXDirection()
  return self:GoTo(self.Crouch) 
end

function Abe:CrouchToRoll()
  self:SetXSpeed(6.25)
  self:SetXVelocity(0)
  self:PlayAnimation{"AbeCrouchToRoll"} 
  return self:GoTo(self.Roll) 
end

function Abe:Roll()
  if self:FrameIs(0+1) or self:FrameIs(6+1) then
    PlaySoundEffect("MOVEMENT_MUD_STEP")
  end
  
  -- TODO: Check these frame numbers are correct, with +1 the StandToRun seems to jump a little
  -- TODO: This is certainly wrong as partial rolling can get you off the grid
  if self:FrameIs(0) or self:FrameIs(4) or self:FrameIs(8) then
    if self:InputSameAsDirection() then
      if Actions.Run(self.mInput.IsHeld) then
        -- TODO: Fix InputRunPressed and the likes, will be missed if pressed between frames
        -- TODO: Or AbeStandToRun if roll button is pressed
        return self:StandToRun()
      end
    else
      return self:GoTo(self.Crouch)
    end
  elseif self:FrameIs(1+1) or self:FrameIs(5+1) or self:FrameIs(9+1) then
    self:SnapXToGrid()
    if self:InputSameAsDirection() == false then
      return self:GoTo(self.Crouch)
    end
  end
end

local game_speak = 
{
  { Actions.GameSpeak1, { "AbeStandSpeak2",    "AbeCrouchSpeak1" }, "GAMESPEAK_MUD_HELLO"},
  { Actions.GameSpeak2, { "AbeStandSpeak3",    "AbeCrouchSpeak1" }, "GAMESPEAK_MUD_FOLLOWME"},
  { Actions.GameSpeak3, { "AbeStandingSpeak4", "AbeCrouchSpeak2" }, "GAMESPEAK_MUD_WAIT"},
  { Actions.GameSpeak4, { "AbeStandingSpeak4", "AbeCrouchSpeak1" }, "GAMESPEAK_MUD_ANGRY"},
  { Actions.GameSpeak5, { "AbeStandingSpeak4", "AbeCrouchSpeak2" }, "GAMESPEAK_MUD_WORK"},
  { Actions.GameSpeak6, { "AbeStandSpeak2",    "AbeCrouchSpeak2" }, "GAMESPEAK_MUD_ALLYA"},
  { Actions.GameSpeak7, { "AbeStandSpeak5",    "AbeCrouchSpeak1" }, "GAMESPEAK_MUD_SORRY"},
  { Actions.GameSpeak8, { "AbeStandSpeak3",    "AbeCrouchSpeak2" }, "GAMESPEAK_MUD_NO_SAD"},  -- TODO: actually "Stop it"
}

function Abe:HandleGameSpeak(standing)
  local numGameSpeaks = #game_speak
  for i = 1, numGameSpeaks do
    local item = game_speak[i]
      if item[1](self.mInput.IsPressed) then
        if standing == 1 then
          self:GameSpeakStanding(item[2][standing], item[3])
        else
          self:GameSpeakCrouching(item[2][standing], item[3])
        end
        return true
      end
  end
  return false
end

function Abe:GameSpeakStanding(anim, soundEffect)
  self:PlayAnimation{"AbeStandSpeak1"}
  PlaySoundEffect(soundEffect)
  self:PlayAnimation{anim}
  self:SetAnimation("AbeStandIdle")
end

function Abe:GameSpeakFartStanding()
  PlaySoundEffect("GAMESPEAK_MUD_FART")
  self:PlayAnimation{"AbeStandSpeak5"}
  self:SetAnimation("AbeStandIdle")
end

function Abe:GameSpeakCrouching(anim, soundEffect)
  PlaySoundEffect(soundEffect)
  self:PlayAnimation{anim}
  self:SetAnimation("AbeCrouchIdle")
end

function Abe:GameSpeakFartCrouching()
  self:GameSpeakCrouching("AbeCrouchSpeak1", "GAMESPEAK_MUD_FART")
  self:SetAnimation("AbeCrouchIdle")
end

function Abe:Crouch()
  if Actions.Up(self.mInput.IsHeld) then
    return self:CrouchToStand()
  elseif self:InputSameAsDirection() then 
    return self:CrouchToRoll()
  elseif self:InputNotSameAsDirection() then
    return self:CrouchStand()
  elseif self:HandleGameSpeak(2) then
    -- stay in this state
  elseif Actions.RollOrFart(self.mInput.IsHeld) then
    self:GameSpeakFartCrouching()
  end
  -- TODO: Crouching object pick up
end

function Abe:StandToHop()
  -- TODO: Must start at frame 9!
  -- TODO HACK should be passing 9 and 10 here?
  self:PlayAnimation{"AbeStandToHop", startFrame = 8, endFrame = 9+3, onFrame = function()
    if self.mApi:FrameNumber() == 8 then
      print("First")
      self:SetXSpeed(17)
    else
      print("Others")
      self:SetXSpeed(13.569992)
      self:SetYSpeed(-2.7)
    end
  end} 

  self:SetYVelocity(-1.8)
  self:PlayAnimation{"AbeHopping", onFrame = function()
    if self:FrameIs(3) then
      self:SnapXToGrid()
    elseif self:FrameIs(2) then
      self:SetYSpeed(0)
      self:SetYVelocity(0)
    end
  end}

  self:SetXSpeed(0)
  self:PlayAnimation{"AbeHoppingToStand"}

  return self:GoTo(self.Stand)
end

function Abe:Stand()
  if self:InputNotSameAsDirection() then 
    return self:StandTurnAround()
  elseif self:InputSameAsDirection() then 
    if Actions.Run(self.mInput.IsHeld) then 
      return self:StandToRun()
    elseif Actions.Sneak(self.mInput.IsHeld) then
      return self:StandToSneak()
    else 
      return self:StandToWalk() end
  elseif Actions.Down(self.mInput.IsHeld) then
    return self:StandToCrouch()
  elseif self:HandleGameSpeak(1) then
    -- stay in this state
  elseif Actions.RollOrFart(self.mInput.IsHeld) then
    self:GameSpeakFartStanding()
  elseif Actions.Jump(self.mInput.IsHeld) then
    return self:StandToHop()
  end
end

function Abe:Exec()
  while true do
    while true do
      -- TODO Keep "pressed" flags for buttons that we can check mid animation
      -- TODO Clear pressed flags before we change states
      -- TODO: Change pressed checking to use stored flags/pressed states
      self:SetAnimation(self.mData.Animation)
      local frameChanged = self.mApi:AnimUpdate()
      if self.mData.mFunc(self) then
        --self:ApplyMovement()
        break 
      end
      if frameChanged then
        self:ApplyMovement()
      end
      coroutine.yield()
    end
  end
end

function Abe:CoRoutineProc()
  self:GoTo(self.Stand)
  self:Exec()
end

local oldX = 0
local oldY = 0
function Abe:DebugPrintPosDeltas()
  if oldX ~= self.mApi.mXPos then
    local delta = self.mApi.mXPos - oldX
    if delta ~= self.mApi.mXPos then
      print("DELTA: " .. delta .. " on frame: " .. self.mApi:FrameNumber() .. " XPOS: " .. self.mApi.mXPos .. " speed " .. self.mXSpeed .. " xvel " .. self.mXVelocity)
    end
  end
  oldX = self.mApi.mXPos
  
  if oldY ~= self.mApi.mYPos then
    local delta = self.mApi.mYPos - oldY
    if delta ~= self.mApi.mYPos then
      print("DELTA: " .. delta .. " on frame: " .. self.mApi:FrameNumber() .. " YPOS: " .. self.mApi.mYPos .. " speed " .. self.mYSpeed .. " yvel " .. self.mYVelocity)
    end
  end
  oldY = self.mApi.mYPos
end

function Abe:Update() 
  --print("+Update") 
  coroutine.resume(self.mThread, self) 
  --print("-Update")
  self:DebugPrintPosDeltas()
end

function Abe.create()
  local ret = {}
  setmetatable(ret, Abe)
  ret.mNextFunction = ret.Stand
  ret.mLastAnimationName = ""
  ret.mInvertX = false
  ret.mXSpeed = 0
  ret.mXVelocity = 0
  ret.mYSpeed = 0
  ret.mYVelocity = 0
  ret.mAnims = {}
  ret.mAnims[Abe.Stand] =   { name = "AbeStandIdle",    xspeed = 0,           xvel = 0 }
  ret.mAnims[Abe.Walk] =    { name = "AbeWalking",      xspeed = 2.777771,    xvel = 0 }
  ret.mAnims[Abe.Run] =     { name = "AbeRunning",      xspeed = 6.25,        xvel = 0 }
  ret.mAnims[Abe.Crouch] =  { name = "AbeCrouchIdle",   xspeed = 0,           xvel = 0 }
  ret.mAnims[Abe.Sneak] =   { name = "AbeSneaking",     xspeed = 2.5,         xvel = 0 }
  ret.mAnims[Abe.Roll] =    { name = "AbeRolling",      xspeed = 6.25,        xvel = 0 }

  ret.mThread = coroutine.create(ret.CoRoutineProc)
  return ret
end

local function Abe_Debug()
  local a = Abe.create()
  a.mApi = {}
  a.mApi.mXPos = 0
  a.mApi.mYPos = 0
  a.mApi.SetAnimation = function(anim) print("Fake set animation: " .. anim) end
   
  a:Test()
end

-- Use when testing logic outside of the engine
--Abe_Debug()

-- C++ call points
function init(cppObj)
    cppObj.states = Abe.create()
    cppObj.states.mApi = cppObj
end

function update(cppObj, input)
    cppObj.states.mInput = input
    cppObj.states:Update()
end
