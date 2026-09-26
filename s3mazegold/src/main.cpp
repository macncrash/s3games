// S3 MAZE GOLD
//   s3mazegold                 walk the hedge until the gold double pays, then leave
//   s3mazegold --sim           autopilot leaves only when that is true
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/mazegold.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    mazegold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const int bare = cart.gold() + cart.cream();
    const bool math = cart.score() == cart.gold() * 2 + cart.cream();
    if (cart.won() && cart.left() && cart.atExit() && cart.finisherGold() && math && cart.gold() >= 1 &&
        cart.cream() >= 1 && cart.score() >= cart.line() && bare < cart.line() && cart.steps() > 8) {
        std::printf("S3 MAZE GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  steps %d  (%.1f s)\n",
                    cart.gold(), cart.cream(), cart.score(), cart.steps(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MAZE GOLD  STAY  gold %d  cream %d  score %d  steps %d  %s  (%.1f s)\n", cart.gold(), cart.cream(),
                cart.score(), cart.steps(), cart.say(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MAZE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mazegold [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<mazegold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
