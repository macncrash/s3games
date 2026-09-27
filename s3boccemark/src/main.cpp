// S3 BOCCEMARK
//   s3boccemark            one end; closest to the pallino finishes the mark
//   s3boccemark --sim      autopilot finishes the mark
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/boccemark.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    boccemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.finished()) {
        std::printf("S3 BOCCEMARK  FINISHED MARK  closest to the pallino  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BOCCEMARK  OPEN  no finished mark  (%.1f s)\n", frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BOCCEMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3boccemark [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<boccemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
