// S3 MUSHPASS
//   s3mushpass            play
//   s3mushpass --sim      the team clears the pass
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/pass.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    mushpass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won())
        std::printf("S3 MUSHPASS  WIN  pass clear  storm %.1fs left  (%.1f s)\n", cart.storm(), frames / 60.0);
    else
        std::printf("S3 MUSHPASS  FAIL  legs %d  storm %.1fs  (%.1f s)\n", cart.legs(), cart.storm(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MUSHPASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mushpass [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<mushpass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
