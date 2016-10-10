local Door = {}
Door.__index = Door
Door.mName = "Door"

function Door.create(cppObj, rect, stream)
  local ret = {}
  setmetatable(ret, Door)
  
  local level = stream:ReadU16()
  local path = stream:ReadU16()
  local camera = stream:ReadU16()
  local scale = stream:ReadU16()
  if (scale == 1) then
    print("WARNING: Half scale not supported")
  end
  local doorNumber = stream:ReadU16()
  ret.mId = stream:ReadU16()
  local targetDoorNumber = stream:ReadU16()
  local skin = stream:ReadU16()
  local startOpen = stream:ReadU16()

  local hubId1 = stream:ReadU16()
  local hubId2 = stream:ReadU16()
  local hubId3 = stream:ReadU16()
  local hubId4 = stream:ReadU16()
  local hubId5 = stream:ReadU16()
  local hubId6 = stream:ReadU16()
  local hubId7 = stream:ReadU16()
  local hubId8 = stream:ReadU16()

  local wipeEffect = stream:ReadU16()
  local xoffset = stream:ReadU16()
  local yoffset = stream:ReadU16()

  local wipeXStart = stream:ReadU16()
  local wipeYStart = stream:ReadU16()

  local abeFaceLeft = stream:ReadU16()
  local closeAfterUse = stream:ReadU16()

  local removeThrowables = stream:ReadU16()
    
  cppObj.mXPos = rect.x + xoffset + 5
  cppObj.mYPos = rect.y + rect.h + yoffset

  return ret
end

function Door:SetAnimation(anim)
  if self.mLastAnim ~= anim then
    self.mApi:SetAnimation(anim)
    self.mLastAnim = anim
  end
end


function Door:Update()
  -- AE door types
  self:SetAnimation("DoorClosed_Barracks") -- DoorToClose_Barracks have to play this in reverse to get DoorToOpen_Barracks
  --"DOOR.BAN_2012_AePc_0" -- mines
  --"SHDOOR.BAN_2012_AePc_0" -- wooden mesh door
  --"TRAINDOR.BAN_2013_AePc_0" -- feeco train door - not aligned!
  --"BRDOOR.BAN_2012_AePc_0" -- brewer door, not aligned
  --"BWDOOR.BAN_2012_AePc_0" -- bone werkz door
  --"STDOOR.BAN_8001_AePc_0" -- blank? 
  --"FDDOOR.BAN_2012_AePc_0" -- feeco normal door
  --"SVZDOOR.BAN_2012_AePc_0" -- scrab vault door

  self.mApi:AnimUpdate()
  
  -- DoorToClose_Barracks
  -- TODO: Open anim is close in reverse
  -- PlaySoundEffect("FX_DOOR")
end

objects.ae[5] = Door.create
