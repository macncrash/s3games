// S3 HELIGRASS
//   s3heligrass            play
//   s3heligrass --sim      autopilot lands on the grass and stops
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/heli.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    heli::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    std::printf("S3 HELIGRASS  %s  landed on the grass  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 HELIGRASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3heligrass [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<heli::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
