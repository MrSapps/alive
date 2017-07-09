// Syntatic helper for all other scripts to include more scripts
function include(scriptName)
{
    // Engine::Include
    gEngine.include(scriptName);
}

function load_map(mapName)
{
    gEngine.LoadMap(mapName);
}

function init()
{
    log_info("Main script init");
}
