// S3 CUE SEVEN
//   s3cueseven                 play cue until first to seven
//   s3cueseven --sim           autopilot, exits 0 only when you are first to seven
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/cueseven.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    cueseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.rules() && cart.won() && cart.you() >= 7 && cart.them() < 7) {
        std::printf("S3 CUE SEVEN  PASS  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 CUE SEVEN  SHORT  you %d  them %d  rules %d  (%.1f s)\n", cart.you(), cart.them(),
                cart.rules() ? 1 : 0, frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CUE SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3cueseven [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<cueseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
