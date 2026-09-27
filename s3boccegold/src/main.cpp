// S3 BOCCE GOLD
//   s3boccegold            one end; only a lone gold double finishes it
//   s3boccegold --sim      autopilot, exits 0 only on that double
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/boccegold.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    boccegold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.goldPts() == 2 && cart.creamPts() == 0) {
        std::printf("S3 BOCCE GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  (%.1f s)\n", cart.goldPts(),
                    cart.creamPts(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BOCCE GOLD  OPEN  gold %d  cream %d  (%.1f s)\n", cart.goldPts(), cart.creamPts(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BOCCE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3boccegold [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<boccegold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
