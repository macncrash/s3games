// S3 DART
//   s3dart            play 501, double out
//   s3dart --sim      autopilot checks out
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/dart.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    dart::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (!cart.rules() || !cart.won() || cart.left() != 0) {
        std::printf("S3 DART  FAIL  left %d  darts %d  rules %d  (%.1f s)\n", cart.left(), cart.darts(),
                    cart.rules() ? 1 : 0, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 DART  GAME SHOT  501 double-out  %s  darts %d  (%.1f s)\n", cart.out(), cart.darts(),
                frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DART %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3dart [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<dart::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
