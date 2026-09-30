// S3 HELISLIP
//   s3helislip            play
//   s3helislip --sim      autopilot berths in the slip before the tide
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
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    std::printf("S3 HELISLIP  %s  berthed in the slip  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 HELISLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3helislip [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<slip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
