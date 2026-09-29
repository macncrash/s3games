// S3 OVENTAPE
//   s3oventape                 play the oven until the drawer matches the tape
//   s3oventape --sim           autopilot fills the drawer and leaves
//   s3oventape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/oven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    oventape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 OVENTAPE  FAIL  rules  %s\n", cart.reason());
        std::fflush(stdout);
        return 1;
    }
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "bake.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    bool names = std::strcmp(cart.tapeLabel(0), "CRUST") == 0 && std::strcmp(cart.tapeLabel(1), "GOLD") == 0 &&
                 std::strcmp(cart.tapeLabel(2), "MILK") == 0;
    bool scores = cart.tapeScore(0) == 6 && cart.tapeScore(1) == 9 && cart.tapeScore(2) == 4;
    if (cart.won() && cart.left() && cart.matched() && cart.rules() && names && scores && cart.pulls() == 3 &&
        cart.drawerScore() == 19 && cart.traps() == 0 && cart.held(0) && cart.held(1) && cart.held(2)) {
        std::printf("S3 OVENTAPE  PASS  the drawer matches the tape (%s, %s, %s) and you leave (%.1f s)\n",
                    cart.tapeLabel(0), cart.tapeLabel(1), cart.tapeLabel(2), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 OVENTAPE  FAIL  %s  %s  pulls %d  till %d  traps %d  %d%d%d  (%.1f s)\n", cart.modeName(),
                cart.reason(), cart.pulls(), cart.drawerScore(), cart.traps(), cart.held(0) ? 1 : 0,
                cart.held(1) ? 1 : 0, cart.held(2) ? 1 : 0, frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 OVENTAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3oventape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<oventape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
