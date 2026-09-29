// S3 SUBSLIP
//   s3subslip            play
//   s3subslip --sim      the boat berths itself before the tide turns
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/slip.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    slip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won()) {
        std::printf("S3 SUBSLIP  berthed in the slip  %ds before the tide turned\n", cart.tideLeft());
        return 0;
    }
    std::printf("S3 SUBSLIP  missed the slip  hull %d\n", cart.hull());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SUBSLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3subslip [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<slip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
