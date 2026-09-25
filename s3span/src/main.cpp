// S3 SPAN
//   s3span            play
//   s3span --sim      autopilot holds the span; exit 0 only if the column crosses
//   s3span --version
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/span.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    span::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won()) {
        std::printf("S3 SPAN  WIN  the column is across  %d of %d  (%.1f s)\n", cart.across(), cart.column(),
                    cart.seconds());
        return 0;
    }
    std::printf("S3 SPAN  FAIL  the span gave way  %d of %d across  (%.1f s)\n", cart.across(), cart.column(),
                cart.seconds());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SPAN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3span [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<span::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
