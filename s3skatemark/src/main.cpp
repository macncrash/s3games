// S3 SKATEMARK
//   s3skatemark            skate until the mark is finished
//   s3skatemark --sim      autopilot lands the line and leaves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/skatemark.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    skatemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.finished()) {
        std::printf("S3 SKATEMARK  FINISHED MARK  line %d/%d  falls %d  (%.1f s)\n", cart.line(),
                    skatemark::Game::kTricks, cart.falls(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SKATEMARK  OPEN  no finished mark  line %d/%d  falls %d  (%.1f s)\n", cart.line(),
                skatemark::Game::kTricks, cart.falls(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SKATEMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skatemark [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<skatemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
