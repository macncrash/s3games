// S3 SCULL PASS
//   s3scullpass                 clear the pass before the storm clock
//   s3scullpass --sim           the scull makes the end before the clock dies
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/pass.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    scullpass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 SCULL PASS  FAIL  %s  x %.1f  y %.1f  hdg %.2f  spd %.2f  (%.1f s)\n", why, cart.x(), cart.y(),
                    cart.heading(), cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    int left = int(cart.stormLeft() + 0.5f);
    if (left < 0) left = 0;
    std::printf("S3 SCULL PASS  CLEAR  cleared the pass before the storm clock  %02d:%02d left\n", left / 60, left % 60);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SCULL PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scullpass [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<scullpass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
