// S3 TRENCH PURS
//   s3trenchpurs            play
//   s3trenchpurs --sim      the gun keeps itself running until the watch ends
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/purs.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    purs::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    const char* verdict = cart.won() ? "PASS" : "FAIL";
    std::printf("S3 TRENCH PURS  %s  the machine held  kills %d  breaches %d  (%.1f s)\n", verdict, cart.kills(),
                cart.breaches(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TRENCH PURS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3trenchpurs [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<purs::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
