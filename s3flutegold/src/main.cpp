// S3 FLUTE GOLD
//   s3flutegold            play the flute until only the gold counts double
//   s3flutegold --sim      autopilot leaves on the gold double
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/flutegold.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    flutegold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.finisherGold() && cart.score() >= cart.line() && cart.bare() < cart.line() &&
        cart.golds() == flutegold::kGolds && cart.cream() == flutegold::kCreams) {
        std::printf(
            "S3 FLUTE GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  notes %d  "
            "line %d  (%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.notes(), cart.line(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FLUTE GOLD  SHORT  score %d  bare %d  golds %d  cream %d  notes %d  line %d  (%.1f s)\n",
                cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.notes(), cart.line(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FLUTE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3flutegold [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<flutegold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
