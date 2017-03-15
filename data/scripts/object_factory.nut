include("base_map_object.nut");

include("hoist.nut");
include("door.nut");
include("background_animation.nut");
include("switch.nut");
include("mine.nut");
include("electric_wall.nut");
include("slam_door.nut");

enum ObjectTypesAe
{
    Hoist = 2,
    Door = 5,
    BackgroundAnimation = 13,
    Switch = 17,
    Mine = 24,
    ElectricWall = 38,
    SlamDoor = 85
}


function init_object_factory()
{
    objects <- {};
    objects.ae <- 
    {
       [ObjectTypesAe.Hoist] = Hoist,
       [ObjectTypesAe.Door] = Door,
       [ObjectTypesAe.BackgroundAnimation] = BackgroundAnimation,
       [ObjectTypesAe.Switch] = Switch,
       [ObjectTypesAe.Mine] = Mine,
       [ObjectTypesAe.ElectricWall] = ElectricWall,
       [ObjectTypesAe.SlamDoor] = SlamDoor
    };

    objects.ao <- 
    {

    };
}

function object_factory(/*MapObject*/ mapObj, /*GridMap*/ map, isAo, typeId, /*ObjRect*/ rect, /*IStream*/ stream)
{
    if (isAo)
    {
        log_error("AO objects not implemented");
        return null;
    }
 
    if (typeId in objects.ae) 
    {
        local factory = objects.ae[typeId];
        log_info("Constructing object for type " + typeId);
        local obj = factory(mapObj, map, rect, stream);
        log_info("Setting instance");
        mapObj.SetScriptInstance(obj); // Store the squirrel object instance ref in the C++ object
        log_info("Returning");
        return true;
    }

    log_info("No factory found for object type " + typeId);
    return false;
}
