// S3 CLIFFTURN
//   s3cliffturn            play
//   s3cliffturn --sim      the other crew's clock, three turns, no tip
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/cliff.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    cliff::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    std::printf("%s\n", cart.summary().c_str());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CLIFFTURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3cliffturn [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<cliff::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
