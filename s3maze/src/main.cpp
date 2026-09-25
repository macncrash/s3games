// S3 MAZE
//   s3maze            walk the hedge
//   s3maze --sim      autopilot takes the path out
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/maze.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    maze::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (!cart.won() || cart.steps() < 1) {
        std::printf("S3 MAZE  FAIL  still in the hedge  steps %d  (%.1f s)\n", cart.steps(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 MAZE  OUT  the exit was the only win  steps %d  (%.1f s)\n", cart.steps(), frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MAZE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3maze [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<maze::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
