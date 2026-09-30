// S3 CULVERTWELL
//   s3culvertwell            play
//   s3culvertwell --sim      the keeper holds the well through three waves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/well.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    culvertwell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    std::printf("S3 CULVERTWELL  %s  the well %s  wave %d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.won() ? "stands" : "fell", cart.wave() + 1, frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CULVERTWELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3culvertwell [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<culvertwell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
