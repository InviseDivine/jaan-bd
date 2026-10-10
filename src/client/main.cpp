#include <Game.hpp>
#define RAYTMX_IMPLEMENTATION
#include "raytmx.h"
int main() {
    Game::get().run();
    return 0;
}