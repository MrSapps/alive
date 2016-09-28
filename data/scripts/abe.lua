local Abe = {}
Abe.__index = Abe

function Abe:InputNotSameAsDirection()
    return (self.mInput:InputLeft() and self.mApi:FacingRight()) or (self.mInput:InputRight() and self.mApi:FacingLeft())
end

function Abe:InputSameAsDirection()
    return (self.mInput:InputLeft() and self.mApi:FacingLeft()) or (self.mInput:InputRight() and self.mApi:FacingRight())
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
function Abe:SetXVelocity(velocity) self.mXVelocity = velocity end
function Abe:SnapToGrid() 
  -- TODO: This breaks sometimes, stand idle, press inverse direction and take 1 step to repro issue
  self.mApi:SnapToGrid() 
end

function Abe:WaitForAnimationComplete() self:WaitForAnimationCompleteCb(nil) end

function Abe:SetAndWaitForAnimationCompleteCb(anim, frameCallBackFunc, frame)
  self:SetAnimation(anim)
  self:WaitForAnimationCompleteCb(frameCallBackFunc, frame)
end

function Abe:SetAndWaitForAnimationFrameCb(anim, frame, cb) self:SetAndWaitForAnimationCompleteCb(anim, cb, frame) end
function Abe:SetAndWaitForAnimationFrame(anim, frame) self:SetAndWaitForAnimationCompleteCb(anim, nil, frame) end
function Abe:SetAndWaitForAnimationComplete(anim) self:SetAndWaitForAnimationCompleteCb(anim, nil, -1) end

function Abe:ToStandCommon(anim) 
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationCompleteCb(anim, function() 
    if self:FrameIs(2) then PlaySoundEffect("MOVEMENT_MUD_STEP") end
  end, -1) 
  --self:SnapToGrid()
  return self:GoTo(self.Stand)
end

function Abe:ToStand() return self:ToStandCommon("AbeWalkToStand") end
function Abe:ToStand2() return self:ToStandCommon("AbeWalkToStandMidGrid") end

function Abe:WalkToRunCommon(anim)
  self:SetAndWaitForAnimationComplete(anim)
  return self:GoTo(self.Run)
end

function Abe:WalkToRun() return self:WalkToRunCommon("AbeWalkingToRunning") end
function Abe:WalkToRun2() return self:WalkToRunCommon("AbeWalkingToRunningMidGrid") end

function Abe:WalkToSneakCommon(anim)
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete(anim) 
  return self:GoTo(self.Sneak)
end

function Abe:WalkToSneak() return self:WalkToSneakCommon("AbeWalkingToSneaking") end
function Abe:WalkToSneak2() return self:WalkToSneakCommon("AbeWalkingToSneakingMidGrid") end

function Abe:StandToRun()
  self:SetXSpeed(6.25)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete("AbeStandToRun")
  return self:GoTo(self.Run)
end

-- TODO: Fix issue with skid turn/stop grid snapping jumping
-- to wrong grid block
function Abe:RunToSkidTurnAround()
  self:SetXSpeed(6.25)
  self:SetXVelocity(0.375)
  self:SetAndWaitForAnimationFrameCb('AbeRunningToSkidTurn', 15, 
    function()
      if self:FrameIs(14) then
        print("Handle last frame of AbeRunningToSkidTurn")
        self:SetXVelocity(0)
        self:SnapToGrid()
      end
    end) 
  

  -- TODO: Also have AbeRunningTurnAroundToWalk if run no longer held
  -- TODO: Handle AbeStandTurnAroundToRunning
  
  self:SetXSpeed(6.25)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete("AbeRunningTurnAround")
  self:FlipXDirection()
  
  return self:GoTo(self.Run)
end

function Abe:RunToRoll()
  print("TODO: RunToRoll")
end

function Abe:RunToJump()
  print("TODO: RunToJump")
end

function Abe:Run()
  if self:FrameIs(0) and self.mApi:AnimationComplete() == false then
    self:SetXSpeed(12)
  else
    self:SetXSpeed(6.25)
  end
  
  if self:FrameIs(0+1) or self:FrameIs(8+1) then
    if self:InputSameAsDirection() and self.mInput:InputRun() and self.mInput:InputJump() then
      return self:RunToJump()
    end
  end

  if self:FrameIs(4+1) or self:FrameIs(12+1) then
    self:SnapToGrid()
    if self:InputNotSameAsDirection() then 
      return self:RunToSkidTurnAround()
    elseif self:InputSameAsDirection() then
      PlaySoundEffect("MOVEMENT_MUD_STEP") -- TODO: Always play fx?
      if self.mInput:InputRun() == false then 
        if self:FrameIs(4+1) then return self:RunToWalk() else return self:RunToWalk2() end
      elseif self.mInput:InputJump() then
        return self:RunToJump()
      elseif self.mInput:InputRollOrFart() then
        return self:RunToRoll()
      end
    else
      return self:RunToSkidStop()
    end
  end
end

function Abe:RunToSkidStop()
  self:SetXVelocity(0.375)
  self:SetAndWaitForAnimationFrameCb('AbeRunningSkidStop', 15, 
    function()
      if self:FrameIs(14) then
        print("Handle last frame of AbeRunningSkidStop")
        self:SetXVelocity(0)
        self:SnapToGrid()
      end
  end) 
  return self:GoTo(self.Stand)
end

function Abe:RunToWalkCommon(anim)
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete(anim)
  return self:GoTo(self.Walk)
end

function Abe:RunToWalk() return self:RunToWalkCommon("AbeRunningToWalk") end
function Abe:RunToWalk2() return self:RunToWalkCommon("AbeRunningToWalkingMidGrid") end

function Abe:StandToSneak() 
  self:SetXSpeed(2.5)
  self:SetAndWaitForAnimationComplete("AbeStandToSneak")
  return self:GoTo(self.Sneak)
end

function Abe:Sneak()
  if (self:InputSameAsDirection()) then
    if self:FrameIs(6+1) or self:FrameIs(16+1) then
      self:SnapToGrid()
      PlaySoundEffect("MOVEMENT_MUD_STEP") 
      if self.mInput:InputSneak() == false then
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
    self:SetAndWaitForAnimationComplete(anim)
    return self:GoTo(self.Stand)
end

function Abe:AbeSneakToStand() return self:AbeSneakToStandCommon("AbeSneakToStand") end
function Abe:AbeSneakToStand2() return self:AbeSneakToStandCommon("AbeSneakToStandMidGrid") end

function Abe:SneakToWalkCommon(anim)
  self:SetAndWaitForAnimationComplete(anim)
  return self:GoTo(self.Walk)
end

function Abe:SneakToWalk() return self:SneakToWalkCommon("AbeSneakingToWalking") end
function Abe:SneakToWalk2() return self:SneakToWalkCommon("AbeSneakingToWalkingMidGrid") end

function Abe:ApplyMovement()
  if self.mXSpeed > 0 then
    self.mXSpeed = self.mXSpeed - self.mXVelocity
    if self.mApi:FacingLeft() then
      self.mApi.mXPos = self.mApi.mXPos - self.mXSpeed
    else
      self.mApi.mXPos = self.mApi.mXPos + self.mXSpeed
    end
  end
end

function Abe:Walk()
  if self:FrameIs(5+1) or self:FrameIs(14+1) then 
    PlaySoundEffect("MOVEMENT_MUD_STEP") 
    self:SnapToGrid()
    if (self:InputSameAsDirection() == true) then
      if (self.mInput:InputRun()) then
        if self:FrameIs(5+1) then return self:WalkToRun() else return self:WalkToRun2() end
      elseif (self.mInput:InputSneak()) then 
        if self:FrameIs(5+1) then return self:WalkToSneak() else return self:WalkToSneak2() end
      end
    end
  elseif self:FrameIs(2+1) or self:FrameIs(11+1) then
    if (self:InputSameAsDirection() == false) then 
      if self:FrameIs(2+1) then return self:ToStand() else return self:ToStand2() end
    end
  end
end

function Abe:WaitForAnimationCompleteCb(frameCallBackFunc, frame)
  print("Start wait for complete at frame " .. frame)
  local frameChanged = true
  while true do
    if frameChanged then
      if ((frame == -1 and self.mApi:FrameNumber() == self.mApi:NumberOfFrames()-1) or (frame ~= -1 and self.mApi:FrameNumber()+1==frame)) then
        print("Wait for complete done at frame " .. frame)
        return
      else
        self:ApplyMovement()
      end
    end
    frameChanged = self.mApi:AnimUpdate()
    
    if frameChanged then
      if frameCallBackFunc ~= nil then 
        frameCallBackFunc() 
      end
    end
    coroutine.yield()
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
  self:SetAndWaitForAnimationComplete("AbeStandToWalk")
  return self:GoTo(self.Walk)
end

function Abe:StandToCrouch()
  self:SetAndWaitForAnimationComplete("AbeStandToCrouch")
  return self:GoTo(self.Crouch)
end

function Abe:CrouchToStand()
  self:SetAndWaitForAnimationComplete("AbeCrouchToStand")
  return self:GoTo(self.Stand)
end

function Abe:StandTurnAround() 
  PlaySoundEffect("GRAVEL_SMALL") -- TODO: Add to json
  self:SetAndWaitForAnimationComplete('AbeStandTurnAround') 
  self:FlipXDirection()
  return self:GoTo(self.Stand) 
end

function Abe:CrouchStand() 
  self:SetAndWaitForAnimationComplete('AbeCrouchTurnAround') 
  self:FlipXDirection()
  return self:GoTo(self.Crouch) 
end

function Abe:CrouchToRoll()
  self:SetXSpeed(6.25)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete('AbeCrouchToRoll') 
  return self:GoTo(self.Roll) 
end

function Abe:Roll()
  if self:FrameIs(0+1) or self:FrameIs(6+1) then
    PlaySoundEffect("MOVEMENT_MUD_STEP")
  end
  
  -- TODO: Handle RollToRun when run pressed
  -- TODO: Handle RollToWalk when roll pressed (or skip because I've never used it and it seems buggy?)
  
  if self:FrameIs(0+1) or self:FrameIs(4+1) or self:FrameIs(8+1) then
    self:SnapToGrid()
    if self:InputSameAsDirection() == false then 
      return self:GoTo(self.Crouch)
    end
  end
end

local game_speak = 
{
  { Actions.InputGameSpeak1, { "AbeStandSpeak2",    "AbeCrouchSpeak1" }, "GAMESPEAK_MUD_HELLO"},
  { Actions.InputGameSpeak2, { "AbeStandSpeak3",    "AbeCrouchSpeak1" }, "GAMESPEAK_MUD_FOLLOWME"},
  { Actions.InputGameSpeak3, { "AbeStandingSpeak4", "AbeCrouchSpeak2" }, "GAMESPEAK_MUD_WAIT"},
  { Actions.InputGameSpeak4, { "AbeStandingSpeak4", "AbeCrouchSpeak1" }, "GAMESPEAK_MUD_ANGRY"},
  { Actions.InputGameSpeak5, { "AbeStandingSpeak4", "AbeCrouchSpeak2" }, "GAMESPEAK_MUD_WORK"},
  { Actions.InputGameSpeak6, { "AbeStandSpeak2",    "AbeCrouchSpeak2" }, "GAMESPEAK_MUD_ALLYA"},
  { Actions.InputGameSpeak7, { "AbeStandSpeak5",    "AbeCrouchSpeak1" }, "GAMESPEAK_MUD_SORRY"},
  { Actions.InputGameSpeak8, { "AbeStandSpeak3",    "AbeCrouchSpeak2" }, "GAMESPEAK_MUD_NO_SAD"},  -- TODO: actually "Stop it"
}

function Abe:HandleGameSpeak(standing)
  local numGameSpeaks = #game_speak
  for i = 1, numGameSpeaks do
    local item = game_speak[i]
      if item[1](self.mInput) then
        if standing == 1 then
          self:GameSpeakStanding(item[2][standing], item[3])
        else
          self:GameSpeakCrouching(item[2][standing], item[3])
        end
        return
      end
  end
end

function Abe:GameSpeakStanding(anim, soundEffect)
  self:SetAndWaitForAnimationComplete("AbeStandSpeak1")
  PlaySoundEffect(soundEffect)
  self:SetAndWaitForAnimationComplete(anim)
  self:SetAnimation("AbeStandIdle")
end

function Abe:GameSpeakFartStanding()
  PlaySoundEffect("GAMESPEAK_MUD_FART")
  self:SetAndWaitForAnimationComplete("AbeStandSpeak5")
  self:SetAnimation("AbeStandIdle")
end

function Abe:GameSpeakCrouching(anim, soundEffect)
  PlaySoundEffect(soundEffect)
  self:SetAndWaitForAnimationComplete(anim)
  self:SetAnimation("AbeCrouchIdle")
end

function Abe:GameSpeakFartCrouching(anim, soundEffect)
  self:GameSpeakCrouching("AbeCrouchSpeak1", "GAMESPEAK_MUD_FART")
  self:SetAnimation("AbeCrouchIdle")
end

function Abe:Crouch()
  if self.mInput:InputUp() then
    return self:CrouchToStand()
  elseif self:InputSameAsDirection() then 
    return self:CrouchToRoll()
  elseif self:InputNotSameAsDirection() then
    return self:CrouchStand()
  elseif (self.mInput:InputRollOrFart()) then
    self:GameSpeakFartCrouching()
  else
    self:HandleGameSpeak(2)
  end
  -- TODO: Crouching object pick up
end

function Abe:Stand()
  if (self:InputSameAsDirection()) then 
    if self.mInput:InputRun() then 
      return self:StandToRun()
    elseif self.mInput:InputSneak() then
      return self:StandToSneak()
    else 
      return self:StandToWalk() end
  elseif (self:InputNotSameAsDirection()) then 
    return self:StandTurnAround()
  elseif (self.mInput:InputDown()) then
      return self:StandToCrouch()
  elseif (self.mInput:InputRollOrFart()) then
    self:GameSpeakFartStanding()
  else
    self:HandleGameSpeak(1)
  end
end

function Abe:Exec()
  while true do
    while true do
      self:SetAnimation(self.mData.Animation)
      if self.mApi:AnimUpdate() then
        if self.mData.mFunc(self) then
          --self:ApplyMovement()
          break 
        end
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
function Abe:DebugPrintPosDeltas()
  if oldX ~= self.mApi.mXPos then
    local delta = self.mApi.mXPos - oldX
    if delta ~= self.mApi.mXPos then
      print("DELTA: " .. delta .. " on frame: " .. self.mApi:FrameNumber() .. " XPOS: " .. self.mApi.mXPos .. " speed " .. self.mXSpeed .. " vel " .. self.mXVelocity)
    end
  end
  oldX = self.mApi.mXPos
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
  ret.mXSpeed = 0
  ret.mXVelocity = 0
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
