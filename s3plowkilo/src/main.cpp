// S3 PLOW KILO
//   s3plowkilo            play
//   s3plowkilo --sim      the plow finishes the kilometer
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/plow.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    plow::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (!cart.won()) {
        if (!cart.over()) std::printf("S3 PLOW KILO  FAIL  timed out at %d m\n", cart.meters());
        std::fflush(stdout);
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PLOW KILO %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3plowkilo [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<plow::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
