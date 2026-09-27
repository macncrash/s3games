// S3 MAZETAPE
//   s3mazetape                 walk until the drawer matches the tape
//   s3mazetape --sim           autopilot fills the drawer and leaves
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/mazetape.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    mazetape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 MAZETAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    bool names = std::strcmp(cart.tapeLabel(0), "KEY") == 0 && std::strcmp(cart.tapeLabel(1), "BELL") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "LAMP") == 0;
    bool scores = cart.tapeScore(0) == 4 && cart.tapeScore(1) == 6 && cart.tapeScore(2) == 8;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && names && scores && cart.held(0) &&
        cart.held(1) && cart.held(2) && cart.traps() == 0 && cart.drawerScore() == 18 && cart.steps() > 0) {
        std::printf("S3 MAZETAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MAZETAPE  FAIL  %s  %s  steps %d  till %d  traps %d  (%.1f s)\n", cart.modeName(), cart.reason(),
                cart.steps(), cart.drawerScore(), cart.traps(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MAZETAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mazetape [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<mazetape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
