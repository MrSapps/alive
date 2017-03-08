enum ObjectTypesAe
{
    Switch = 17
}

function init_object_factory()
{
    objects <- {};
    objects.ae <- 
    {
       [ObjectTypesAe.Switch] = Switch.create
    };

    objects.ao <- 
    {

    };
}

function object_factory(xpos, ypos, isAo, typeId, /*ObjRect*/ rect, /*IStream*/ stream)
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
        local obj = factory(xpos, ypos, rect, stream);
		log_info("Setting instance");
		obj.SetScriptInstance(obj); // Store the squirrel object instance ref in the C++ object
		log_info("Returning");
		return obj;
    }

    log_info("No factory found for object type " + typeId);
    return null;
}

