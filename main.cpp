#include "game_engine.h"

int main()
{
#if DEBUG
    GameEngine game_engine;
    game_engine.run();
#else
    try
    {
        GameEngine game_engine;
        game_engine.run();
    }
    catch(std::exception& e)
    {
        messageBox(e.what());
        return -1;
    }
    catch(...)
    {
        messageBox("Unknown error.");
        return -1;
    }
#endif

    return 0;
}
