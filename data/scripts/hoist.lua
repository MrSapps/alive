local Hoist = {}
Hoist.__index = Hoist

function Hoist.create(cppObj, rect, stream)
  local ret = {}
  setmetatable(ret, Hoist)
  ret.mHoistType = stream:ReadU16()
  ret.mEdgeType = stream:ReadU16()
  ret.mId = stream:ReadU16()
  ret.mScale = stream:ReadU16()
  return ret
end

function Hoist:Update()
  --print("Hoist:Update") 
end

objects.ae[2] = Hoist.create
