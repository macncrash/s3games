// S3 GOLF GOLD
//   s3golfgold                 putt until only the gold counts double
//   s3golfgold --sim           autopilot, exits 0 only on that double
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/golfgold.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    golfgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const bool math = cart.score() == cart.gold() * 2 + cart.cream();
    const int bare = cart.bare();
    if (cart.won() && cart.finisherGold() && math && cart.gold() >= 1 && cart.score() >= cart.line() &&
        bare < cart.line() && cart.shots() >= 1) {
        std::printf("S3 GOLF GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  shots %d  (%.1f s)\n",
                    cart.gold(), cart.cream(), cart.score(), cart.shots(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 GOLF GOLD  NO DOUBLE  gold %d  cream %d  score %d  shots %d  %s %s  (%.1f s)\n", cart.gold(),
                cart.cream(), cart.score(), cart.shots(), cart.phase(), cart.say(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GOLF GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3golfgold [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<golfgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
