// S3 SALLY DAWN
//   s3sallydawn         keep the flares lit until morning
//   s3sallydawn --sim   the sentry walks the sally alone
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/sally.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    sally::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won()) {
        std::printf("S3 SALLY DAWN  WIN  the flares held until dawn  (%.1f s)\n", frames / 60.0);
        return 0;
    }
    std::printf("S3 SALLY DAWN  FAIL  a flare went dark  (%.1f s)\n", frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SALLY DAWN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sallydawn [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<sally::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
