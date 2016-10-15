objects = {}
objects.ae = {}
objects.ao = {}

require("background_animation")
require("door")
require("electric_wall")
require("hoist")
require("mine")
require("slam_door")
require("switch")

function object_factory(cppObj, isAo, typeId, rect, stream)
  if isAo then
    print("AO objects not implemented")
    return false 
  end

  local factory = objects.ae[typeId]
  if factory then
    print("Constructing object for type " .. typeId)
    cppObj.states = factory(cppObj, rect, stream)
    cppObj.states.mApi = cppObj
    return true
  end
  
  print("No factory found for object type " .. typeId)
  return false
end

function update(cppObj, input)
    cppObj.states.mInput = input
    cppObj.states:Update()
end
