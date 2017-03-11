include("abe.nut");

function on_init_map(playerMapObj, xpos, ypos)
{
    log_info("on_init_map for map creating player object");
    local player = Abe(playerMapObj);
    playerMapObj.SetScriptInstance(player);

    player.mBase.mXPos = xpos;
    player.mBase.mYPos = ypos;

    // TODO: Tell camera to follow the player
  
    return true;
}
