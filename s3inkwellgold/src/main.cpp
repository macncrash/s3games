// S3 INKWELL GOLD
//   s3inkwellgold                 dip until only the gold counts double
//   s3inkwellgold --sim           autopilot leaves on a gold double
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/inkwellgold.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    inkwellgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.goldOut() && cart.score() >= cart.line() && cart.bare() < cart.line() && cart.golds() > 0 &&
        cart.cream() == 0) {
        std::printf(
            "S3 INKWELL GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  line %d  "
            "(%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.line(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 INKWELL GOLD  SHORT  score %d  bare %d  golds %d  cream %d  line %d  (%.1f s)\n", cart.score(),
                cart.bare(), cart.golds(), cart.cream(), cart.line(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 INKWELL GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3inkwellgold [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<inkwellgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
