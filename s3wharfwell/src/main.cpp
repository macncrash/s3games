// S3 WHARFWELL
//   s3wharfwell            play
//   s3wharfwell --sim      the keeper holds the well through three waves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/wharf.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    wharf::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const bool pass = cart.won();
    std::printf("S3 WHARFWELL  %s  the well stands  wave %d  well %d  score %d  (%.1f s)\n", pass ? "PASS" : "FAIL",
                cart.wave() + 1, cart.well(), cart.score(), frames / 60.0);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 WHARFWELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wharfwell [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<wharf::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
