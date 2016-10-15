local Mine = {}
Mine.__index = Mine
Mine.mName = "Mine"

function Mine.create(cppObj, rect, stream)
  local ret = {}
  setmetatable(ret, Mine)
  
  cppObj.mXPos = rect.x + 10
  cppObj.mYPos = rect.y + 22
  
  stream:ReadU32() -- Skip unused "num patterns"
  stream:ReadU32() -- Skip unused "patterns"
  
  ret.mScale= stream:ReadU32()
  if (ret.mScale == 1) then
     print("Warning: Mine background scale not implemented")
  end
  
  return ret
end

function Mine:SetAnimation(anim)
  if self.mLastAnim ~= anim then
    self.mApi:SetAnimation(anim)
    self.mLastAnim = anim
  end
end

function Mine:Update()
  self:SetAnimation("LANDMINE.BAN_1036_AePc_0")
  self.mApi:AnimUpdate()
end

objects.ae[24] = Mine.create
