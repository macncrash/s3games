// S3 ORCHARDWELL
//   s3orchardwell            play
//   s3orchardwell --sim      the keeper holds the well through three waves
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/well.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    orchardwell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (!cart.won()) {
        std::printf("S3 ORCHARDWELL  FAIL  the well fell  wave %d  (%.1f s)\n", cart.wave() + 1, frames / 60.0);
        return 1;
    }
    std::printf("S3 ORCHARDWELL  the well stands through three waves\n");
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 ORCHARDWELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3orchardwell [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<orchardwell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
