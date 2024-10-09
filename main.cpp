#include "src/Engine.h"

int main()
{
    grass::Engine engine;
    engine.init();
    engine.run();
    engine.cleanup();
    return 0;
}
