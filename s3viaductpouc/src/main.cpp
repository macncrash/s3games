// VIADUCT POUCH
//   s3viaductpouc            play
//   s3viaductpouc --sim      the courier crosses with the pouch
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/pouch.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    pouch::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const bool win = cart.won();
    std::printf("VIADUCT POUCH  %s  carried across  (%.1f s)\n", win ? "PASS" : "FAIL", frames / 60.0);
    return win ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("VIADUCT POUCH %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3viaductpouc [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<pouch::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
