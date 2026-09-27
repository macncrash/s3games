// S3 GOLFTAPE
//   s3golftape            play a short golf
//   s3golftape --sim      putt until the drawer matches the tape
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/golftape.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    golftape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 GOLFTAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    bool names = std::strcmp(cart.tapeLabel(0), "FADE") == 0 && std::strcmp(cart.tapeLabel(1), "PITCH") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "DROP") == 0;
    bool scores = cart.tapeScore(0) == 4 && cart.tapeScore(1) == 3 && cart.tapeScore(2) == 1;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && names && scores && cart.shots() == 3 &&
        cart.drawerScore() == 8 && cart.board() == 8 && cart.traps() == 0 && cart.held(0) && cart.held(1) &&
        cart.held(2)) {
        std::printf(
            "S3 GOLFTAPE  PASS  the drawer matches the tape (%s, %s, %s) and the short golf is closed (%.1f s)\n",
            cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 GOLFTAPE  FAIL  %s  %s  shots %d  till %d  board %d  traps %d  %d%d%d  (%.1f s)\n", cart.modeName(),
                cart.reason(), cart.shots(), cart.drawerScore(), cart.board(), cart.traps(), cart.held(0) ? 1 : 0,
                cart.held(1) ? 1 : 0, cart.held(2) ? 1 : 0, frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GOLFTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3golftape [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<golftape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
