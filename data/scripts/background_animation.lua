local BackgroundAnimation = {}
BackgroundAnimation.__index = BackgroundAnimation
BackgroundAnimation.mName = "BackgroundAnimation"

function BackgroundAnimation.create(cppObj, rect, stream)
  local ret = {}
  setmetatable(ret, BackgroundAnimation)
  local animId = stream:ReadU32()
  local animName = nil
  if (animId == 1201) then
    animName = "BAP01C06.CAM_1201_AePc_0"
    cppObj.mXPos = cppObj.mXPos + 5
    cppObj.mYPos = cppObj.mYPos + 3
  elseif (animId == 1202) then
    animName = "FARTFAN.BAN_1202_AePc_0"
  else
    animName = "AbeStandSpeak1"
  end

  print("Id is " .. animId .. " animation is " .. animName)
  
  cppObj:SetAnimation(animName) 
  return ret
end

function BackgroundAnimation:Update()
  self.mApi:AnimUpdate()
end

objects.ae[13] = BackgroundAnimation.create
