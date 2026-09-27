// S3 TABLE GOLD
//   s3tablegold                 play the short table
//   s3tablegold --sim           autopilot, exits 0 only on that double
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/tablegold.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    tablegold::Game cart;
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
        bare < cart.line() && cart.goals() >= 1) {
        std::printf(
            "S3 TABLE GOLD  DOUBLE  only the gold counts double  gold %d  cream %d  score %d  goals %d  (%.1f s)\n",
            cart.gold(), cart.cream(), cart.score(), cart.goals(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 TABLE GOLD  NO DOUBLE  gold %d  cream %d  score %d  goals %d  %s %s  (%.1f s)\n", cart.gold(),
                cart.cream(), cart.score(), cart.goals(), cart.phase(), cart.say(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TABLE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tablegold [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<tablegold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
