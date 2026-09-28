// S3 SCULLLOCK
//   s3sculllock            play
//   s3sculllock --sim      the scull takes every lock
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/lock.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    scull::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won())
        std::printf("S3 SCULLLOCK  WIN  legs %d  gates clear  end made  (%.1f s)\n", cart.legs(), frames / 60.0);
    else
        std::printf("S3 SCULLLOCK  FAIL  legs %d  lives %d  (%.1f s)\n", cart.legs(), cart.lives(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SCULLLOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sculllock [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<scull::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
