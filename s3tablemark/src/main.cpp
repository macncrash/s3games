// S3 TABLEMARK
//   s3tablemark            slide the puck until it finishes on the mark
//   s3tablemark --sim      autopilot finishes the mark
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/tablemark.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    tablemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.finished()) {
        std::printf("S3 TABLEMARK  FINISHED MARK  on the mark  shots %d  (%.1f s)\n", cart.shots(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 TABLEMARK  OPEN  no finished mark  shots %d  (%.1f s)\n", cart.shots(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TABLEMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tablemark [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<tablemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
