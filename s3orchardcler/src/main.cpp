// S3 ORCHARDCLER
//   s3orchardcler            play
//   s3orchardcler --sim      rake the orchard clear before the clock dies
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/orchard.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    orchard::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const char* verdict = cart.won() ? "WIN" : "FAIL";
    std::printf("S3 ORCHARDCLER %s  ground clear before the clock  left %d  %.1fs on the clock\n", verdict,
                cart.left(), cart.secondsLeft());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 ORCHARDCLER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3orchardcler [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<orchard::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
