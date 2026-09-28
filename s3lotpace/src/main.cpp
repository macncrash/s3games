// S3 LOTPACE
//   s3lotpace            play the lot
//   s3lotpace --sim      wait for the third pace, then fire
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/lot.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    lot::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (!cart.won() || cart.pace() != 3) {
        std::printf("S3 LOTPACE  FAIL  pace %d  (%.1f s)\n", cart.pace(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 LOTPACE  WIN  third pace  (%.1f s)\n", frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 LOTPACE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lotpace [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<lot::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
