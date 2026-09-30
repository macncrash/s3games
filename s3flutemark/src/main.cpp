// S3 FLUTEMARK
//   s3flutemark            play the flute until a finished mark ends it
//   s3flutemark --sim      autopilot lands the phrase and leaves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/flutemark.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    flutemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (cart.won() && cart.finished()) {
        std::printf("S3 FLUTEMARK  FINISHED MARK  notes %d/%d  misses %d  (%.1f s)\n", cart.notes(),
                    flutemark::Game::kMarks, cart.misses(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FLUTEMARK  OPEN  no finished mark  notes %d/%d  misses %d  (%.1f s)\n", cart.notes(),
                flutemark::Game::kMarks, cart.misses(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FLUTEMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3flutemark [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<flutemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
