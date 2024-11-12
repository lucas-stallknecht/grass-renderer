#include "src/Engine.h"

int main()
{
    grass::Engine engine;
    if (!engine.init()) return 1;

    engine.run();

    engine.cleanup();
    return 0;
}
