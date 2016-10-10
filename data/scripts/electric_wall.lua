local ElectricWall = {}
ElectricWall.__index = ElectricWall
ElectricWall.mName = "ElectricWall"

function ElectricWall.create(cppObj, rect, stream)
  local ret = {}
  setmetatable(ret, ElectricWall)
  
  cppObj.mXPos = rect.x
  cppObj.mYPos = rect.y
  
  ret.mScale = stream:ReadU16()
  if (ret.mScale == 1) then
    print("Warning: electric_wall scale not implemented")
  end 
  
  ret.mId = stream:ReadU16()
  ret.mEnabled = stream:ReadU16()
   
  print("WALL ID IS " .. ret.mId .. " START ON IS " .. ret.mEnabled .. " SCALE IS " .. ret.mScale)

  return ret
end

function ElectricWall:SetAnimation(anim)
  if self.mLastAnim ~= anim then
    self.mApi:SetAnimation(anim)
    self.mLastAnim = anim
  end
end

function ElectricWall:Update()
  if self.mEnabled then
    self:SetAnimation("ELECWALL.BAN_6000_AePc_0")
    self.mApi:AnimUpdate()
  else
    self:SetAnimation(nil)
  end
end

objects.ae[38] = ElectricWall.create
