// S3 HELIKILO
//   s3helikilo            play
//   s3helikilo --sim      autopilot finishes the kilometer
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/heli.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    heli::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won()) {
        std::printf("%s\n", cart.result());
        std::fflush(stdout);
        return 0;
    }
    if (cart.result()[0])
        std::printf("%s\n", cart.result());
    else
        std::printf("S3 HELIKILO  FAIL  unfinished  %d m  (%.1f s)\n", cart.meters(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 HELIKILO %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3helikilo [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<heli::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
