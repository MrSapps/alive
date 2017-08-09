#include "game_functions.hpp"

namespace Utils
{
    bool IsAe()
    {
        return true;
    }
}

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
