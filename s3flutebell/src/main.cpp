// S3 FLUTE BELL
//   s3flutebell            play the flute until the bell
//   s3flutebell --sim      autopilot rings the bell before the third try dies
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/flutebell.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    flutebell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.rung() && cart.dead() < 3 && cart.notes() >= flutebell::Game::kMarks) {
        std::printf("S3 FLUTE BELL  PASS  bell rang before the third try died  dead %d  notes %d  (%.1f s)\n",
                    cart.dead(), cart.notes(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.why() : "timed out";
    std::printf("S3 FLUTE BELL  FAIL  %s  dead %d  notes %d  (%.1f s)\n", why, cart.dead(), cart.notes(),
                frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FLUTE BELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3flutebell [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<flutebell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
