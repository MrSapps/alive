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
  --self.mApi:SnapToGrid() 
end

function Abe:WaitForAnimationComplete() self:WaitForAnimationCompleteCb(nil) end

function Abe:SetAndWaitForAnimationCompleteCb(anim, frameCallBackFunc, frame)
  self:SetAnimation(anim)
  self:WaitForAnimationCompleteCb(frameCallBackFunc, frame)
end

function Abe:SetAndWaitForAnimationFrame(anim, frame) self:SetAndWaitForAnimationCompleteCb(anim, nil, frame) end
function Abe:SetAndWaitForAnimationComplete(anim) self:SetAndWaitForAnimationCompleteCb(anim, nil, -1) end

function Abe:GameSpeak(anim, soundEffect)
  self:SetAndWaitForAnimationComplete("AbeStandSpeak1")
  PlaySoundEffect(soundEffect)
  self:SetAndWaitForAnimationComplete(anim)
  self:SetAnimation('AbeStandIdle')
end

function Abe:ToStand() return self:ToStandCommon("AbeWalkToStand") end
function Abe:ToStand2() return self:ToStandCommon("AbeWalkToStandMidGrid") end

function Abe:WalkToRun()
  self:SetAndWaitForAnimationComplete('AbeWalkingToRunning')
  --self:SetXVelocity(0)
  --self:SetXSpeed(6.25)
  return self:GoTo(self.Run)
end

function Abe:WalkToSneak()
  --self:SetXSpeed(2.777771)
  --self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete('AbeWalkingToSneaking') 
  return self:GoTo(self.Sneak)
end

function Abe:StandToRun()
  --self:SetXSpeed(6.25)
  --self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete('AbeStandToRun')
  return self:GoTo(self.Run)
end

function Abe:Run()
  if self:FrameIs(0) then
    self:SetXSpeed(12)
  else
    self:SetXSpeed(6.25)
  end
  
  if self:FrameIs(4+1) or self:FrameIs(12+1) then
    if (self:InputSameAsDirection()) then 
      if self.mInput:InputRun() == false then self:RunToWalk() end
    else
      return self:RunToSkidStop()
    end
  end
end

function Abe:RunToSkidStop()
  self:SetXVelocity(0.375)
  self:SetAndWaitForAnimationFrame('AbeRunningSkidStop', 15)
  self:SnapToGrid()
  return self:GoTo(self.Stand)
end

function Abe:RunToWalk()
  --self:SetXSpeed(2.777771)
  -- TODO: On frame 9!
  self:SetAndWaitForAnimationComplete("AbeRunningToWalkingMidGrid")
  --self:SetXVelocity(0)
  return self:GoTo(self.Walk)
end

function Abe:StandToSneak()
  print("TODO: StandToSneak")
  return self:GoTo(self.Sneak)
end

function Abe:Sneak()
  self:SetAnimation("AbeSneaking")
  if (self:InputNotSameAsDirection()) then self:TurnAround() end
  if (self.mInput:InputSameAsDirection()) then
    if (self.mInput:InputSneak() == false) then self:SneakToWalk() end
  else
    self:ToStand()
  end
end

function Abe:SneakToWalk()
  --self:SetXSpeed(2.7)
  --self:SetXVelocity(0)
  -- TODO: Can also be AbeSneakingToWalkingMidGrid
  self:SetAndWaitForAnimationComplete("AbeSneakingToWalking")
  return self:GoTo(self.Walk)
end

function Abe:TurnAround() 
  PlaySoundEffect("GRAVEL_SMALL") -- TODO: Add to json
  self:SetAndWaitForAnimationComplete('AbeStandTurnAround') 
  self:FlipXDirection()
  return self:GoTo(self.Stand) 
end

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
  if self:FrameIs(5) or self:FrameIs(14) then 
    PlaySoundEffect("MOVEMENT_MUD_STEP") 
    self:SnapToGrid()
    if (self:InputSameAsDirection() == true) then
      if (self.mInput:InputRun()) then return self:WalkToRun()
      elseif (self.mInput:InputSneak()) then return self:WalkToSneak() end
    end
  elseif self:FrameIs(2+1) or self:FrameIs(11+1) then
    --if (OnGround() == false) then ToFalling()
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

function Abe:ToStandCommon(anim) 
  self:SetAndWaitForAnimationCompleteCb(anim, function() 
    if self:FrameIs(2) then PlaySoundEffect("MOVEMENT_MUD_STEP") end
  end, -1) 
  self:SnapToGrid()
  return self:GoTo(self.Stand)
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
  --self:SetXSpeed(2.777771)
  --self:SetXVelocity(0)
  print("Set AbeStandToWalk")
  -- TODO: Need to set XSpeed here, not after this!
  self:SetAndWaitForAnimationComplete("AbeStandToWalk")
  print("AbeStandToWalk done")
  return self:GoTo(self.Walk)
end

-- TODO: Change to using this table
local game_speak = {
  {"AbeStandSpeak2",     "GAMESPEAK_MUD_HELLO"},
  {"AbeStandSpeak3",     "GAMESPEAK_MUD_FOLLOWME"},
  {"AbeStandingSpeak4",  "GAMESPEAK_MUD_WAIT"},
  {"AbeStandingSpeak4",  "GAMESPEAK_MUD_ANGRY"},
  {"AbeStandingSpeak4",  "GAMESPEAK_MUD_WORK"},
  {"AbeStandSpeak2",     "GAMESPEAK_MUD_ALLYA"},
  {"AbeStandSpeak5",     "GAMESPEAK_MUD_SORRY"},
  {"AbeStandSpeak3",     "GAMESPEAK_MUD_NO_SAD"},
}

function Abe:Stand()
  --self:SetXSpeed(0)
  --self:SetXVelocity(0)
  if (self:InputSameAsDirection()) then 
    if self.mInput:InputRun() then 
      return self:StandToRun()
    elseif self.mInput:InputSneak() then 
      return self:StandToSneak()
    else 
      return self:StandToWalk() end
  elseif (self:InputNotSameAsDirection()) then 
    return self:TurnAround()
  elseif (self.mInput:InputGameSpeak1()) then 
    self:GameSpeak("AbeStandSpeak2",     "GAMESPEAK_MUD_HELLO")
  elseif (self.mInput:InputGameSpeak2()) then 
    self:GameSpeak("AbeStandSpeak3",     "GAMESPEAK_MUD_FOLLOWME")
  elseif (self.mInput:InputGameSpeak3()) then 
    self:GameSpeak("AbeStandingSpeak4",  "GAMESPEAK_MUD_WAIT")
  elseif (self.mInput:InputGameSpeak4()) then 
    self:GameSpeak("AbeStandingSpeak4",  "GAMESPEAK_MUD_ANGRY")
  elseif (self.mInput:InputGameSpeak5()) then 
    self:GameSpeak("AbeStandingSpeak4",  "GAMESPEAK_MUD_WORK")
  elseif (self.mInput:InputGameSpeak6()) then 
    self:GameSpeak("AbeStandSpeak2",     "GAMESPEAK_MUD_ALLYA")
  elseif (self.mInput:InputGameSpeak7()) then 
    self:GameSpeak("AbeStandSpeak5",     "GAMESPEAK_MUD_SORRY")
  elseif (self.mInput:InputGameSpeak8()) then 
    self:GameSpeak("AbeStandSpeak3",     "GAMESPEAK_MUD_NO_SAD")   -- Actually "Stop it"
  end
end

function Abe:Exec()
  while true do
    while true do
      self:SetAnimation(self.mData.Animation)
      if self.mApi:AnimUpdate() then
        if self.mData.mFunc(self) then
          self:ApplyMovement()
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
   ret.mAnimChanged = false
   ret.mAnims = {}
   ret.mAnims[Abe.Stand] = { name = "AbeStandIdle", xspeed = 0, xvel = 0}
   ret.mAnims[Abe.Walk] = { name = "AbeWalking", xspeed = 2.777771, xvel = 0}
   ret.mAnims[Abe.Run] = { name = "AbeRunning", xspeed = 6.25, xvel = 0}
   ret.mThread = coroutine.create(ret.CoRoutineProc)
   return ret
end

local function Abe_Debug()
  local a = Abe.create()
  a.mApi = {}
  a.mApi.mXPos = 0
  a.mApi.mYPos = 0
  a.mApi.SetAnimation = function(anim) print("Fake set animation: " .. anim) end
   
  a:GoTo(a.Stand)
  a:GoTo(a.Walk)
    
  while true do
    a:Update()
  end
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
