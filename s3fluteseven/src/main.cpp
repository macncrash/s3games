// S3 FLUTE SEVEN
//   s3fluteseven            a short flute, done when first to seven
//   s3fluteseven --sim      autopilot is first to seven and leaves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/seven.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    fluteseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.left() && cart.you() >= fluteseven::kSeven && cart.them() < fluteseven::kSeven) {
        std::printf("S3 FLUTE SEVEN  WIN  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FLUTE SEVEN  SHORT  %s  you %d  them %d  (%.1f s)\n", cart.over() ? cart.why() : "timed out",
                cart.you(), cart.them(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FLUTE SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fluteseven [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<fluteseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
