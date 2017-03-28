include("abe.nut");

function on_init_map(playerMapObj, map, xpos, ypos)
{
    log_info("on_init_map for map creating player object map is " + map);
    local player = Abe(playerMapObj, map);
    playerMapObj.SetScriptInstance(player);

    player.mBase.mXPos = xpos;
    player.mBase.mYPos = ypos;

    // TODO: Tell camera to follow the player
  
    return true;
}
