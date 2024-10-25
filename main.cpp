#include "game.h"
#include "game_utils.h"

int main()
{
#if DEBUG
    Game game;
    game.run();
#else
    try
    {
        GameEngine game;
        game.run();
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
