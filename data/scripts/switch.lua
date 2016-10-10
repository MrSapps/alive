local Switch = {}
Switch.__index = Switch

function Switch.create(cppObj, rect, stream)
  local ret = {}
  setmetatable(ret, Switch)
  
  cppObj.mXPos = rect.x + 37
  cppObj.mYPos = rect.y + rect.h - 5
  
  ret.mScale= stream:ReadU32()
  if (ret.mScale == 1) then
     print("WARNING: Half scale not supported")
  end
  
  local onSound = stream:ReadU16()
  local offSound = stream:ReadU16()
  local soundDirection = stream:ReadU16()

  -- The ID of the object that this switch will apply "targetAction" to
  ret.mId = stream:ReadU16()
    
  return ret
end

function Switch:SetAnimation(anim)
  if self.mLastAnim ~= anim then
    self.mApi:SetAnimation(anim)
    self.mLastAnim = anim
  end
end

function Switch:Activate(direction)
  print("TODO: Activate")
  -- TODO: Activate objects with self.mId
end

function Switch:Update()
  self:SetAnimation("SwitchIdle")
  -- SwitchActivateLeft
  -- SwitchDeactivateLeft
  -- SwitchActivateRight
  -- SwitchDeactivateRight
  -- PlaySoundEffect("FX_LEVER")
  self.mApi:AnimUpdate()
end

objects.ae[17] = Switch.create
