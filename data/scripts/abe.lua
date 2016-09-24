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
  end
  self.mLastAnimationName = anim
end

function Abe:FlipXDirection() self.mApi:FlipXDirection() end
function Abe:FrameIs(frame) return self.mApi:FrameNumber() == frame end
function Abe:SetXSpeed(speed) end
function Abe:WaitForAnimationComplete() while self.mApi:IsLastFrame() == false do coroutine.yield() end end

function Abe:SetAndWaitForAnimationComplete(anim)
  self:SetAnimation(anim)
  self:WaitForAnimationComplete()
end

function Abe:GameSpeak(anim, soundEffect)
  self:SetAndWaitForAnimationComplete("AbeStandSpeak1")
  PlaySoundEffect(soundEffect)
  self:SetAndWaitForAnimationComplete(anim)
  self:SetAnimation('AbeStandIdle')
end

function Abe:Stand()
  self:SetAnimation('AbeStandIdle')
  if (self:InputSameAsDirection()) then self:StandToWalk()
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
  self:SetXSpeed(2.7)
  self:SetAndWaitForAnimationComplete('AbeStandToWalk')
  self:Next(self.Walk)
end

function Abe:ToStand() 
  self:SetAndWaitForAnimationComplete("AbeWalkToStand") 
  self:Next(self.Stand)
end

function Abe:Walk()
  self:SetAnimation('AbeWalking')
  if self:FrameIs(2) then
    PlaySoundEffect("MOVEMENT_MUD_STEP")
    if (self:InputSameAsDirection() == false) then self:ToStand()
    elseif (self.mInput:InputRun()) then self:WalkToRun()
    elseif (self.mInput:InputSneak()) then self:WalkToSneak()
    end
  end
end

function Abe:WalkToRun()
  self:SetAndWaitForAnimationComplete('AbeWalkingToRunning') 
  self:Next(self.Run)
end

function Abe:WalkToSneak()
  self:SetXSpeed(2.7)
  self:SetAndWaitForAnimationComplete('AbeWalkingToSneaking') 
  self:Next(self.Sneak)
end

function Abe:Run()

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
  -- TODO: Can also be AbeSneakingToWalkingMidGrid
  self:SetAndWaitForAnimationComplete("AbeSneakingToWalking")
  self:Next(self.Walk)
end

function Abe:TurnAround() 
  self:SetAndWaitForAnimationComplete('AbeStandTurnAround') 
  self:FlipXDirection()
  self:Stand() 
end

function Abe:Next(func) self.mNextFunction = func end
function Abe:CoRoutineProc()
  while true do
      self:mNextFunction()
      -- TODO: If next changed then run it before yielding?
      --print("Yeild")
      coroutine.yield()
  end
end

function Abe:Update() coroutine.resume(self.mThread, self) end

function Abe.create()
   local ret = {}
   setmetatable(ret, Abe)
   ret.mNextFunction = ret.Stand
   ret.mLastAnimationName = ""
   ret.mThread = coroutine.create(ret.CoRoutineProc)
   return ret
end

-- C++ call points
function init(cppObj)
    cppObj.states = Abe.create()
    cppObj.states.mApi = cppObj
end

function update(cppObj, input)
    cppObj.states.mInput = input
    cppObj.states:Update()
end
