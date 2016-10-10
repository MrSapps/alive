local SlamDoor = {}
SlamDoor.__index = SlamDoor

function SlamDoor.create(cppObj, rect, stream)
  local ret = {}
  setmetatable(ret, SlamDoor)
  
  cppObj.mXPos = rect.x + 13
  cppObj.mYPos = rect.y + 18
  
  ret.mClosed = stream:ReadU16()   
  ret.mScale = stream:ReadU16()
  ret.mId = stream:ReadU16()
  ret.mInverted = stream:ReadU16()
  ret.mDelete = stream:ReadU32()

  return ret
end

function SlamDoor:SetAnimation(anim)
  if self.mLastAnim ~= anim then
    self.mApi:SetAnimation(anim)
    self.mLastAnim = anim
  end
end

function SlamDoor:Update()
  if self.mClosed then
    -- TODO: Handle closed to opening etc
    self:SetAnimation("SLAM.BAN_2020_AePc_1")
    self.mApi:AnimUpdate()
  else
    self:SetAnimation(nil)
  end
end

objects.ae[85] = SlamDoor.create
