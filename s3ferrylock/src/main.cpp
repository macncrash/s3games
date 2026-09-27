// S3 FERRY LOCK
//   s3ferrylock            pass the lock
//   s3ferrylock --sim      the autopilot must clear both gates
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/ferry.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    ferrylock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 110;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 FERRY LOCK  FAIL  %s  x %.1f  y %.1f  (%.1f s)\n", why, cart.x(), cart.y(), frames / 60.0);
        return 1;
    }
    std::printf("S3 FERRY LOCK  CLEAR  passed the lock without scraping a gate  (%.1f s)\n", cart.seconds());
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FERRY LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ferrylock [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<ferrylock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
