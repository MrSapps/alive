include("door.nut");
include("switch.nut");
include("electric_wall.nut");

enum ObjectTypesAe
{
    Door = 5,
    Switch = 17,
    ElectricWall = 38
}

function init_object_factory()
{
    objects <- {};
    objects.ae <- 
    {
       [ObjectTypesAe.Door] = Door,
       [ObjectTypesAe.Switch] = Switch,
       [ObjectTypesAe.ElectricWall] = ElectricWall
    };

    objects.ao <- 
    {

    };
}

function object_factory(/*MapObject*/ mapObj, isAo, typeId, /*ObjRect*/ rect, /*IStream*/ stream)
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
        local obj = factory(mapObj, rect, stream);
        log_info("Setting instance");
        mapObj.SetScriptInstance(obj); // Store the squirrel object instance ref in the C++ object
        log_info("Returning");
        return true;
    }

    log_info("No factory found for object type " + typeId);
    return false;
}

