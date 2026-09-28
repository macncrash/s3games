// S3 LOT WELL
//   s3lotwell            play
//   s3lotwell --sim      autopilot keeps the well standing
//   s3lotwell --version
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/well.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    lotwell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    const int limit = 60 * 90;
    int frames = 0;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won()) {
        std::printf("S3 LOT WELL  WIN  the well stands through three waves\n");
        return 0;
    }
    std::printf("S3 LOT WELL  FAIL  wave %d  cracks %d  score %d\n", cart.wave() + 1, cart.cracks(), cart.score());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 LOT WELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lotwell [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<lotwell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
