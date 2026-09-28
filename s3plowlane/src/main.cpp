// S3 PLOWLANE
//   s3plowlane            play
//   s3plowlane --sim      the plow stays in the lane and beats the other crew
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/plow.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    plow::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    std::printf("S3 PLOWLANE  %s  leg %.1fs  crew %.1fs\n", cart.won() ? "PASS" : "FAIL", cart.legTime(),
                cart.crewTime());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PLOWLANE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3plowlane [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<plow::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
