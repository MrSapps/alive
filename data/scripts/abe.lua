Abe = {}
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
    self.mAnimChanged = true
  end
  self.mLastAnimationName = anim
end

function Abe:FlipXDirection() self.mApi:FlipXDirection() end

function Abe:FrameIs(frame) 
  return self.mApi:FrameNumber() == frame
end

function Abe:SetXSpeed(speed) self.mXSpeed = speed end
function Abe:SetXVelocity(velocity) self.mXVelocity = velocity end
function Abe:SnapToGrid() 
  --self.mApi:SnapToGrid() 
end

function Abe:WaitForAnimationCompleteCb(frameCallBackFunc)
  print("Start wait for complete")
  
  -- Call back, move and let the engine render frame 0
  if frameCallBackFunc ~= nil then frameCallBackFunc() end
  self:ApplyMovement()
  coroutine.yield()

  while true do 
    local frameChanged = self.mApi:AnimUpdate()
    if frameChanged then
      if frameCallBackFunc ~= nil then frameCallBackFunc() end
      if self.mApi:IsLastFrame() then
        print("Wait for complete done")
        return
      else
        self:ApplyMovement()
      end
    end
    coroutine.yield() -- return for rendering/input update
  end
end

function Abe:WaitForAnimationComplete() self:WaitForAnimationCompleteCb(nil) end

function Abe:SetAndWaitForAnimationCompleteCb(anim, frameCallBackFunc)
  self:SetAnimation(anim)
  self:WaitForAnimationCompleteCb(frameCallBackFunc)
end

function Abe:SetAndWaitForAnimationComplete(anim) self:SetAndWaitForAnimationCompleteCb(anim, nil) end

function Abe:GameSpeak(anim, soundEffect)
  self:SetAndWaitForAnimationComplete("AbeStandSpeak1")
  PlaySoundEffect(soundEffect)
  self:SetAndWaitForAnimationComplete(anim)
  self:SetAnimation('AbeStandIdle')
end

function Abe:Stand()
  self:SetAnimation('AbeStandIdle')
  self:SetXSpeed(0)
  self:SetXVelocity(0)
  if (self:InputSameAsDirection()) then 
    if self.mInput:InputRun() then self:StandToRun()
    elseif self.mInput:InputSneak() then self:StandToSneak()
    else self:StandToWalk() end
  elseif (self:InputNotSameAsDirection()) then self:TurnAround()
  elseif (self.mInput:InputGameSpeak1()) then self:GameSpeak("AbeStandSpeak2",     "GAMESPEAK_MUD_HELLO")
  elseif (self.mInput:InputGameSpeak2()) then self:GameSpeak("AbeStandSpeak3",     "GAMESPEAK_MUD_FOLLOWME")
  elseif (self.mInput:InputGameSpeak3()) then self:GameSpeak("AbeStandingSpeak4",  "GAMESPEAK_MUD_WAIT")
  elseif (self.mInput:InputGameSpeak4()) then self:GameSpeak("AbeStandingSpeak4",  "GAMESPEAK_MUD_ANGRY")
  elseif (self.mInput:InputGameSpeak5()) then self:GameSpeak("AbeStandingSpeak4",  "GAMESPEAK_MUD_WORK")
  elseif (self.mInput:InputGameSpeak6()) then self:GameSpeak("AbeStandSpeak2",     "GAMESPEAK_MUD_ALLYA")
  elseif (self.mInput:InputGameSpeak7()) then self:GameSpeak("AbeStandSpeak5",     "GAMESPEAK_MUD_SORRY")
  elseif (self.mInput:InputGameSpeak8()) then self:GameSpeak("AbeStandSpeak3",     "GAMESPEAK_MUD_NO_SAD") end -- Actually "Stop it"
end

function Abe:StandToWalk()
  print("StandToWalk")
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete('AbeStandToWalk')
  self:Next(self.Walk)
end

function Abe:ToStandCommon(anim) 
  self:SetAndWaitForAnimationCompleteCb(anim, function() 
    if self:FrameIs(2) then PlaySoundEffect("MOVEMENT_MUD_STEP") end
  end) 
  self:SnapToGrid()
  self:Next(self.Stand)
end

function Abe:ToStand() 
  self:ToStandCommon("AbeWalkToStand")
end

function Abe:ToStand2()
  self:ToStandCommon("AbeWalkToStandMidGrid")
end

function Abe:Walk()
  self:SetAnimation('AbeWalking')
  if self:FrameIs(5) or self:FrameIs(14) then 
    PlaySoundEffect("MOVEMENT_MUD_STEP") 
    self:SnapToGrid()
    if (self:InputSameAsDirection() == true) then
      if (self.mInput:InputRun()) then self:WalkToRun()
      elseif (self.mInput:InputSneak()) then self:WalkToSneak() end
    end
  elseif self:FrameIs(2) or self:FrameIs(11) then
    --if (OnGround() == false) then ToFalling()
    if (self:InputSameAsDirection() == false) then if self:FrameIs(2) then self:ToStand() else self:ToStand2() end
    end
  end
end

function Abe:WalkToRun()
  self:SetAndWaitForAnimationComplete('AbeWalkingToRunning')
  self:SetXVelocity(0)
  self:SetXSpeed(6.25)
  self:Next(self.Run)
end

function Abe:WalkToSneak()
  self:SetXSpeed(2.777771)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete('AbeWalkingToSneaking') 
  self:Next(self.Sneak)
end

function Abe:StandToRun()
  self:SetXSpeed(6.25)
  self:SetXVelocity(0)
  self:SetAndWaitForAnimationComplete('AbeStandToRun')
  self:Next(self.Run)
end

function Abe:Run()
  self:SetAnimation("AbeRunning")
  if self:FrameIs(0) then
    print("Use 12") -- TODO: Is also applied to frame 1
    self:SetXSpeed(12)
  else
    self:SetXSpeed(6.25)
  end
  
  if self:FrameIs(4) or self:FrameIs(12) then
    if (self:InputSameAsDirection()) then 
      if self.mInput:InputRun() == false then self:RunToWalk() end
    else
      self:RunSkidStop()
    end
  end
end

function Abe:RunSkidStop()
  self:SetXVelocity(0.375)
  self:SetAndWaitForAnimationComplete('AbeRunningSkidStop') -- TODO: Wait for frame 14
  self:SnapToGrid()
  self:Next(self.Stand)
end

function Abe:RunToWalk()
  self:SetXSpeed(2.777771)
  -- TODO: On frame 9!
  self:SetAndWaitForAnimationComplete("AbeRunningToWalkingMidGrid")
  self:SetXVelocity(0)
  self.Walk()
end

function Abe:StandToSneak()
  print("TODO: StandToSneak")
  self:Next(self.Sneak)
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
  self:SetXSpeed(2.7)
  self:SetXVelocity(0)
  -- TODO: Can also be AbeSneakingToWalkingMidGrid
  self:SetAndWaitForAnimationComplete("AbeSneakingToWalking")
  self:Next(self.Walk)
end

function Abe:TurnAround() 
  PlaySoundEffect("GRAVEL_SMALL") -- TODO: Add to json
  self:SetAndWaitForAnimationComplete('AbeStandTurnAround') 
  self:FlipXDirection()
  self:Stand() 
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

function Abe:Next(func) self.mNextFunction = func end

function Abe:CoRoutineProc()
  local frameChanged = false
  while true do
    if frameChanged or self.mAnimChanged then
      self.mAnimChanged = false
      self:mNextFunction()
      self:ApplyMovement()
    end
    if self.mAnimChanged == false then
      frameChanged = self.mApi:AnimUpdate()
    end
    coroutine.yield()
  end
end

oldX = 0
function Abe:Update() 
  --print("+Update") 
  coroutine.resume(self.mThread, self) 
  --print("-Update")
  if oldX ~= self.mApi.mXPos then
    local delta = self.mApi.mXPos - oldX
    if delta ~= self.mApi.mXPos then
      print("DELTA: " .. delta .. " on frame: " .. self.mApi:FrameNumber() .. " XPOS: " .. self.mApi.mXPos)
    end
  end
  oldX = self.mApi.mXPos
end

function Abe.create()
   local ret = {}
   setmetatable(ret, Abe)
   ret.mNextFunction = ret.Stand
   ret.mLastAnimationName = ""
   ret.mXSpeed = 0
   ret.mXVelocity = 0
   ret.mAnimChanged = false
   ret.mThread = coroutine.create(ret.CoRoutineProc)
   return ret
end

-- C++ call points
function init(cppObj)
    cppObj.states = Abe.create()
    cppObj.states.mApi = cppObj
    cppObj.states:SetAnimation('AbeStandIdle')
end

function update(cppObj, input)
    cppObj.states.mInput = input
    cppObj.states:Update()
end
