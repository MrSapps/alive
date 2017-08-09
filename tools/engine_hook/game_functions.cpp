#include "game_functions.hpp"

GameFunctions& Funcs()
{
    static GameFunctions f;
    return f;
}

GameVars& Vars()
{
    static GameVars v;
    return v;
}
