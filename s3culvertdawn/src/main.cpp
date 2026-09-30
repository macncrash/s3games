// S3 CULVERTDAWN
//   s3culvertdawn            play
//   s3culvertdawn --sim      the watch keeps every flare lit until dawn
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/dawn.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    culvertdawn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    std::printf("S3 CULVERTDAWN  %s  %s  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.won() ? "the flares held until dawn" : "a flare went out", frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CULVERTDAWN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3culvertdawn [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<culvertdawn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
