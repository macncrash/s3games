// S3 LOT
//   s3lot            play
//   s3lot --sim      autopilot clears the lot
//   s3lot --version
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lot.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    lot::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    const int limit = 60 * 50;
    int frames = 0;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won()) {
        std::printf("S3 LOT  WIN  three targets, then the gate, %.1fs left on the clock\n", cart.clockLeft());
        return 0;
    }
    std::printf("S3 LOT  FAIL  targets %d/3  gate %s  clock %.1fs\n", cart.hits(), cart.gateOpen() ? "open" : "shut",
                cart.clockLeft());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 LOT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lot [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<lot::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
